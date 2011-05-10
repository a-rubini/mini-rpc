/*
 * Mini-ipc: Exported functions for client operation
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
#include <poll.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "minipc-int.h"

struct minipc_ch *minipc_client_create(const char *name, int flags);

int minipc_call(struct minipc_ch *ch, const struct minipc_pd *pd,
		uint32_t *ret, void *args)
{
	struct mpc_link *link = mpc_get_link(ch);
	struct pollfd pfd;
	int i, retsize;
	static uint32_t newargs[MINIPC_MAX_ARGUMENTS];

	CHECK_LINK(link);

	/* Build the packet to send out */
	newargs[0] = pd->id.i;
	newargs[1] = pd->retval;
	for (i = 0; i < (MINIPC_MAX_ARGUMENTS - 3); i++) {
		newargs[i+2] = args[i];
		if (!args[i])
			break;
	}
	/* It has been copied up to i included */
	if (send(ch->fd, newargs, sizeof(newargs[0]) * (i + 2), 0) < 0)
		return -1;

	/* Get the reply packet and return its lenght */
	pfd.fd = ch->fd;
	pfd.events = POLLIN | POLLHUP;
	pfd.revents = 0;
	if(poll(&pfd, 1, MPC_TIMEOUT) <= 0) {
		errno = ETIMEDOUT;
		return -1;
	}
	retsize = MINIPC_GET_ASIZE(pd->retval);
	return recv(ch->fd, ret, retsize, 0);
}
