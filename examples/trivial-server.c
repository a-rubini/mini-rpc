/*
 * Example mini-ipc server
 *
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 *
 * Released in the public domain
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/time.h>

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

static int ss_tod_function(const struct minipc_pd *pd,
			   uint32_t *args, void *ret)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	*(struct timeval *)ret = tv;
	return 0;
}

static int ss_sqrt_function(const struct minipc_pd *pd,
			    uint32_t *args, void *ret)
{
	double input, output;

	input = *(double *)args;
	output = sqrt(input);
	*(double *)ret = output;
	return 0;
}


/* Describe the three functions above */
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

const struct minipc_pd ss_tod_struct = {
	.f = ss_tod_function,
	.name = "gettimeofday",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT, struct timeval),
	.args = {
		MINIPC_ARG_END,
	},
};

const struct minipc_pd ss_sqrt_struct = {
	.f = ss_sqrt_function,
	.name = "sqrt",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_DOUBLE, double),
	.args = {
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_DOUBLE, double),
		MINIPC_ARG_END,
	},
};

int main(int argc, char **argv)
{
	struct minipc_ch *server;

	server = minipc_server_create("trivial", 0);
	if (!server) {
		fprintf(stderr, "%s: server_create(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}
	minipc_set_logfile(server, stderr);
	minipc_export(server, &ss_sum_struct);
	minipc_export(server, &ss_tod_struct);
	minipc_export(server, &ss_sqrt_struct);
	while (1) {
		if (minipc_server_action(server, 1000) < 0) {
			fprintf(stderr, "%s: server_action(): %s\n", argv[0],
				strerror(errno));
			exit(1);
		}
		fprintf(stdout, "%s: looping...\n", __func__);
	}
}
