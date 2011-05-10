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
#include <sys/time.h>

#include "minipc.h"

static int ss_do_sum(int a, int b)
{
	return a + b;
}


/* These are the ones called from outside */
static int ss_sum_function(const struct minipc_pd *pd,
		       uint32_t *args, void *ret)
{
	int c;

	/* unmarshall the arguments, call, marshall back */
	c = ss_do_sum(args[0], args[1]);
	*(int *)ret = c;
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

const struct minipc_pd ss_tod_struct = {
	.f = ss_tod_function,
	.name = "gettimeofday",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT, struct timeval),
	.args = {
		MINIPC_ARG_END,
	},
};

int main(int argc, char **argv)
{
	struct minipc_ch *server;

	server = minipc_server_create("sample", 0);
	if (!server) {
		fprintf(stderr, "%s: server_create(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}
	minipc_set_logfile(server, stderr);
	minipc_export(server, &ss_sum_struct);
	minipc_export(server, &ss_tod_struct);
	while (1) {
		if (minipc_server_action(server, 1000) < 0) {
			fprintf(stderr, "%s: server_action(): %s\n", argv[0],
				strerror(errno));
			exit(1);
		}
		fprintf(stdout, "%s: looping...\n", __func__);
	}
}
