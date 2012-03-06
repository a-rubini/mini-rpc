/*
 * Mini-ipc: an example freestanding server, based in memory
 *
 * Copyright (C) 2011,2012 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 *
 * This code is copied from trivial-server, and made even more trivial
 */

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include "minipc.h"

/* A function that ignores the RPC and is written normally */
static int ss_do_sum(int a, int b)
{
	return a + b;
}

/* The following ones are RPC-aware */
static int ss_sum_function(const struct minipc_pd *pd,
			   uint32_t *args, void *ret)
{
	int i;

	i = ss_do_sum(args[0], args[1]);
	*(int *)ret = i;
	return 0;
}

static int ss_mul_function(const struct minipc_pd *pd,
			     uint32_t *args, void *ret)
{
	int a, b;

	a = *(int *)args;
	b = *(int *)(args + 1);
	*(int *)ret = a * b;
	return 0;
}


/* Describe the two functions above */
const struct minipc_pd ss_sum_struct = {
	.f = ss_sum_function,
	.name = "sum",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
		MINIPC_ARG_END,
	},
};

const struct minipc_pd ss_mul_struct = {
	.f = ss_mul_function,
	.name = "mul",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
		MINIPC_ARG_END,
	},
};

int main(int argc, char **argv)
{
	struct minipc_ch *server;

	server = minipc_server_create("mem:f000", 0);
	if (!server)
		return 1;
	minipc_export(server, &ss_sum_struct);
	minipc_export(server, &ss_mul_struct);
	while (1) {
		/* do something else... */
		minipc_server_action(server, 1000);
	}
}

