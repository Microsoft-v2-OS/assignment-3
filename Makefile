CC      = gcc
CFLAGS  = -Wall -Wextra -O2

SRCS = main.c banner.c malloc_handler.c mmap_handler.c demand_paging.c \
       memory_monitor.c logger_std.c logger_sys.c
OBJS = $(SRCS:.c=.o)

.PHONY: all static dynamic clean

all: static dynamic

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

static: mem_static

dynamic: mem_dynamic

mem_static: $(OBJS)
	$(CC) $(CFLAGS) -static $(OBJS) -o $@

mem_dynamic: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

$(OBJS): $(wildcard *.h)

clean:
	rm -f $(OBJS) mem_static mem_dynamic