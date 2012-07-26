# $Id$

CC	?= gcc
RM	?= rm
CPPFLAGS	:= $(CPPFLAGS)
LDFLAGS		:= $(LDFLAGS)
SRC_FILES	:= $(wildcard *.c)
OBJ_FILES	:= $(patsubst %.c,%.o,$(SRC_FILES))

.PHONY: all
all: esos_tui

%.o: %.c
	$(CC) $(CPPFLAGS) $< -o $@

esos_tui: %.o
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
