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
#include <sys/time.h>

#include "minipc.h"


/* The description here is the same as in the server */
const struct minipc_pd ss_sum_struct = {
	.id = 0x73756d00, /* "sum\0" */
	.retval = MINIPC_ARG_ENCODE(MINIPC_AT_INT, int),
	.args = {
		MINIPC_ARG_ENCODE(MINIPC_AT_INT, int),
		MINIPC_ARG_ENCODE(MINIPC_AT_INT, int),
		0
	},
};

const struct minipc_pd ss_tod_struct = {
	.id = 0x746f6400, /* "tod\0" */
	.retval = MINIPC_ARG_ENCODE(MINIPC_AT_STRUCT, struct timeval),
	.args = {
		0
	},
};

int main(int argc, char **argv)
{
	struct minipc_ch *server;
	uint32_t args[16];
	uint32_t ret[4];
	int a, b, c;
	struct timeval tv;

	server = minipc_client_create("sample", 0);
	if (!server)
		exit(1);
	minipc_set_logfile(server, stderr);

	/* gettod, sum, sum, gettod */
	minipc_marshall_args(&ss_tod_struct, args, NULL);
	minipc_call(server, &ss_tod_struct, args, ret);
	minipc_unmarshall_ret(&ss_tod_struct, ret, &tv, NULL);


	a = 345; b = 628;
	minipc_marshall_args(&ss_sum_struct, args, &a, &b, NULL);
	minipc_call(server, &ss_sum_struct, args, ret);
	minipc_unmarshall_ret(&ss_sum_struct, ret, &c, NULL);
	printf("%i + %i = %i\n", a, b, c);

	a = 10; b = 20;
	minipc_marshall_args(&ss_sum_struct, args, &a, &b, NULL);
	minipc_call(server, &ss_sum_struct, args, ret);
	minipc_unmarshall_ret(&ss_sum_struct, ret, &c, NULL);
	printf("%i + %i = %i\n", a, b, c);

	minipc_export(server, "sum", &ss_sum_struct);
	minipc_export(server, "gettimeofday", &ss_tod_struct);
	while (1) {
		minipc_server_action(server, 1000);
		fprintf(stdout, "%s: looping...\n", __func__);
	}
}
