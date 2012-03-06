
AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(CROSS_COMPILE)gcc
CPP             = $(CC) -E
AR              = $(CROSS_COMPILE)ar
NM              = $(CROSS_COMPILE)nm
STRIP           = $(CROSS_COMPILE)strip
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump

CFLAGS = -Wall -ggdb -O2 -fno-strict-aliasing
LDFLAGS = -L. -lminipc -lm

# We need to support freestading environments: an embedded CPU that
# sits as a server on its own memory are and awaits commands
# The user can pass IPC_FREESTANDING=y
IPC_FREESTANDING ?= $(shell ./check-freestanding $(CC))
IPC_HOSTED ?=       $(shell ./check-freestanding -n $(CC))

# Hosted is the opposite of freestanding, and cflags change too
ifeq ($(IPC_FREESTANDING),y)
  IPC_HOSTED = n
  CFLAGS += -ffreestanding -Iarch-$(ARCH)
else
  IPC_HOSTED = y
endif

OBJ-$(IPC_HOSTED) = minipc-core.o minipc-server.o minipc-client.o
OBJ-$(IPC_FREESTANDING) = minipc-mem-server.o

LIB = libminipc.a

# export these to the examples (but if you make there IPC_HOSTED is default
export IPC_FREESTANDING IPC_HOSTED


all: $(LIB)
	$(MAKE) -C examples

$(LIB): $(OBJ-y)
	$(AR) r $@ $^

# the default puts LDFLAGS too early. Bah...
%: %.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

# This is stupid, it won't apply the first time, but, well... it works
$(wildcard *.o): $(wildcard *.h)

clean:
	rm -f *.o *~ $(LIB)
	$(MAKE) -C examples clean
