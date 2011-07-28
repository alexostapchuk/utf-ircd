/************************************************************************
 *   IRC - Internet Relay Chat, iserv/i_io.c
 *   Copyright (C) 1998 Christophe Kalt
 *   Copyright (C) 2005 skold
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *   $Id$
 */

#include "os.h"
#include "i_defines.h"
#define I_IO_C
#include "i_externs.h"
#undef I_IO_C

#define IOBUFSIZE 4096
static char		iobuf[IOBUFSIZE + 1];
static char		rbuf[IOBUFSIZE + 1];
static int		iob_len = 0, rb_len = 0;

/*
 ** We expect to receive server configuration line in format
 ** ready to use in ircd config and already parsed and checked
 ** by ircd itself. We do not parse or verify anything here
 ** assuming that ircd is never wrong about the proper syntax.
*/

/*
 * parse_ircd
 *
 * We only parse data coming from ircd and never send back
 */
static void
parse_ircd()
{
	char *ch, *buf = iobuf;
	DebugLog((DEBUG_RAW, "> parse_ircd()"));

	iobuf[iob_len] = '\0';
	
	while (ch = index(buf, '\n'))
	    {
		*ch = '\0';
		DebugLog((DEBUG_MISC, "parse_ircd(): got [%s]", buf ? buf : "NULL"));
		addPendingLine(buf);
		buf = ch + 1;			/* Leave the rest for the next io cycle */
	    }
	rb_len = 0; iob_len = 0;
	if (strlen(buf))
		bcopy(buf, rbuf, rb_len = strlen(buf)); /* Store incomplete line in rbuf */
	DebugLog((DEBUG_RAW, "< parse_ircd()"));
}

/*
 * loop_io
 *
 *	select()/poll() loop
 */
void
loop_io()
{
    /* the following is from ircd/s_bsd.c */
#if ! defined(USE_POLL)
# define SET_READ_EVENT( thisfd )       FD_SET( thisfd, &read_set)
# define TST_READ_EVENT( thisfd )       FD_ISSET( thisfd, &read_set)
        fd_set  read_set;
        int     highfd = -1;
#else
/* most of the following use pfd */
# define POLLSETREADFLAGS       (POLLIN|POLLRDNORM)
# define POLLREADFLAGS          (POLLSETREADFLAGS|POLLHUP|POLLERR)

# define SET_READ_EVENT( thisfd ){  CHECK_PFD( thisfd );\
                                   pfd->events |= POLLSETREADFLAGS;}

# define TST_READ_EVENT( thisfd )       pfd->revents & POLLREADFLAGS

# define CHECK_PFD( thisfd )                    \
        if ( pfd->fd != thisfd ) {              \
                pfd = &poll_fdarray[nbr_pfds++];\
                pfd->fd     = thisfd;           \
                pfd->events = 0;                \
					    }

        struct	pollfd   poll_fdarray[2];
        struct	pollfd * pfd	= poll_fdarray;
        int 	nbr_pfds 	= 0;
#endif

	int	i, nfds		= 0;
	struct	timeval wait;
	time_t	now		= time(NULL);

#if !defined(USE_POLL)
	FD_ZERO(&read_set);
	highfd = 0;
#else
	/* set up such that CHECK_FD works */
	nbr_pfds = 0;
	pfd      = poll_fdarray;
	pfd->fd  = -1;
#endif  /* USE_POLL */

	SET_READ_EVENT(0);
	nfds = 1;		/* ircd stream */
#if !defined(USE_POLL)
	highfd = 1;
#endif
	DebugLog((DEBUG_RAW, "> loop_io()"));
	DebugLog((DEBUG_IO, "io_loop(): checking for %d fd's", nfds));
	wait.tv_sec = 5; wait.tv_usec = 0;
#if ! defined(USE_POLL)
	nfds = select(highfd + 1, (SELECT_FDSET_TYPE *)&read_set,
		      0, 0, &wait);
	DebugLog((DEBUG_IO, "io_loop(): select() returned %d, errno = %s",
		  nfds, strerror(errno)));
#else
	nfds = poll(poll_fdarray, nbr_pfds,
		    wait.tv_sec * 1000 + wait.tv_usec/1000 );
	DebugLog((DEBUG_IO, "io_loop(): poll() returned %d, errno = %s",
		  nfds, (nfds == -1) ? strerror(errno) : "0"));
	pfd = poll_fdarray;
#endif
	if (nfds == -1)
		if (errno == EINTR)
		{
			DebugLog((DEBUG_RAW, "< loop_io()"));
			return;
		}
		else
		    {
			DebugLog((DEBUG_RAW, "Fatal select/poll error: %s",
				strerror(errno)));
			sendto_log(LOG_CRIT,
				   "Fatal select/poll error: %s",
				   strerror(errno));
			exit(1);
		    }
	if (nfds == 0)	/* end of timeout */
	{
		DebugLog((DEBUG_RAW, "< loop_io()"));
		return;
	}
	/* no matter select() or poll(), fd #0 is ircd stream */
	if (TST_READ_EVENT(0))
		nfds--;
#if defined(USE_POLL)
	pfd = poll_fdarray;
#endif
#ifdef ISERV_DEBUG
	if (fcntl(0, F_GETFL) & O_NONBLOCK)
	    DebugLog((DEBUG_IO, "ircd stream is set to NONBLOCK"));
	else
	    DebugLog((DEBUG_IO, "ircd stream is set to BLOCKING"));
#endif	

	if (TST_READ_EVENT(0))
	    {
		/* data from the ircd.. */
		while (1)
		    {
			if (rb_len)
				bcopy(rbuf, iobuf, iob_len = rb_len); /* Fill the iobuf with the incomplete data 
									received on previous iteration */
			if ((i = recv(0, iobuf + iob_len, IOBUFSIZE - iob_len, 0)) <= 0)
			    {
				DebugLog((DEBUG_IO, "loop_io(): recv() returned %d bytes, errno = %s", i, 
					((i < 0) ? strerror(errno) : "")));
				break;
			    }
			iob_len += i;
			DebugLog((DEBUG_IO,
				  "loop_io(): got %d bytes from ircd [%d]", i,
				  iob_len));
			parse_ircd();
		    }
		if (i == 0)
		    {
			sendto_log(LOG_CRIT,
				   "Daemon exiting.");
			exit(0);
		    }
	    }
	DebugLog((DEBUG_RAW, "< loop_io()"));
	    
}

/* End of story */
