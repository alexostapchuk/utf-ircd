/************************************************************************
 *   IRC - Internet Relay Chat, ircd/ircd_signal.c
 *   Copyright (C) 1990 Jarkko Oikarinen and
 *                      University of Oulu, Computing Center
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
 */

#include "os.h"
#include "s_defines.h"
#define IRCD_SIGNAL_C
#include "s_externs.h"
#undef IRCD_SIGNAL_C


RETSIGTYPE s_die(int s _UNUSED_)
{
#ifdef	USE_SYSLOG
	(void)syslog(LOG_CRIT, "Server Killed By SIGTERM");
	(void)closelog();
#endif
#ifdef LOG_EVENTS
	log_closeall();
#endif
	ircd_writetune(tunefile);
	flush_connections(me.fd);
	exit(-1);
}

RETSIGTYPE s_rehash(int s _UNUSED_)
{
#if POSIX_SIGNALS
	struct	sigaction act;

	act.sa_handler = s_rehash;
	act.sa_flags = 0;
	(void)sigemptyset(&act.sa_mask);
	(void)sigaddset(&act.sa_mask, SIGHUP);
	(void)sigaction(SIGHUP, &act, NULL);
#else
	(void)signal(SIGHUP, s_rehash);	/* sysV -argv */
#endif
	dorehash = 1;
}

RETSIGTYPE s_restart(int s _UNUSED_)
{
/*
 * TODO: logging here
 */
	dorestart = 1;
}

RETSIGTYPE dummy(int s _UNUSED_)
{
#ifndef HAVE_RELIABLE_SIGNALS
	(void)signal(SIGALRM, dummy);
	(void)signal(SIGPIPE, dummy);
# ifndef HPUX	/* Only 9k/800 series require this, but don't know how to.. */
#  ifdef SIGWINCH
	(void)signal(SIGWINCH, dummy);
#  endif
# endif
#else
# if POSIX_SIGNALS
	struct  sigaction       act;

	act.sa_handler = dummy;
	act.sa_flags = 0;
	(void)sigemptyset(&act.sa_mask);
	(void)sigaddset(&act.sa_mask, SIGALRM);
	(void)sigaddset(&act.sa_mask, SIGPIPE);
#  ifdef SIGWINCH
	(void)sigaddset(&act.sa_mask, SIGWINCH);
#  endif
	(void)sigaction(SIGALRM, &act, (struct sigaction *)NULL);
	(void)sigaction(SIGPIPE, &act, (struct sigaction *)NULL);
#  ifdef SIGWINCH
	(void)sigaction(SIGWINCH, &act, (struct sigaction *)NULL);
#  endif
# endif
#endif
}

void	restore_sigchld()
{
#if POSIX_SIGNALS
	struct	sigaction act;
	act.sa_handler = SIG_DFL;
	act.sa_flags = 0;
	(void)sigemptyset(&act.sa_mask);
	(void)sigaddset(&act.sa_mask, SIGCHLD);
	(void)sigaction(SIGCHLD, &act, NULL);
#else	/* POSIX_SIGNALS */
	(void)signal(SIGCHLD, SIG_DFL);
#endif	/* POSIX_SIGNALS */
}

void	setup_signals()
{
#if POSIX_SIGNALS
	struct	sigaction act;

	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;
	(void)sigemptyset(&act.sa_mask);
	(void)sigaddset(&act.sa_mask, SIGPIPE);
	(void)sigaddset(&act.sa_mask, SIGALRM);
# ifdef	SIGWINCH
	(void)sigaddset(&act.sa_mask, SIGWINCH);
	(void)sigaction(SIGWINCH, &act, NULL);
# endif
	(void)sigaction(SIGPIPE, &act, NULL);
	act.sa_handler = dummy;
	(void)sigaction(SIGALRM, &act, NULL);
	act.sa_handler = s_rehash;
	(void)sigemptyset(&act.sa_mask);
	(void)sigaddset(&act.sa_mask, SIGHUP);
	(void)sigaction(SIGHUP, &act, NULL);
	act.sa_handler = s_restart;
	(void)sigaddset(&act.sa_mask, SIGINT);
	(void)sigaction(SIGINT, &act, NULL);
	act.sa_handler = s_die;
	(void)sigaddset(&act.sa_mask, SIGTERM);
	(void)sigaction(SIGTERM, &act, NULL);
# if defined(USE_IAUTH) /* Obsolete -skold */
	act.sa_handler = SIG_IGN;
#  ifdef SA_NOCLDWAIT
        act.sa_flags = SA_NOCLDWAIT;
#  else
        act.sa_flags = 0;
#  endif
	(void)sigaddset(&act.sa_mask, SIGCHLD);
	(void)sigaction(SIGCHLD, &act, NULL);
# endif

#else /* POSIX_SIGNALS */

# ifndef	HAVE_RELIABLE_SIGNALS
	(void)signal(SIGPIPE, dummy);
#  ifdef	SIGWINCH
	(void)signal(SIGWINCH, dummy);
#  endif
# else /* HAVE_RELIABLE_SIGNALS */
#  ifdef	SIGWINCH
	(void)signal(SIGWINCH, SIG_IGN);
#  endif
	(void)signal(SIGPIPE, SIG_IGN);
# endif /* HAVE_RELIABLE_SIGNALS */
	(void)signal(SIGALRM, dummy);   
	(void)signal(SIGHUP, s_rehash);
	(void)signal(SIGTERM, s_die); 
	(void)signal(SIGINT, s_restart);
# if defined(USE_IAUTH)
	(void)signal(SIGCHLD, SIG_IGN);
# endif
#endif /* POSIX_SIGNAL */

#ifdef RESTARTING_SYSTEMCALLS
	/*
	** At least on Apollo sr10.1 it seems continuing system calls
	** after signal is the default. The following 'siginterrupt'
	** should change that default to interrupting calls.
	*/
	(void)siginterrupt(SIGALRM, 1);
#endif
}
