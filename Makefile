# zerogue Makefile for LINUX
# Pasha Paterson <draco@zerobandwidth.net>
# Ashwin N <ashwin@despammed.com> 

SHELL = /bin/bash
CC = gcc

# Define either of KEYS_QWERTY or KEYS_DVORAK below
# Set -O0 for debug, -O2 for release
# Set -g for debug, remove for release
CFLAGS = -DKEYS_QWERTY -O0 -g
#CFLAGS = -DKEYS_QWERTY -O2

LDFLAGS = -lncurses 

# Added appraise.c in zerogue 0.4.0.
# Added floorobject.c in zerogue 0.4.2.
SOURCES = appraise.c floorobject.c hit.c init.c instruct.c inventory.c \
          level.c machdep.c main.c message.c monster.c move.c object.c \
          pack.c play.c random.c ring.c room.c save.c score.c special_hit.c \
          throw.c trap.c use.c zap.c
OBJECTS = $(SOURCES:.c=.o)
DFILES = $(SOURCES:.c=.d)

rogue: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o rogue

include $(DFILES)

%.d: %.c
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
    sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

.PHONY: clean
clean:
	rm -f $(OBJECTS) rogue
