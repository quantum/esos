CC		?= gcc
RM		?= rm
CPPFLAGS	:= $(CPPFLAGS)
CFLAGS		:= $(CFLAGS)
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
	$(CC) -m64 -std=gnu99 -Wall -Wextra -pedantic -c -g -O2 $(CPPFLAGS) $(CFLAGS) -D_GNU_SOURCE -o $@ $<

esos_tui: $(OBJ_FILES)
	$(CC) -m64 -std=gnu99 -Wall -Wextra -pedantic $(LDFLAGS) $(OBJ_FILES) -lncurses -ltinfo -lcdk \
	-liniparser -lparted -lblkid -luuid -lcurl -lanl -o $@

