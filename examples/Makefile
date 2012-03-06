
# examples for mini-rpc; depends on ..

AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(CROSS_COMPILE)gcc
CPP             = $(CC) -E
AR              = $(CROSS_COMPILE)ar
NM              = $(CROSS_COMPILE)nm
STRIP           = $(CROSS_COMPILE)strip
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump

CFLAGS = -Wall -ggdb -I.. -O2
LDFLAGS = -L.. -lminipc -lm

# we may be hosted or freestanding. For freestanding there is one
# example only. The following variables are exported by the upper
# Makefile, but if you "make" in this directory, it checks by itself
IPC_FREESTANDING ?= $(shell ../check-freestanding $(CC))
IPC_HOSTED ?=       $(shell ../check-freestanding -n $(CC))

# Hosted is the opposite of freestanding, and cflags change too
ifeq ($(IPC_FREESTANDING),y)
  IPC_HOSTED = n
  CFLAGS += -ffreestanding -I../arch-$(ARCH)
else
  IPC_HOSTED = y
endif


PROGS-$(IPC_HOSTED) = trivial-server trivial-client
PROGS-$(IPC_HOSTED) += pty-server pty-client
PROGS-$(IPC_HOSTED) += mbox-process mbox-bridge mbox-client
PROGS-$(IPC_HOSTED) += shmem-server shmem-client

IPC_FREESTANDING ?= n

PROGS-$(IPC_FREESTANDING) += freestanding-server

all: $(PROGS-y)

# the default puts LDFLAGS too early. Bah...
%: %.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

pty-server: pty-server.o pty-rpc_server.o pty-rpc_structs.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -lutil -o $@

pty-client: pty-client.o pty-rpc_structs.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

shmem-server: shmem-server.o shmem-structs.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

shmem-client: shmem-client.o shmem-structs.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@


# This is stupid, it won't apply the first time, but, well... it works
$(PROGS-y) $(wildcard *.o): $(wildcard ../*.h ../*.a)

clean:
	rm -f *.o *~ $(PROGS)
