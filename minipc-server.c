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
struct minipc_ch *minipc_server_create(const char *name, int flags)
{
	struct mpc_link *link, *next;
	struct sockaddr_un sun;
	int fd, i;

	link = calloc(1, sizeof(*link));
	if (!link) return NULL;
	link->magic = MPC_MAGIC;
	link->flags = flags;
	strncpy(link->name, name, MINIPC_MAX_NAME);
	link->name[MINIPC_MAX_NAME-1] = '\0';

	for (i = 0; i < MINIPC_MAX_CLIENTS; i++)
		link->fd[i] = -1;

	/* now create the socket and prepare the service */
	fd = socket(SOCK_STREAM, AF_UNIX, 0);
	if(fd < 0)
		goto out_free;
	sun.sun_family = AF_UNIX;
	strcpy(sun.sun_path, "/tmp/.minipc-");
	strcat(sun.sun_path, link->name);
	unlink(sun.sun_path);
	if (bind (fd, (struct sockaddr *)&sun, sizeof(sun)) < 0)
		goto out_free;
	if (listen(fd, 5) < 0)
		goto out_free;

	/* success: link to the list and return */
	next = __mpc_base;
	link->nextl = __mpc_base;
	__mpc_base = link;
	return &link->ch;
 out_free:
	free(link);
	return NULL;
}

/*
 * The following ones add to the export list and remove from it
 */
int minipc_export(struct minipc_ch *ch, const char *name,
		  const struct minipc_pd *pd)
{
	struct mpc_link *link = mpc_get_link(ch);
	struct mpc_flist *flist;

	CHECK_LINK(link);

	flist = calloc(1, sizeof(*flist));
	if (!flist)
		return -1;
	flist->name = name;
	flist->pd = pd;
	flist->next = link->flist;
	link->flist = flist;
	if (link->logf)
		fprintf(link->logf, "%s: exported %p (%s) with pd %p --"
			"id %08x, retval %08x, args %08x...\n", __func__,
			flist, name, pd, pd->id.i, pd->retval, pd->args[0]);
	return 0;
}

int minipc_unexport(struct minipc_ch *ch, const char *name,
		    const struct minipc_pd *pd)
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

/*
 * Internal functions used by server action below: handle a request
 * or the arrival of a new client
 */
static void mpc_handle_client(struct mpc_link *link, int pos, int fd)
{
	static uint32_t args[MINIPC_MAX_ARGUMENTS];
	static uint32_t ret[MINIPC_MAX_ARGUMENTS];
	const struct minipc_pd *pd;
	struct mpc_flist *flist;
	int i;

	i = recv(fd, args, sizeof(args), 0);
	if (i < 0 && errno == EINTR)
		 return;
	if (i <= 0) {
		if (link->logf)
			fprintf(link->logf, "%s: error %i in fd %i, closing\n",
				__func__, i < 0 ? errno : 0, fd);
		close(fd);
		link->fd[pos] = -1;
		return;
	}
	/* Write last-arg as 0, to be safer (but not safe) */
	i /= sizeof(args[0]);
	if (i >= 0) args[i-1] = 0;

	/* Arg[0] is the id of the function */
	for (flist = link->flist; flist; flist = flist->next)
		if (flist->pd->id.i == args[0])
			break;
	if (!flist) {
		if (link->logf)
			fprintf(link->logf, "%s: id %08x not found\n",
				__func__, args[0]);
		return;
	}
	pd = flist->pd;
	if (link->logf)
		fprintf(link->logf, "%s: request for %08x (%s)\n",
			__func__, args[0], flist->name);
	/* Call the function, return value */
	i = pd->f(pd, args, ret);
	if (i < 0) {
		ret[0] = __MINIPC_ARG_ENCODE(MINIPC_AT_ERROR, ret[0]);
		i = 1; /* number of words to send back */
	}
	send(fd, ret, i * sizeof(ret[0]), 0);
}

static void mpc_handle_server(struct mpc_link *link, int fd)
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
	/* First, look for all clients */
	for (i = 0; i < MINIPC_MAX_CLIENTS; i++) {
		if (link->fd[i] < 0)
			continue;
		if (!FD_ISSET(link->fd[i], &set))
			continue;
		mpc_handle_client(link, i, link->fd[i]);
	}
	/* Finally, look for a new client */
	if (!FD_ISSET(ch->fd, &set))
		mpc_handle_server(link, ch->fd);
	return 0;
}
