# $Id$

CC		?= gcc
RM		?= rm
CPPFLAGS	:= $(CPPFLAGS)
LDFLAGS		:= $(LDFLAGS)
SRC_FILES	:= $(wildcard *.c)
OBJ_FILES	:= $(patsubst %.c,%.o,$(SRC_FILES))

.PHONY: all
all: esos_tui

.PHONY: clean
clean:
	$(RM) $(OBJ_FILES)
	$(RM) esos_tui

%.o: %.c
	$(CC) -Wall -c -g $(CPPFLAGS) $< -o $@

esos_tui: %.o
	$(CC) -Wall $(LDFLAGS) $(OBJECTS) -o $@
