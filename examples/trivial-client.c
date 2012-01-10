/*
 * Example mini-ipc client
 *
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 *
 * Released in the public domain
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include "minipc.h"

#define TRIVIAL_TIMEOUT 100 /* ms */

/* The description here is the same as in the server */
const struct minipc_pd ss_sum_struct = {
	.name = "sum",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
		MINIPC_ARG_END,
	},
};

const struct minipc_pd ss_tod_struct = {
	.name = "gettimeofday",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT, struct timeval),
	.args = {
		MINIPC_ARG_END,
	},
};

const struct minipc_pd ss_sqrt_struct = {
	.name = "sqrt",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_DOUBLE, double),
	.args = {
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_DOUBLE, double),
		MINIPC_ARG_END,
	},
};

int main(int argc, char **argv)
{
	struct minipc_ch *client;
	int a, b, c, ret;
	struct timeval tv;
	double rt_in, rt_out;

	client = minipc_client_create("trivial", 0);
	if (!client) {
		fprintf(stderr, "%s: client_create(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}
	minipc_set_logfile(client, stderr);

	/*
	 * gettod, sum, sum, gettod
	 * pause a while in-between, so several clients can be run
	 * concurrently as a load test on the server
	 */
	ret = minipc_call(client, TRIVIAL_TIMEOUT, &ss_tod_struct, &tv, NULL);
	if (ret < 0) {
		goto error;
	}
	printf("tv: %li.%06li\n", tv.tv_sec, tv.tv_usec);
	usleep(500*1000);

	a = 345; b = 628;
	ret = minipc_call(client, TRIVIAL_TIMEOUT, &ss_sum_struct, &c, a, b);
	if (ret < 0) {
		goto error;
	}
	printf("%i + %i = %i\n", a, b, c);
	usleep(500*1000);

	a = 10; b = 20;
	ret = minipc_call(client, TRIVIAL_TIMEOUT, &ss_sum_struct, &c, a, b);
	if (ret < 0) {
		goto error;
	}
	printf("%i + %i = %i\n", a, b, c);
	usleep(500*1000);

	rt_in = 2.0;
	ret = minipc_call(client, TRIVIAL_TIMEOUT, &ss_sqrt_struct, &rt_out, rt_in);
	if (ret < 0) {
		goto error;
	}
	printf("sqrt(%lf) = %lf\n", rt_in, rt_out);
	usleep(500*1000);

	return 0;

 error:
	fprintf(stderr, "Error in rpc: %s\n", strerror(errno));
	exit(1);
}
