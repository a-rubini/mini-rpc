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

int minipc_call(struct minipc_ch *ch, int millisec_timeout,
		const struct minipc_pd *pd, void *ret, ...)
{
	struct mpc_link *link = mpc_get_link(ch);
	struct mpc_shmem *shm = link->memaddr;
	struct pollfd pfd;
	int i, narg, size, retsize, pollnr;
	int atype, asize;
	va_list ap;
	struct mpc_req_packet *p_out, _pkt_out = {"",};
	struct mpc_rep_packet *p_in, _pkt_in;

	CHECK_LINK(link);

	if (shm) {
		p_out = &shm->request;
		p_in = &shm->reply;
	} else {
		p_out = & _pkt_out;
		p_in = & _pkt_in;
	}

	/* Build the packet to send out -- marshall args */
	if (link->logf) {
		fprintf(link->logf, "%s: calling \"%s\"\n",
			__func__, pd->name);
	}
	memcpy(p_out->name, pd->name, MINIPC_MAX_NAME);

	va_start(ap, ret);
	for (i = narg = 0; ; i++) {
		int next_narg = narg;

		atype = MINIPC_GET_ATYPE(pd->args[i]);
		asize = MINIPC_GET_ASIZE(pd->args[i]);
		next_narg += MINIPC_GET_ANUM(asize);
		if (next_narg >= MINIPC_MAX_ARGUMENTS) /* unlikely */
			goto doesnt_fit;

		switch (atype) {
		case MINIPC_ATYPE_NONE:
			goto out; /* end of list */
		case MINIPC_ATYPE_INT:
			p_out->args[narg++] = va_arg(ap, int);
			break;
		case MINIPC_ATYPE_INT64:
			*(uint64_t *)(p_out->args + narg)
				= va_arg(ap, uint64_t);
			narg = next_narg;
			break;
		case MINIPC_ATYPE_DOUBLE:
			*(double *)(p_out->args + narg) = va_arg(ap, double);
			narg = next_narg;
			break;
		case MINIPC_ATYPE_STRING:
		{
			char *sin = va_arg(ap, void *);
			char *sout = (void *)(p_out->args + narg);
			int slen = strlen(sin);
			int alen;

			/*
			 * argument len is arbitrary, terminate and 4-align
			 * we can't use next_narg here, as len changes
			 */
			alen = MINIPC_GET_ANUM(slen + 1);
			if (narg + alen >= MINIPC_MAX_ARGUMENTS)
				goto doesnt_fit;
			strcpy(sout, sin);
			narg += alen;
			break;
		}
		case MINIPC_ATYPE_STRUCT:
			memcpy(p_out->args + narg, va_arg(ap, void *), asize);
			narg = next_narg;
			break;

		default:
			if (link->logf) {
				fprintf(link->logf, "%s: unkown type 0x%x\n",
					__func__, atype);
			}
			errno = EPROTO;
			return -1;
		}
	}
 out:
	va_end(ap);

	if (shm) {
		shm->nrequest++;
	} else {
		size = sizeof(p_out->name) + sizeof(p_out->args[0]) * narg;
		if (send(ch->fd, p_out, size, 0) < 0) {
			/* errno already set */
			return -1;
		}
	}

	/* Wait for the reply packet */
	pfd.fd = ch->fd;
	pfd.events = POLLIN | POLLHUP;
	pfd.revents = 0;
	pollnr = poll(&pfd, 1, millisec_timeout);
	if (pollnr < 0) {
		/* errno already set */
		return -1;
	}
	if (pollnr == 0) {
		errno = ETIMEDOUT;
		return -1;
	}

	if (shm) {
		read(ch->fd, &i, 1);
		size = retsize = sizeof(shm->reply);
	} else {
		/* this "size" is wrong for strings, so recv the max size */
		size = MINIPC_GET_ASIZE(pd->retval) + sizeof(uint32_t);
		retsize = recv(ch->fd, p_in, sizeof(*p_in), 0);
	}
	/* if very short, we have a problem */
	if (retsize < (sizeof(p_in->type)) + sizeof(int))
		goto too_short;
	/* remote error reported */
	if (MINIPC_GET_ATYPE(p_in->type) == MINIPC_ATYPE_ERROR) {
		int remoteerr = *(int *)&p_in->val;

		if (link->logf) {
			fprintf(link->logf, "%s: remote error \"%s\"\n",
				__func__, strerror(remoteerr));
		}
		*(int *)ret = remoteerr;
		errno = EREMOTEIO;
		return -1;
	}
	/* another check: the return type must match */
	if (MINIPC_GET_ATYPE(p_in->type) != MINIPC_GET_ATYPE(pd->retval)) {
		if (link->logf) {
			fprintf(link->logf, "%s: wrong code %08x (not %08x)\n",
				__func__, p_in->type, pd->retval);
		}
		errno = EPROTO;
		return -1;
	}
	/* check size */
	if (retsize < size)
		goto too_short;
	/* all good */
	memcpy(ret, &p_in->val, MINIPC_GET_ASIZE(p_in->type));
	return 0;

too_short:
	if (link->logf) {
		fprintf(link->logf, "%s: short reply (%i bytes)\n",
			__func__, retsize);
	}
	errno = EPROTO;
	return -1;

doesnt_fit:
	if (link->logf) {
		fprintf(link->logf, "%s: rpc call \"%s\" won't fit %i slots\n",
			__func__, pd->name, MINIPC_MAX_ARGUMENTS);
	}
	errno = EPROTO;
	return -1;
}
