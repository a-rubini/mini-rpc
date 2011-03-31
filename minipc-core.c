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
#include <errno.h>

#include "minipc-int.h"

struct mpc_link *__mpc_base;

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
				__func__, flist, flist->name);
		return;
	}
	*nextp = flist->next;
	fprintf(link->logf, "%s: unexported function %p (%s)\n",
		__func__, flist, flist->name);
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
		errno = -ENOENT;
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

int minipc_set_logfile(struct minipc_ch *ch, FILE *logf)
{
	struct mpc_link *link = mpc_get_link(ch);

	CHECK_LINK(link);

	link->logf = logf;
	return 0;
}

