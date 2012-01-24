/*
 * Example mini-ipc server for shmem
 * (mostly copied from the pty server code)
 *
 * Copyright (C) 2011-2012 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 *
 * Released in the public domain
 */

/*
 * This file includes the RPC functions exported by the server.
 * RPC-related stuff has been split to a different file for clarity.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "minipc.h"
#include "shmem-structs.h"

/* Run getenv and setenv on behalf of the client */
static int shm_server_do_getenv(const struct minipc_pd *pd,
			       uint32_t *args, void *ret)
{
	char *envname = (void *)args;
	char *res = getenv(envname);

	strcpy(ret, res); /* FIXME: max size */
	return 0;
}
static int shm_server_do_setenv(const struct minipc_pd *pd,
			       uint32_t *args, void *ret)
{
	char *envname = (void *)args;
	char *envval;

	args = minipc_get_next_arg(args, pd->args[0]);
	envval = (void *)args;

	setenv(envname, envval, 1);
	return 0;
}

/* add two integer numbers */
static int shm_server_do_add(const struct minipc_pd *pd,
			       uint32_t *args, void *ret)
{
	int a, b, sum;

	a = args[0];
	args = minipc_get_next_arg(args, pd->args[0]);
	b = args[0];
	sum = a + b;
	*(int *)ret = sum;
	return 0;
}

/* calculate strlen for a string of the client */
static int shm_server_do_strlen(const struct minipc_pd *pd,
			       uint32_t *args, void *ret)
{
	char *s = (void *)args;

	*(int *)ret = strlen(s);
	return 0;
}

/* strcat two remote strings and pass the result back */
static int shm_server_do_strcat(const struct minipc_pd *pd,
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
static int shm_server_do_stat(const struct minipc_pd *pd,
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

int shm_export_functions(struct minipc_ch *ch)
{
	int i;

	/* Use a handy table for the registration loop */
	static struct {
		struct minipc_pd *desc;
		minipc_f *f;
	} export_list [] = {
		{&rpc_getenv, shm_server_do_getenv},
		{&rpc_setenv, shm_server_do_setenv},
		{&rpc_add, shm_server_do_add},
		{&rpc_strlen, shm_server_do_strlen},
		{&rpc_strcat, shm_server_do_strcat},
		{&rpc_stat, shm_server_do_stat},
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

/* the main function only runs the server */
int main(int argc, char **argv)
{
	struct minipc_ch *ch;

	if (argc != 2) {
		fprintf(stderr, "%s: use \"%s <shmid>\n", argv[0], argv[0]);
		exit(1);
	}

	ch = minipc_server_create(argv[1], 0);
	if (!ch) {
		fprintf(stderr, "%s: rpc_open(%s): %s\n", argv[0], argv[1],
			strerror(errno));
		exit(1);
	}
	minipc_set_logfile(ch, stderr);

	/* Register your functions */
	if (shm_export_functions(ch)) {
		fprintf(stderr, "%s: exporting RPC functions: %s\n", argv[0],
			strerror(errno));
		exit(1);
	}

	/* Then just obey requests */
	while (1) {
		minipc_server_action(ch, 1000);
	}
}

