CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -Wextra -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lncurses

PREFIX = /usr
MAKE_USER := $(or $(SUDO_USER),$(DOAS_USER),$(USER))
MAKE_GROUP := $(shell id -gn $(MAKE_USER))
MAKE_HOME := $(shell getent passwd $(MAKE_USER) | cut -d: -f6)
