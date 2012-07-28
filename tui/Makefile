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
	$(CC) -m64 -Wall -c -g $(CPPFLAGS) $< -D_GNU_SOURCE -o $@

esos_tui: $(OBJ_FILES)
	$(CC) -m64 -Wall $(LDFLAGS) $(OBJ_FILES) -lncurses -lcdk -liniparser -o $@
