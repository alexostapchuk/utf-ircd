/************************************************************************
 *   IRC - Internet Relay Chat, iserv/a_log.c
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
 *   $Id: i_log.c,v 1.6 2006/02/14 09:52:12 gvs Exp $
 */
 

#include "os.h"
#include "i_defines.h"
#define I_LOG_C
#include "i_externs.h"
#undef I_LOG_C

static FILE	*debug = NULL, *iservlog = NULL;
const char *logstrings[] =  {
			    "UNDEF",
			    "ALERT",
			    "CRITICAL",
			    "ERROR",
			    "WARNING",
			    "NOTICE",
			    "INFO",
			    "DEBUG",
			    "UNDEF", "UNDEF",
			    "IO",
			    "MISC",
			    "ALL"
			    };
void
init_filelogs()
{
#if defined(ISERV_DEBUG)
	if (debug)
		fclose(debug);
	if (DEBUGLVL)
	{
		debug = fopen(ISERVDBG_PATH, "a");
# if defined(USE_SYSLOG)
		if (!debug)
			syslog(LOG_ERR, "Failed to open \"%s\" for writing",
			       ISERVDBG_PATH);
# endif
	}
#endif /* ISERV_DEBUG */
	if (iservlog)
		fclose(iservlog);
#ifdef FNAME_ISERVLOG
	iservlog = fopen(LOG_DIR "/" FNAME_ISERVLOG, "a");
# if defined(USE_SYSLOG)
	if (!iservlog)
		syslog(LOG_NOTICE, "Failed to open \"%s\" for writing",
		       LOG_DIR "/" FNAME_ISERVLOG);
# endif
#endif
}

void
init_syslog()
{
#if defined(USE_SYSLOG)
	closelog();
	openlog("iserv", LOG_PID|LOG_NDELAY, LOG_FACILITY);
#endif
}

void	vsendto_log(int flags, char *pattern, va_list va)
{
	char	logbuf[4096];

	logbuf[0] = '>';
	vsprintf(logbuf + 1, pattern, va);

#if defined(USE_SYSLOG)
	if (flags < 10)
		syslog(flags, "%s", logbuf + 1);
#endif

	strcat(logbuf, "\n");

#if defined(ISERV_DEBUG) && defined(DEBUGLVL)
	if ((flags <= (DEBUGLVL + 9))  && (flags > LOG_DEBUG) && debug)
	    {
		fprintf(debug, "[%s] %s", logstrings[flags], logbuf + 1);
		fflush(debug);
	    }
#endif
	if (iservlog && (flags < LOG_DEBUG))
	    {
		fprintf(iservlog, "[%s][%s]: %s", myctime(time(NULL)), logstrings[flags], logbuf+1);
		fflush(iservlog);
	    }
}

void	sendto_log(int flags, char *pattern, ...)
{
        va_list va;
        va_start(va, pattern);
        vsendto_log(flags, pattern, va);
        va_end(va);
}


