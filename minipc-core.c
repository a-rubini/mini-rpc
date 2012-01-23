/*
 * Mini-ipc: Core library functions and data
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
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/shm.h>

#include "minipc-int.h"

struct mpc_link *__mpc_base;

static int __mpc_poll_usec = MINIPC_DEFAULT_POLL;

void mpc_free_flist(struct mpc_link *link, struct mpc_flist *flist)
{
	struct mpc_flist **nextp;

	/* Look for flist and release it*/
	for (nextp = &link->flist; (*nextp); nextp = &(*nextp)->next)
		if (*nextp == flist)
			break;
	if (!*nextp) {
		if (link->logf)
			fprintf(link->logf, "%s: function not found %p (%s)\n",
				__func__, flist, flist->pd->name);
		return;
	}
	*nextp = flist->next;
	fprintf(link->logf, "%s: unexported function %p (%s)\n",
		__func__, flist->pd->f, flist->pd->name);
	free(flist);
}

int minipc_close(struct minipc_ch *ch)
{
	struct mpc_link *link = mpc_get_link(ch);
	struct mpc_link **nextp;

	CHECK_LINK(link);

	/* Look for link in our list */
	for (nextp = &__mpc_base; (*nextp); nextp = &(*nextp)->nextl)
		if (*nextp == link)
			break;

	if (!*nextp) {
		errno = ENOENT;
		return -1;
	}

	(*nextp)->nextl = link->nextl;

	if (link->logf) {
		fprintf(link->logf, "%s: found link %p (fd %i)\n",
			__func__, link, link->ch.fd);
	}
	close(ch->fd);
	/* Release allocated functions */
	while (link->flist)
		mpc_free_flist(link, link->flist);
	free(link);
	return 0;
}

int minipc_set_poll(int usec)
{
	int ret = __mpc_poll_usec;

	if (usec <= 0) {
		errno = EINVAL;
		return -1;
	}
	__mpc_poll_usec = usec;
	return ret;
}

int minipc_set_logfile(struct minipc_ch *ch, FILE *logf)
{
	struct mpc_link *link = mpc_get_link(ch);

	CHECK_LINK(link);

	link->logf = logf;
	return 0;
}

/* create a link, either server or client */
struct minipc_ch *__minipc_link_create(const char *name, int flags)
{
	struct mpc_link *link, *next;
	struct sockaddr_un sun;
	int fd, i;

	link = calloc(1, sizeof(*link));
	if (!link) return NULL;
	link->magic = MPC_MAGIC;
	link->flags = flags;
	strncpy(link->name, name, sizeof(link->name) -1);

	/* now create the socket and prepare the service */
	fd = socket(SOCK_STREAM, AF_UNIX, 0);
	if(fd < 0)
		goto out_free;
	link->ch.fd = fd;
	sun.sun_family = AF_UNIX;
	strcpy(sun.sun_path, MINIPC_BASE_PATH);
	strcat(sun.sun_path, "/");
	strcat(sun.sun_path, link->name);
	mkdir(MINIPC_BASE_PATH, 0777); /* may exist, ignore errors */

	if (flags & MPC_FLAG_SERVER) {
		for (i = 0; i < MINIPC_MAX_CLIENTS; i++)
			link->fd[i] = -1;
		FD_ZERO(&link->fdset);
		FD_SET(fd, &link->fdset);

		i = 1;
		unlink(sun.sun_path);
		if (bind (fd, (struct sockaddr *)&sun, sizeof(sun)) < 0)
			goto out_close;
		if (listen(fd, 5) < 0)
			goto out_close;
	} else { /* client */
		if (connect(fd, (struct sockaddr *)&sun, sizeof(sun)) < 0)
			goto out_close;
	}

	/* success: link to the list and return */
	link->addr = sun;
	next = __mpc_base;
	link->nextl = __mpc_base;
	__mpc_base = link;
	return &link->ch;

 out_close:
	close(fd);
 out_free:
	free(link);
	return NULL;
}
