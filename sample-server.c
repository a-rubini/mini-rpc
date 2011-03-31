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
#include <sys/socket.h>

#include "minipc-int.h"

struct minipc_ch *minipc_server_create(const char *name, int flags);
struct minipc_ch *minipc_client_create(const char *name, int flags);
