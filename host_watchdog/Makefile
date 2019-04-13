all:

CFLAGS := -g -pipe -W -Wall -Wextra -O2

.PHONY: all
all: host_watchdog

host_watchdog: CFLAGS += -I.

.PHONY: clean
clean:
	$(RM) host_watchdog
