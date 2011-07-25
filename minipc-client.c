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
#include <unistd.h>
#include <stdarg.h>
#include <poll.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "minipc-int.h"

struct minipc_ch *minipc_client_create(const char *name, int f)
{
	return __minipc_link_create(name, MPC_USER_FLAGS(f) | MPC_FLAG_CLIENT);
}

int minipc_call(struct minipc_ch *ch, const struct minipc_pd *pd,
		void *ret, ...)
{
	struct mpc_link *link = mpc_get_link(ch);
	struct pollfd pfd;
	int i, narg, size, retsize;
	va_list ap;
	struct mpc_req_packet pkt_out = {"",};
	struct mpc_rep_packet pkt_in;

	CHECK_LINK(link);

	/* Build the packet to send out -- marshall args */
	strcpy(pkt_out.name, pd->name);
	va_start(ap, ret);
	for (i = narg = 0; ; i++) {
		if (narg >= MINIPC_MAX_ARGUMENTS) /* unlikely */
			break;

		switch (MINIPC_GET_ATYPE(pd->args[i])) {
		case MINIPC_ATYPE_NONE:
			goto out; /* end of list */
		case MINIPC_ATYPE_INT:
			pkt_out.args[narg++] = va_arg(ap, int);
			break;
		case MINIPC_ATYPE_INT64:
			*(uint64_t *)(pkt_out.args + narg)
				= va_arg(ap, uint64_t);
			narg += sizeof(uint64_t) / sizeof(pkt_out.args[0]);
			break;
		case MINIPC_ATYPE_DOUBLE:
			*(double *)(pkt_out.args + narg) = va_arg(ap, double);
			narg += sizeof(double) / sizeof(pkt_out.args[0]);
			break;

		default:
			/* FIXME: structures and strings */
			break;
		}
	}
 out:
	va_end(ap);

	size = sizeof(pkt_out.name) + sizeof(pkt_out.args[0]) * narg;
	if (send(ch->fd, &pkt_out, size, 0) < 0)
		return -1;

	/* Get the reply packet and return its lenght */
	pfd.fd = ch->fd;
	pfd.events = POLLIN | POLLHUP;
	pfd.revents = 0;
	if(poll(&pfd, 1, MPC_TIMEOUT) <= 0) {
		errno = ETIMEDOUT;
		return -1;
	}
	size = MINIPC_GET_ASIZE(pd->retval) + sizeof(uint32_t);
	retsize = recv(ch->fd, &pkt_in, size, 0);
	if (retsize != size) { /* FIXME: differentiate -1 from short reply */
		*(int *)ret = errno;
		errno = EREMOTEIO;
		return -1;
	}
	/* another check: the return type must match */
	if (pkt_in.type != pd->retval) {
		*(int *)ret = EINVAL;
		errno = EREMOTEIO;
		return -1;
	}
	memcpy(ret, &pkt_in.val, MINIPC_GET_ASIZE(pd->retval));
	return 0; /* FIXME: return size must be checked */
}
