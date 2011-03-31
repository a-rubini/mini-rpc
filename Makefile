
AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(CROSS_COMPILE)gcc
CPP             = $(CC) -E
AR              = $(CROSS_COMPILE)ar
NM              = $(CROSS_COMPILE)nm
STRIP           = $(CROSS_COMPILE)strip
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump

CFLAGS = -Wall -ggdb -O2
LDFLAGS = -L. -lminipc

OBJ = minipc-core.o minipc-server.o minipc-client.o

LIB = libminipc.a
PROGS = sample-server sample-client

all: $(LIB) $(PROGS)

$(LIB): $(OBJ)
	$(AR) r $@ $^

# This is stupid, it won't apply the first time, but, well... it works
$(wildcard *.o): $(wildcard *.h)

clean:
	rm -f *.o *~ $(LIB) $(PROGS)