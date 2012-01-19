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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "minipc.h"
#include "pty-server.h"

static int saved_fdm; /* file descriptor of pty master, used by "feed" */
static struct pty_counts *saved_pc; /* counters, used by "count" */


/* Return the count accessing the pointer to the structure within main() */
static int pty_server_do_count(const struct minipc_pd *pd,
			       uint32_t *args, void *ret)
{
	*(struct pty_counts *)ret = *saved_pc;
	return 0;
}

/* Run getenv and setenv on behalf of the client */
static int pty_server_do_getenv(const struct minipc_pd *pd,
			       uint32_t *args, void *ret)
{
	char *envname = (void *)args;
	char *res = getenv(envname);

	strcpy(ret, res); /* FIXME: max size */
	return 0;
}
static int pty_server_do_setenv(const struct minipc_pd *pd,
			       uint32_t *args, void *ret)
{
	char *envname = (void *)args;
	char *envval;

	args = minipc_get_next_arg(args, pd->args[0]);
	envval = (void *)args;

	setenv(envname, envval, 1);
	return 0;
}

/* feed a string to the sub-shell (adding a newline) */
static int pty_server_do_feed(const struct minipc_pd *pd,
			       uint32_t *args, void *ret)
{
	char *command = (void *)args;
	int wrote;

	wrote = write(saved_fdm, command, strlen(command));
	wrote += write(saved_fdm, "\n", 1);
	return 0;
}

/* calculate strlen for a string of the client */
static int pty_server_do_strlen(const struct minipc_pd *pd,
			       uint32_t *args, void *ret)
{
	char *s = (void *)args;

	*(int *)ret = strlen(s);
	return 0;
}

/* strcat two remote strings and pass the result back */
static int pty_server_do_strcat(const struct minipc_pd *pd,
			       uint32_t *args, void *ret)
{
	char *s, *t;
	char scat[256];

	s = (void *)args;
	args = minipc_get_next_arg(args, pd->args[0]);
	t = (void *)args;

	strncpy(scat, s, sizeof(scat));
	strncat(scat, t, sizeof(scat) - strlen(s));

	strcpy(ret, scat); /* FIXME: max size */
	return 0;
}

/* stat a pathname received from the remote party */
static int pty_server_do_stat(const struct minipc_pd *pd,
			       uint32_t *args, void *ret)
{
	struct stat stbuf;
	char *fname = (void *)args;

	if (stat(fname, &stbuf) < 0)
		return -1;
	memcpy(ret, &stbuf, sizeof(stbuf));
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
		{&rpc_getenv, pty_server_do_getenv},
		{&rpc_setenv, pty_server_do_setenv},
		{&rpc_feed, pty_server_do_feed},
		{&rpc_strlen, pty_server_do_strlen},
		{&rpc_strcat, pty_server_do_strcat},
		{&rpc_stat, pty_server_do_stat},
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


