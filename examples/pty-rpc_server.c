/*
 * Example mini-ipc server
 *
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 *
 * Released in the public domain
 */

/*
 * This file includes the RPC functions exported by the server.
 * RPC-related stuff has been split to a different file for clarity.
 */
#include "minipc.h"
#include "pty-server.h"

static int saved_fdm; /* file descriptor of pty master, used by "seed" */
static struct pty_counts *saved_pc; /* counters, used by "count" */


/* Return the count accessing the pointer to the structure within main() */
static int pty_server_do_count(const struct minipc_pd *pd,
			       uint32_t *args, void *ret)
{
	*(struct pty_counts *)ret = *saved_pc;
	return 0;
}



/*
 * The following is called by the main function, and exports stuff.
 * All the rest happens through callbacks from the library
 */
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

int pty_export_functions(struct minipc_ch *ch, int fdm, struct pty_counts *pc)
{
	int i;

	/* save data for the callbacks */
	saved_fdm = fdm;
	saved_pc = pc;

	/* Use a handy table for the registration loop */
	static struct {
		struct minipc_pd *desc;
		minipc_f *f;
	} export_list [] = {
		{&rpc_count, pty_server_do_count},
	};

	/*
	 * Complete the data structures by filling the function pointers
	 * and then register each of the exported procedures
	 */
	for (i = 0; i < ARRAY_SIZE(export_list); i++) {
		export_list[i].desc->f = export_list[i].f;
		if (minipc_export(ch, export_list[i].desc) < 0)
			return -1;
	}

	return 0;
}


