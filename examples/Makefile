
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

PROGS = trivial-server trivial-client

all: $(PROGS)

# the default puts LDFLAGS too early. Bah...
%: %.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

# This is stupid, it won't apply the first time, but, well... it works
$(PROGS): $(wildcard ../*.h ../*.a)

clean:
	rm -f *.o *~ $(PROGS)
