# Makefile for the mkpasswd
# $Id: Makefile,v 1.6 2010-11-10 13:38:39 gvs Exp $

CC=gcc
CFLAGS	= -O2 -Wall
LDFLAGS = -lcrypt

OBJS =	mkpasswd.o
SRCS =	mkpasswd.c

.c.o:
	$(CC) $(CFLAGS) -c $<

all: mkpasswd

clean:
	/bin/rm -rf *.o core *.core

mkpasswd: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# Catch any changes in compilation options at the top of this file
$(OBJS): Makefile
