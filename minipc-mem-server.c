/*
 * Mini-ipc: Exported functions for freestanding server
 *
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 *
 * This replicates some code of minipc-core and minipc-server.
 * It implements the functions needed to make a freestanding server
 * (for example, an lm32 running on an FPGA -- the case I actually need).
 */

#include "minipc-int.h"
#include <string.h>
#include <sys/errno.h>

/* HACK: use a static link, one only */
static struct mpc_link __static_link;

/* The create function just picks an hex address from the name "mem:AABBCC" */
struct minipc_ch *minipc_server_create(const char *name, int flags)
{
	struct mpc_link *link = &__static_link;
	int i, c, addr = 0;
	int memsize = sizeof(struct mpc_shmem);

	if (link->magic) {
		errno = EBUSY;
		return NULL;
	}

	/* Most code from __minipc_link_create and __minipc_memlink_create */
	flags |= MPC_FLAG_SERVER;

	if (strncmp(name, "mem:", 4)) {
		errno = EINVAL;
		return NULL;
	}

	/* Ok, valid name. Hopefully */
	link->magic = MPC_MAGIC;
	link->flags = flags;
	strncpy(link->name, name, sizeof(link->name) -1);

	/* Parse the hex address */
	for (i = 4; (c = name[i]); i++) {
		addr *= 0x10;
		if (c >= '0' && c <= '9') addr += c - '0';
		if (c >= 'a' && c <= 'f') addr += c - 'a' + 10;
		if (c >= 'A' && c <= 'F') addr += c - 'A' + 10;
	}
	link->flags |= MPC_FLAG_SHMEM; /* needed? */

	link->memaddr = (void *)addr;
	link->memsize = memsize;
	link->pid = 0; /* hack: nrequest */
	if (link->flags & MPC_FLAG_SERVER)
		memset(link->memaddr, 0, memsize);

	return &link->ch;
}

/* Close only marks the link as available */
int minipc_close(struct minipc_ch *ch)
{
	struct mpc_link *link = mpc_get_link(ch);
	CHECK_LINK(link);
	link->magic = 0; /* available */
	return 0;
}

/* HACK: use a static array of flist, to avoid malloc and free */
static struct mpc_flist __static_flist[MINIPC_MAX_EXPORT];

static void *calloc(size_t unused, size_t unused2)
{
	int i;
	struct mpc_flist *p;

	for (p = __static_flist, i = 0; i < MINIPC_MAX_EXPORT; p++, i++)
		if (!p->pd)
			break;
	if (i == MINIPC_MAX_EXPORT) {
		errno = ENOMEM;
		return NULL;
	}
	return p;
}

static void free(void *ptr)
{
	struct mpc_flist *p = ptr;
	p->pd = NULL;
}


/* From: minipc-core.c, but relying on fake free above */
void mpc_free_flist(struct mpc_link *link, struct mpc_flist *flist)
{
	struct mpc_flist **nextp;

	/* Look for flist and release it*/
	for (nextp = &link->flist; (*nextp); nextp = &(*nextp)->next)
		if (*nextp == flist)
			break;
	if (!*nextp) {
		return;
	}
	*nextp = flist->next;
	free(flist);
}


/* From: minipc-server.c -- but no log and relies on fake calloc above */
int minipc_export(struct minipc_ch *ch, const struct minipc_pd *pd)
{
	struct mpc_link *link = mpc_get_link(ch);
	struct mpc_flist *flist;

	CHECK_LINK(link);

	flist = calloc(1, sizeof(*flist));
	if (!flist)
		return -1;
	flist->pd = pd;
	flist->next = link->flist;
	link->flist = flist;
	return 0;
}

/* From: minipc-server.c -- but no log file */
int minipc_unexport(struct minipc_ch *ch, const struct minipc_pd *pd)
{
	struct mpc_link *link = mpc_get_link(ch);
	struct mpc_flist *flist;

	CHECK_LINK(link);
	/* We must find the flist that points to pd */
	for (flist = link->flist; flist; flist = flist->next)
		if (flist->pd == pd)
			break;
	if (!flist) {
		errno = EINVAL;
		return -1;
	}
	flist = container_of(&pd, struct mpc_flist, pd);
	mpc_free_flist(link, flist);
	return 0;
}


/* From: minipc-server.c */
uint32_t *minipc_get_next_arg(uint32_t arg[], uint32_t atype)
{
	int asize;
	char *s = (void *)arg;

	if (MINIPC_GET_ATYPE(atype) != MINIPC_ATYPE_STRING)
		asize = MINIPC_GET_ANUM(MINIPC_GET_ASIZE(atype));
	else
		asize = MINIPC_GET_ANUM(strlen(s) + 1);
	return arg + asize;
}

/* From: minipc-server.c (mostly: mpc_handle_client) */
int minipc_server_action(struct minipc_ch *ch, int timeoutms)
{
	struct mpc_link *link = mpc_get_link(ch);

	struct mpc_req_packet *p_in;
	struct mpc_rep_packet *p_out;
	struct mpc_shmem *shm = link->memaddr;
	const struct minipc_pd *pd;
	struct mpc_flist *flist;
	int i;

	CHECK_LINK(link);

	/* keep track of the request in an otherwise unused field */
	if (shm->nrequest == link->pid)
		return 0;
	link->pid = shm->nrequest;

	p_in = &shm->request;
	p_out = &shm->reply;

	/* use p_in->name to look for the function */
	for (flist = link->flist; flist; flist = flist->next)
		if (!(strcmp(p_in->name, flist->pd->name)))
			break;
	if (!flist) {
		p_out->type = MINIPC_ARG_ENCODE(MINIPC_ATYPE_ERROR, int);
		*(int *)(&p_out->val) = EOPNOTSUPP;
		goto send_reply;
	}
	pd = flist->pd;

	/* call the function and send back stuff */
	i = pd->f(pd, p_in->args, p_out->val);
	if (i < 0) {
		p_out->type = MINIPC_ARG_ENCODE(MINIPC_ATYPE_ERROR, int);
		*(int *)(&p_out->val) = errno;
	} else {
		/* Use retval, but fix the length for strings */
		if (MINIPC_GET_ATYPE(pd->retval) == MINIPC_ATYPE_STRING) {
			int size = strlen((char *)p_out->val) + 1;

			size = (size + 3) & ~3; /* align */
			p_out->type =
				__MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, size);
		} else {
			p_out->type = pd->retval;
		}
	}

 send_reply:
	shm->nreply++; /* message already in place */
	return 0;
}
