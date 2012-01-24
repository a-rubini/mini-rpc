/*
 * Mini-ipc: Exported functions for server operation
 *
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 * Based on ideas and code by Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>

#include "minipc-int.h"

/*
 * This function creates a server structure and links it to the
 * process-wide list of links
 */
struct minipc_ch *minipc_server_create(const char *name, int f)
{
	return __minipc_link_create(name, MPC_USER_FLAGS(f) | MPC_FLAG_SERVER);
}

/*
 * The following ones add to the export list and remove from it
 */
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
	if (link->logf)
		fprintf(link->logf, "%s: exported %p (%s) with pd %p --"
			" retval %08x, args %08x...\n", __func__,
			flist, pd->name, pd, pd->retval, pd->args[0]);
	return 0;
}

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
		if (link->logf)
			fprintf(link->logf, "%s: not found pd %p\n",
					   __func__, pd);
		errno = EINVAL;
		return -1;
	}
	flist = container_of(&pd, struct mpc_flist, pd);
	mpc_free_flist(link, flist);
	return 0;
}

/* Return the current fdset associated to the service */
int minipc_server_get_fdset(struct minipc_ch *ch, fd_set *setptr)
{
	struct mpc_link *link = mpc_get_link(ch);

	CHECK_LINK(link);
	*setptr = link->fdset;
	return 0;
}

/* Get a pointer to the next argument in the array */
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


/*
 * Internal functions used by server action below: handle a request
 * or the arrival of a new client
 */
static void mpc_handle_client(struct mpc_link *link, int pos, int fd)
{
	struct mpc_req_packet *p_in, _pkt_in;
	struct mpc_rep_packet *p_out, _pkt_out;
	struct mpc_shmem *shm = link->memaddr;
	const struct minipc_pd *pd;
	struct mpc_flist *flist;
	int i;

	if (shm) {
		p_in = &shm->request;
		p_out = &shm->reply;
		/* read one byte, it's just a signal */
		read(fd, &i, 1);
	} else {
		p_in = & _pkt_in;
		p_out = & _pkt_out;
		/* receive the packet and manage errors */
		i = recv(fd, p_in, sizeof(*p_in), 0);
		if (i < 0 && errno == EINTR)
			return;
		if (i <= 0)
			goto close_client;
	}

	/* use p_in->name to look for the function */
	for (flist = link->flist; flist; flist = flist->next)
		if (!(strcmp(p_in->name, flist->pd->name)))
			break;
	if (!flist) {
		if (link->logf)
			fprintf(link->logf, "%s: function %s not found\n",
				__func__, p_in->name);
		p_out->type = MINIPC_ARG_ENCODE(MINIPC_ATYPE_ERROR, int);
		*(int *)(&p_out->val) = EOPNOTSUPP;
		goto send_reply;
	}
	pd = flist->pd;
	if (link->logf)
		fprintf(link->logf, "%s: request for %s\n",
			__func__, pd->name);

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
	if (shm) {
		shm->nreply++; /* message already in place */
		return;
	}
	/* send a 32-bit value plus the declared return length */
	if (send(fd, p_out, sizeof(p_out->type)
	     + MINIPC_GET_ASIZE(p_out->type), MSG_NOSIGNAL) < 0)
		goto close_client;
	return;

 close_client:
	if (link->logf)
		fprintf(link->logf, "%s: error %i in fd %i, closing\n",
			__func__, i < 0 ? errno : 0, fd);
	close(fd);
	FD_CLR(fd, &link->fdset);
	link->fd[pos] = -1;
	return;
}

static void mpc_handle_connection(struct mpc_link *link, int fd)
{
	int i, newfd;
	struct sockaddr_un sun;
	socklen_t slen = sizeof(sun);

	newfd = accept(fd, (struct sockaddr *)&sun, &slen);
	if (link->logf)
		fprintf(link->logf, "%s: accept returned fd %i (error %i)\n",
			__func__, newfd, newfd < 0 ? errno : 0);
	if (newfd < 0)
		return;
	/* Lookf for a place for this */
	for (i = 0; i < MINIPC_MAX_CLIENTS; i++)
		if (link->fd[i] < 0)
			break;
	if (i == MINIPC_MAX_CLIENTS) {
		if (link->logf)
			fprintf(link->logf, "%s: refused: too many clients\n",
				__func__);
		close(newfd);
		return;
	}
	link->fd[i] = newfd;
	FD_SET(newfd, &link->fdset);
}


/*
 * The server action returns an error or zero. If the user wants
 * the list of active descriptors, it must ask for the filemask
 * (at this point there is no support for poll, only select)
 */
int minipc_server_action(struct minipc_ch *ch, int timeoutms)
{
	struct mpc_link *link = mpc_get_link(ch);
	struct timeval to;
	fd_set set;
	int i;

	CHECK_LINK(link);

	to.tv_sec = timeoutms/1000;
	to.tv_usec = (timeoutms % 1000) * 1000;
	set = link->fdset;
	i = select(64 /* FIXME: hard constant */, &set, NULL, NULL, &to);
	if (!i)
		return 0;
	if (i < 0 && errno == EINTR)
		return 0;
	if (i < 0)
		return -1;
	if (i == 0) {
		errno = EAGAIN;
		return -1;
	}

	/* A shmem server has only one descriptor, for one client */
	if (link->memaddr) {
		mpc_handle_client(link, -1, ch->fd);
		return 0;
	}

	/* First, look for all clients */
	for (i = 0; i < MINIPC_MAX_CLIENTS; i++) {
		if (link->fd[i] < 0)
			continue;
		if (!FD_ISSET(link->fd[i], &set))
			continue;
		mpc_handle_client(link, i, link->fd[i]);
	}
	/* Finally, look for a new client */
	if (FD_ISSET(ch->fd, &set))
		mpc_handle_connection(link, ch->fd);
	return 0;
}
