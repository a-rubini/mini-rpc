/*
 * Mini-ipc: Exported functions for server operation
 *
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 * Based on ideas by Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

	link = calloc(1, sizeof(*link));
	if (!link) return NULL;
	link->magic = MPC_MAGIC;

	/* FIXME: create the socket and so on */

	/* success: link and return */
	next = __mpc_base;
	link->nextl = __mpc_base;
	__mpc_base = link;
	return &link->ch;
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
			flist, name, pd, pd->id, pd->retval, pd->args[0]);
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
static void mpc_handle_client(struct mpc_link *link, int i, int fd)
{
	/* FIXME */
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
	i = select(64 /* FIXME */, &set, NULL, NULL, &to);
	if (!i)
		return 0;
	if (i < 0 && errno == EINTR)
		return 0;
	if (i < 0)
		return -1;
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
