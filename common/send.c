/*
 *   IRC - Internet Relay Chat, common/send.c
 *   Copyright (C) 1990 Jarkko Oikarinen and
 *		      University of Oulu, Computing Center
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
# include "s_defines.h"
#define SEND_C
# include "s_externs.h"
#undef SEND_C

#if defined(INET6) || defined(__FreeBSD__) || defined(__sun__)
#define	_NO_IP_	{{0}}
#else
#define	_NO_IP_	0
#endif

static	char	sendbuf[2048];
static	int	send_message (aClient *, char *, long);
static	void	vsendto_prefix_one(aClient *, aClient *, char *, va_list);


#ifndef CLIENT_COMPILE
#ifndef USE_OLD8BIT
/* I hope it's enough since RFC max message size is 512
   note: it may be static until ircd is multithreaded */
static	char	convbuf[3072];
#endif
static	char	psendbuf[2048];
static u_int8_t	sentalong[MAXCONNECTIONS];
#endif

#ifdef DEBUGMODE
int	writecalls = 0, writeb[10] = {0,0,0,0,0,0,0,0,0,0};
#endif

#ifdef LOG_EVENTS
static	aLogger	*log_conn = NULL;
static	aLogger	*log_user = NULL;

#ifdef USE_SYSLOG
static	aSyslog	priorities[] = {
	{ "EMERG",	LOG_EMERG },
	{ "ALERT",	LOG_ALERT },
	{ "CRIT",	LOG_CRIT },
	{ "ERR",	LOG_ERR },
	{ "WARNING",	LOG_WARNING },
	{ "NOTICE",	LOG_NOTICE },
	{ "INFO",	LOG_INFO },
	{ "DEBUG",	LOG_DEBUG },
	{ NULL,		-1 }
};
#endif
# define	TWONULL	NULL,NULL

#else
# define	TWONULL	NULL
#endif /* LOG_EVENTS */

static	SChan	svchans[SCH_MAX] = {
	{ SCH_ERROR,	"&ERRORS",	TWONULL },
	{ SCH_NOTICE,	"&NOTICES",	TWONULL },
	{ SCH_KILL,	"&KILLS",	TWONULL },
	{ SCH_CHAN,	"&CHANNEL",	TWONULL },
	{ SCH_NUM,	"&NUMERICS",	TWONULL },
	{ SCH_SERVER,	"&SERVERS",	TWONULL },
	{ SCH_HASH,	"&HASH",	TWONULL },
	{ SCH_LOCAL,	"&LOCAL",	TWONULL },
	{ SCH_SERVICE,	"&SERVICES",	TWONULL },
	{ SCH_DEBUG,	"&DEBUG",	TWONULL },
	{ SCH_AUTH,	"&AUTH",	TWONULL },
	{ SCH_OPER,	"&OPER",	TWONULL },
	{ SCH_ISERV,	"&ISERV",	TWONULL },
	{ SCH_WALLOPS,	"&WALLOPS",	TWONULL },
};

#undef TWONULL

/*
** deliver_it
**	Attempt to send a sequence of bytes to the connection.
**	Returns
**
**	< 0	Some fatal error occurred, (but not EWOULDBLOCK).
**		This return is a request to close the socket and
**		clean up the link.
**	
**	>= 0	No real error occurred, returns the number of
**		bytes actually transferred. EWOULDBLOCK and other
**		possibly similar conditions should be mapped to
**		zero return. Upper level routine will have to
**		decide what to do with those unwritten bytes...
**
**	*NOTE*	alarm calls have been preserved, so this should
**		work equally well whether blocking or non-blocking
**		mode is used...
*/
static	int	deliver_it(cptr, str, len)
aClient *cptr;
int	len;
char	*str;
    {
	int	retval;
	aClient	*acpt = cptr->acpt;
#if !defined(CLIENT_COMPILE) && defined(USE_OLD8BIT)
	char	*rusnet_buf;	/* RusNet extensions */
#endif

#ifdef	DEBUGMODE
	writecalls++;
#endif
#ifndef	NOWRITEALARM
	(void)alarm(WRITEWAITDELAY);
#endif
#if !defined(CLIENT_COMPILE) && defined(USE_OLD8BIT)
	if (cptr->transptr)
	{
		rusnet_buf = MyMalloc(len);
		memcpy(rusnet_buf, str, len);
		rusnet_translate(cptr->transptr, RUSNET_DIR_OUTGOING,
							rusnet_buf, len);
		str = rusnet_buf;
	}
#endif
	retval = SEND(cptr, str, len);

	/*
	** Convert WOULDBLOCK to a return of "0 bytes moved". This
	** should occur only if socket was non-blocking. Note, that
	** all is Ok, if the 'write' just returns '0' instead of an
	** error and errno=EWOULDBLOCK.
	**
	** ...now, would this work on VMS too? --msa
	*/
	if (retval < 0 && (errno == EWOULDBLOCK || errno == EAGAIN ||
#ifdef	EMSGSIZE
			   errno == EMSGSIZE ||
#endif
			   errno == ENOBUFS))
	    {
		retval = 0;
		cptr->flags |= FLAGS_BLOCKED;
	    }
	else if (retval > 0)
		cptr->flags &= ~FLAGS_BLOCKED;

#ifndef	NOWRITEALARM
	(void )alarm(0);
#endif
#ifdef DEBUGMODE
	if (retval < 0) {
		writeb[0]++;
		Debug((DEBUG_ERROR,"write error (%s) to %s",
			strerror(errno), cptr->name));
	} else if (retval == 0)
		writeb[1]++;
	else if (retval < 16)
		writeb[2]++;
	else if (retval < 32)
		writeb[3]++;
	else if (retval < 64)
		writeb[4]++;
	else if (retval < 128)
		writeb[5]++;
	else if (retval < 256)
		writeb[6]++;
	else if (retval < 512)
		writeb[7]++;
	else if (retval < 1024)
		writeb[8]++;
	else
		writeb[9]++;
#endif
	if (retval > 0)
	    {
#if defined(DEBUGMODE) && defined(DEBUG_WRITE)
		Debug((DEBUG_WRITE, "send = %d bytes to %d[%s]:[%*.*s]\n",
			retval, cptr->fd, cptr->name, retval, retval, str));
#endif
		cptr->sendB += retval;
		me.sendB += retval;
		if (cptr->sendB > 1023)
		    {
			cptr->sendK += (cptr->sendB >> 10);
			cptr->sendB &= 0x03ff;	/* 2^10 = 1024, 3ff = 1023 */
		    }
		if (acpt != &me)
		    {
			acpt->sendB += retval;
			if (acpt->sendB > 1023)
			    {
				acpt->sendK += (acpt->sendB >> 10);
				acpt->sendB &= 0x03ff;
			    }
		    }
		else if (me.sendB > 1023)
		    {
			me.sendK += (me.sendB >> 10);
			me.sendB &= 0x03ff;
		    }
	    }
#if !defined(CLIENT_COMPILE) && defined(USE_OLD8BIT)
	if (cptr->transptr)
		MyFree (rusnet_buf);	/* Rusnet extensions */
#endif
	return(retval);
}




/*
** dead_link
**	An error has been detected. The link *must* be closed,
**	but *cannot* call ExitClient (m_bye) from here.
**	Instead, mark it with FLAGS_DEADSOCKET. This should
**	generate ExitClient from the main loop.
**
**	If 'notice' is not NULL, it is assumed to be a format
**	for a message to local opers. It can contain only one
**	'%s', which will be replaced by the sockhost field of
**	the failing link.
**
**	Also, the notice is skipped for "uninteresting" cases,
**	like Persons and yet unknown connections...
*/
int	dead_link(to, notice)
aClient *to;
char	*notice;
{
	to->flags |= FLAGS_DEADSOCKET;
	/*
	 * If because of BUFFERPOOL problem then clean dbufs now so that
	 * notices don't hurt operators below.
	 */
	DBufClear(&to->recvQ);
	DBufClear(&to->sendQ);
#ifndef CLIENT_COMPILE
	if (!IsPerson(to) && !IsUnknown(to) && !(to->flags & FLAGS_CLOSING))
		sendto_flag(SCH_ERROR, notice, get_client_name(to, FALSE));
	Debug((DEBUG_ERROR, notice, get_client_name(to, FALSE)));
#endif
	return -1;
}

#ifndef CLIENT_COMPILE
/*
** flush_fdary
**      Used to empty all output buffers for connections in fdary.
*/
void    flush_fdary(fdp)
FdAry   *fdp;
{
        int     i;
        aClient *cptr;

        for (i = 0; i <= fdp->highest; i++)
            {
                if (!(cptr = local[fdp->fd[i]]))
                        continue;
                if (!IsRegistered(cptr)) /* is this needed?? -kalt */
                        continue;
                if (DBufLength(&cptr->sendQ) > 0)
                        (void)send_queued(cptr);
            }
}

/*
** flush_connections
**	Used to empty all output buffers for all connections. Should only
**	be called once per scan of connections. There should be a select in
**	here perhaps but that means either forcing a timeout or doing a poll.
**	When flushing, all we do is empty the obuffer array for each local
**	client and try to send it. if we can't send it, it goes into the sendQ
**	-avalon
*/
void	flush_connections(fd)
int	fd;
{
	Reg	int	i;
	Reg	aClient *cptr;

	if (fd == me.fd)
	    {
		for (i = highest_fd; i >= 0; i--)
			if ((cptr = local[i]) && DBufLength(&cptr->sendQ) > 0)
				(void)send_queued(cptr);
	    }
	else if (fd >= 0 && (cptr = local[fd]) && DBufLength(&cptr->sendQ) > 0)
		(void)send_queued(cptr);
}
#endif

/*
** send_message
**	Internal utility which delivers one message buffer to the
**	socket. Takes care of the error handling and buffering, if
**	needed.
**	if ZIP_LINKS is defined, the message will eventually be compressed,
**	anything stored in the sendQ is compressed.
*/
static	int	send_message(to, msg, len)
aClient	*to;
char	*msg;	/* if msg is a null pointer, we are flushing connection */
long	len;
#if !defined(CLIENT_COMPILE)
{
	int i;

	/* replace remote client with its server */
	if (to->from)
		to = to->from;

	Debug((DEBUG_SEND,"Sending %s %d [%s] ",
		to->name, to->fd, msg ? msg : "FLUSH"));

	if (IsDead(to))
		return 0; /* This socket has already been marked as dead */

	if (to->fd < 0)
	    {
		Debug((DEBUG_ERROR,
		       "Local socket %s with negative fd... AARGH!", to->name));
		return 0;
	    }
	if (IsMe(to))
	    {
		sendto_flag(SCH_ERROR, "Trying to send to myself! [%s]",
							msg ? msg : "FLUSH");
		return 0;
	    }
	if (DBufLength(&to->sendQ) > get_sendq(to))
	{
# ifdef HUB
		if (CBurst(to))
		{
			aConfItem	*aconf = to->serv->nline;

			poolsize -= MaxSendq(aconf->class) >> 1;
			IncSendq(aconf->class);
			poolsize += MaxSendq(aconf->class) >> 1;
			sendto_flag(SCH_NOTICE,
				    "New poolsize %d. (sendq adjusted)",
				    poolsize);
			istat.is_dbufmore++;
		}
		else
# endif
		{
			char ebuf[BUFSIZE];

			ebuf[0] = '\0';
			if (IsService(to) || IsServer(to))
			{
				sprintf(ebuf,
				"Max SendQ limit exceeded for %s: %ld > %ld",
					get_client_name(to, FALSE),
					DBufLength(&to->sendQ), get_sendq(to));
			}
			to->exitc = EXITC_SENDQ;
			return dead_link(to, ebuf[0] ? ebuf :
				"Max Sendq exceeded");
		}
	}
# ifndef USE_OLD8BIT
	if (to->conv) /* do conversion right before compression */
	{
	    char *msgs = msg;
#ifndef DO_TEXT_REPLACE
	    char *mm = &msg[len];
	    int mark;
#endif

#ifndef DO_TEXT_REPLACE
	    /* mark it if there may be unconvertable chars */
	    while (mm > msg && (*(--mm) == '\r' || *mm == '\n'));
	    if (mm > msg && *mm != ':')
		mark = 1;
	    else
		mark = 0;
#endif
	    msg = convbuf;
	    len = conv_do_out(to->conv, msgs, len, &msg,
			      sizeof(convbuf));
#ifndef DO_TEXT_REPLACE
	    /* Avoid "No text to send" warning message */
	    if (mark)
	    {
		mm = &msg[len];
		while (mm > msg && (*(--mm) == '\r' || *mm == '\n'));
		if (mm > msg && *mm == ':' && len < sizeof(convbuf)-1)
		{
		    mm++;
		    memmove(mm+1, mm, &msg[len] - mm);
		    *mm = TEXT_REPLACE_CHAR;
		    msg[++len] = '\0';
		}
	    }
#endif
	}
# endif
# ifdef	ZIP_LINKS
	/*
	** data is first stored in to->zip->outbuf until
	** it's big enough to be compressed and stored in the sendq.
	** send_queued is then responsible to never let the sendQ
	** be empty and to->zip->outbuf not empty.
	*/
	if (to->flags & FLAGS_ZIP)
		msg = zip_buffer(to, msg, &len, 0);

tryagain:
	if (len && (i = dbuf_put(&to->sendQ, msg, len)) < 0)
# else 	/* ZIP_LINKS */
tryagain:
	if ((i = dbuf_put(&to->sendQ, msg, len)) < 0)
# endif	/* ZIP_LINKS */
	{
		if (i == -2 && CBurst(to))
		    {	/* poolsize was exceeded while connect burst */
			aConfItem	*aconf = to->serv->nline;

			poolsize -= MaxSendq(aconf->class) >> 1;
			IncSendq(aconf->class);
			poolsize += MaxSendq(aconf->class) >> 1;
			sendto_flag(SCH_NOTICE,
				    "New poolsize %d. (reached)",
				    poolsize);
			istat.is_dbufmore++;
			goto tryagain;
		    }
		else
		    {
			to->exitc = EXITC_MBUF;
			return dead_link(to,
				"Buffer allocation error for %s");
		    }
	}
	/*
	** Update statistics. The following is slightly incorrect
	** because it counts messages even if queued, but bytes
	** only really sent. Queued bytes get updated in SendQueued.
	*/
	to->sendM += 1;
	me.sendM += 1;
	if (to->acpt != &me)
		to->acpt->sendM += 1;
	/*
	** This little bit is to stop the sendQ from growing too large when
	** there is no need for it to. Thus we call send_queued() every time
	** 2k has been added to the queue since the last non-fatal write.
	** Also stops us from deliberately building a large sendQ and then
	** trying to flood that link with data (possible during the net
	** relinking done by servers with a large load).
	*/
	if (DBufLength(&to->sendQ)/1024 > to->lastsq)
		send_queued(to);
	return 0;
}
#else /* CLIENT_COMPILE */
{
	int	rlen = 0, i;

	Debug((DEBUG_SEND,"Sending %s %d [%s] ", to->name, to->fd, msg));

	if (to->from)
		to = to->from;
	if (to->fd < 0)
	    {
		Debug((DEBUG_ERROR,
		      "Local socket %s with negative fd... AARGH!",
		      to->name));
	    }
	if (IsDead(to))
		return 0; /* This socket has already been marked as dead */

	if ((rlen = deliver_it(to, msg, len)) < 0 && rlen < len)
		return dead_link(to,"Write error to %s, closing link");
	/*
	** Update statistics. The following is slightly incorrect
	** because it counts messages even if queued, but bytes
	** only really sent. Queued bytes get updated in SendQueued.
	*/
	to->sendM += 1;
	me.sendM += 1;
	if (to->acpt != &me)
		to->acpt->sendM += 1;
	return 0;
}
#endif

/*
** send_queued
**	This function is called from the main select-loop (or whatever)
**	when there is a chance the some output would be possible. This
**	attempts to empty the send queue as far as possible...
*/
int	send_queued(to)
aClient *to;
{
	char	*msg;
	long	len, rlen;
	int	more = 0;

	/*
	** Once socket is marked dead, we cannot start writing to it,
	** even if the error is removed...
	*/
	if (IsDead(to))
	    {
		/*
		** Actually, we should *NEVER* get here--something is
		** not working correct if send_queued is called for a
		** dead socket... --msa
		*/
		return -1;
	    }
#ifdef	ZIP_LINKS
	/*
	** Here, we must make sure than nothing will be left in to->zip->outbuf
	** This buffer needs to be compressed and sent if all the sendQ is sent
	*/
	if ((to->flags & FLAGS_ZIP) && to->zip->outcount)
	    {
		if (DBufLength(&to->sendQ) > 0)
			more = 1;
		else
		    {
			msg = zip_buffer(to, NULL, &len, 1);
			
			if (len == -1)
			       return dead_link(to,
						"fatal error in zip_buffer()");

			if (dbuf_put(&to->sendQ, msg, len) < 0)
			    {
				to->exitc = EXITC_MBUF;
				return dead_link(to,
					 "Buffer allocation error for %s");
			    }
		    }
	    }
#endif
	while (DBufLength(&to->sendQ) > 0 || more)
	    {
		msg = dbuf_map(&to->sendQ, &len);
					/* Returns always len > 0 */
		if ((rlen = deliver_it(to, msg, len)) < 0)
			return dead_link(to,"Write error to %s, closing link");
		(void)dbuf_delete(&to->sendQ, rlen);
		to->lastsq = DBufLength(&to->sendQ)/1024;
		if (rlen < len) /* ..or should I continue until rlen==0? */
			break;

#ifdef	ZIP_LINKS
		if (DBufLength(&to->sendQ) == 0 && more)
		    {
			/*
			** The sendQ is now empty, compress what's left
			** uncompressed and try to send it too
			*/
			more = 0;
			msg = zip_buffer(to, NULL, &len, 1);

			if (len == -1)
			       return dead_link(to,
						"fatal error in zip_buffer()");

			if (dbuf_put(&to->sendQ, msg, len) < 0)
			    {
				to->exitc = EXITC_MBUF;
				return dead_link(to,
					 "Buffer allocation error for %s");
			    }
		    }
#endif
	    }

	return (IsDead(to)) ? -1 : 0;
}


#ifndef CLIENT_COMPILE
# ifdef	DBUF_TAIL
#define	DBUF_PHOLDER	{0, 0, NULL, NULL}
# else
#define	DBUF_PHOLDER	{0, 0, NULL}
#endif
static	anUser	ausr = { NULL, NULL, NULL, NULL, 0, 0, 0, 0, NULL,
			 NULL, "anonymous", "anonymous.", "anonymous."};

static	aClient	anon = { NULL, NULL, NULL, &ausr, NULL, NULL, 0, 0,/*flags*/
			 &anon, -2, 0, STAT_CLIENT, "anonymous", "anonymous",
			 "anonymous identity hider", "anonymous.in.IRC",
# ifndef USE_OLD8BIT
			 "",
# endif
			 0, "", NULL,
# ifdef	ZIP_LINKS
			 NULL,
# endif
			 0, DBUF_PHOLDER, DBUF_PHOLDER,
			 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, NULL, 0,
			 {_NO_IP_}, NULL, "" ,'\0', 0,
#ifdef USE_SSL
			 NULL, NULL,
#endif
#ifdef HOLD_ENFORCED_NICKS
			 0,
#endif
			 0, 0
			};
#endif

static size_t end_line(char *line, size_t len)
{
  register size_t newlen;

  newlen = len;
#ifdef	IRCII_KLUDGE
  len = 511;
#else
  len = 510;
#endif
  if (newlen > len)	/* may be it's already ok */
  {
#if !defined(CLIENT_COMPILE) && !defined(USE_OLD8BIT)
    if (UseUnicode)	/* let's count chars - works for utf* only!!! */
    {
      register size_t chsize = 0;
      register char *ch = line;

      while (chsize < len && *ch)		/* go for max 512 chars */
      {
	if ((*ch++ & 0xc0) == 0xc0)		/* first multibyte octet */
	  while ((*ch & 0xc0) == 0x80) ch++;	/* skip rest of octets */
	chsize++;				/* char counted */
      }
      newlen = (char *)ch - line;
    }
    else		/* 8bit encoding - just let's cut it */
#endif
      newlen = len;
  }
#ifndef	IRCII_KLUDGE
  line[newlen++] = '\r';
#endif
  line[newlen++] = '\n';
  line[newlen] = '\0';
  return newlen;
}

/*
 *
 */
static	int	vsendprep(char *pattern, va_list va)
{
	int	len;

	Debug((DEBUG_L10, "sendprep(%s)", pattern));
	len = vsprintf(sendbuf, pattern, va);
	return end_line(sendbuf, len);
}

/*
 * sendpreprep: takes care of building the string according to format & args,
 *		and of adding a complete prefix if necessary
 */
static	int	vsendpreprep(aClient *to, aClient *from, char *pattern, va_list va)
{
	int	len;

	Debug((DEBUG_L10, "sendpreprep(%#x(%s),%#x(%s),%s)",
		to, to->name, from, from->name, pattern));
	if (to && from && MyClient(to) && IsPerson(from) &&
	    !strncmp(pattern, ":%s", 3))
	{
		char	*par = va_arg(va, char *);

		if (from == &anon || !mycmp(par, from->name))
		{
			if (from->user->flags & FLAGS_VHOST)
				len = sprintf(psendbuf, ":%s!%s@%s", from->name,
					from->user->username, from->user->host);
			else
				len = sprintf(psendbuf, ":%s!%s@%s", from->name,
					from->user->username, from->sockhost);
		}
		else
			len = sprintf(psendbuf, ":%s", par);

		len += vsprintf(psendbuf+len, pattern+3, va);
	}
	else
		len = vsprintf(psendbuf, pattern, va);

	return end_line(psendbuf, len);
}

#ifdef	LOG_EVENTS
/*
 * sendto_log
 *	log		message type/syslog severity
 *	source		message source
 *	channel		message channel (e.g. &ERRORS)
 *	msg		message itself
 */
static	void	sendto_log(log, source, channel, msg)
aLogger		*log;
const char	*source, *channel, *msg;
{
    while (log)
    {
#ifdef	USE_SYSLOG
	if (log->syslog != -1)
		syslog(log->syslog, "%s%s%s%s%s", source, *source ? ": " : "",
			channel, *channel ? ": " : "", msg);
#endif
	if (log->logf != -1)
	{
		int buflen = strlen(source) + strlen(channel) + strlen(msg) + 32;
		char *buf = MyMalloc(buflen);

		int i = snprintf(buf, buflen, "%s %s%s%s%s%s\n", myctime(timeofday), 
			    source, *source ? ": " : "",
			    channel, *channel ? ": " : "", msg);
#if 0
               (void)lseek(log->logf, 0, SEEK_END); /* may be more than once */
#endif
               (void)write(log->logf, buf, i);
		MyFree(buf);
	}
	log = log->next;
    }
}
#endif	/* LOG_EVENTS */


/*
** send message to single client
*/
int	vsendto_one(aClient *to, char *pattern, va_list va)
{
	long	len;

	len = vsendprep(pattern, va);
	(void)send_message(to, sendbuf, len);
	return (int)len;
}

int	sendto_one(aClient *to, char *pattern, ...)
{
	int	len;
	va_list	va;
	va_start(va, pattern);
	len = vsendto_one(to, pattern, va);
	va_end(va);
	return len;
}

/*
 * sendto_channel_butone
 *
 * Send a message to all members of a channel that are connected to this
 * server except client 'one'.
 */
#ifndef CLIENT_COMPILE
void	sendto_channel_butone(aClient *one, aClient *from, aChannel *chptr,
						int ops, char *pattern, ...)
{
	Reg	Link	*lp;
	Reg	aClient *acptr, *lfrm = from;
	long	len1, len2 = 0;

	if (IsAnonymous(chptr) && IsClient(from))
	    {
		lfrm = &anon;
	    }

	if (one != from && MyConnect(from) && IsRegisteredUser(from))
	    {
		/* useless junk? */ /* who said that and why? --B. */
		va_list	va;
		va_start(va, pattern);
		vsendto_prefix_one(from, from, pattern, va);
		va_end(va);
	    }

	{
		va_list	va;
		va_start(va, pattern);
		len1 = vsendprep(pattern, va);
		va_end(va);
	}

	for (lp = chptr->clist; lp; lp = lp->next)
	    {
		acptr = lp->value.cptr;
		if (acptr->from == one || IsMe(acptr))
			continue;	/* ...was the one I should skip */
		/* for ops only: get user flags and check it */
		if (ops && !IsServer(acptr))
		    {
			Reg Link *tmp = find_user_link(chptr->members, acptr);

			if (!tmp)	/* this should NOT happen but... */
				continue;

			if (!(tmp->flags & CHFL_CHANOP))
				continue;
		    }
		if (MyConnect(acptr) && IsRegisteredUser(acptr))
		    {
			if (!len2)
			    {
				va_list	va;
				va_start(va, pattern);
				len2 = vsendpreprep(acptr, lfrm, pattern, va);
				va_end(va);
			    }

			if (acptr != from)
				(void)send_message(acptr, psendbuf, len2);
		    }
		else
			(void)send_message(acptr, sendbuf, len1);
	    }
	return;
}

/*
 * sendto_server_butone
 *
 * Send a message to all connected servers except the client 'one'.
 */
void	sendto_serv_butone(aClient *one, char *pattern, ...)
{
	Reg	int	i;
	Reg	long	len=0;
	Reg	aClient *cptr;

	for (i = fdas.highest; i >= 0; i--)
		if ((cptr = local[fdas.fd[i]]) &&
		    (!one || cptr != one->from) && !IsMe(cptr)) {
			if (!len)
			    {
				va_list	va;
				va_start(va, pattern);
				len = vsendprep(pattern, va);
				va_end(va);
			    }
			(void)send_message(cptr, sendbuf, len);
	}
	return;
}

int	sendto_serv_v(aClient *one, int ver, char *pattern, ...)
{
	Reg	int	i, rc=0;
	Reg	long	len=0;
	Reg	aClient *cptr;

	for (i = fdas.highest; i >= 0; i--)
		if ((cptr = local[fdas.fd[i]]) &&
		    (!one || cptr != one->from) && !IsMe(cptr))
		{
			if (cptr->serv->version & ver)
			    {
				if (!len)
				    {
					va_list	va;
					va_start(va, pattern);
					len = vsendprep(pattern, va);
					va_end(va);
				    }
				(void)send_message(cptr, sendbuf, len);
			    }
			else
				rc = 1;
		}
	return rc;
}

/*
 * sendto_common_channels()
 *
 * Sends a message to all people (inclusing user) on local server who are
 * in same channel with user, except for channels set Quiet or Anonymous
 * The calling procedure must take the necessary steps for such channels.
 */
void	sendto_common_channels(aClient *user, char *pattern, ...)
{
	Reg	int	i;
	Reg	aClient *cptr;
	Reg	Link	*channels, *lp;
	long	len = 0;

/*      This is kind of funky, but should work.  The first part below
	is optimized for HUB servers or servers with few clients on
	them.  The second part is optimized for bigger client servers
	where looping through the whole client list is bad.  I'm not
	really certain of the point at which each function equals
	out...but I do know the 2nd part will help big client servers
	fairly well... - Comstud 97/04/24
*/
     
	if (highest_fd < 50) /* This part optimized for HUB servers... */
	    {
		if (MyConnect(user))
		    {
			va_list	va;
			va_start(va, pattern);
			len = vsendpreprep(user, user, pattern, va);
			va_end(va);
			(void)send_message(user, psendbuf, len);
		    }
		for (i = 0; i <= highest_fd; i++)
		    {
			if (!(cptr = local[i]) || IsServer(cptr) ||
			    user == cptr || !user->user)
				continue;
			for (lp = user->user->channel; lp; lp = lp->next)
			    {
				aChannel *chptr = lp->value.chptr;

				if (!IsMember(cptr, chptr))
					continue;
				if (IsAnonymous(chptr))
					continue;
				if (!IsQuiet(chptr))
				    {
#ifndef DEBUGMODE
					if (!len) /* This saves little cpu,
						     but breaks the debug code.. */
#endif
					    {
					      va_list	va;
					      va_start(va, pattern);
					      len = vsendpreprep(cptr, user,
								pattern, va);
					      va_end(va);
					    }

					(void)send_message(cptr, psendbuf, len);
					break;
				    }
			    }
		    }
	    }
	else
	    {
		/* This part optimized for client servers */
		bzero((char *)&sentalong[0], sizeof(u_int8_t) * MAXCONNECTIONS);
		if (MyConnect(user))
		    {
			va_list	va;
			va_start(va, pattern);
			len = vsendpreprep(user, user, pattern, va);
			va_end(va);
			(void)send_message(user, psendbuf, len);
			sentalong[user->fd] = 1;
		    }
		if (!user->user)
			return;
		for (channels = user->user->channel; channels;
		     channels = channels->next)
		    {
			aChannel *chptr = channels->value.chptr;

			if (IsQuiet(chptr))
				continue;
			if (IsAnonymous(chptr))
				continue;
			for (lp = chptr->clist; lp; lp = lp->next)
			    {
				cptr = lp->value.cptr;
				if (user == cptr)
					continue;
				if (!cptr->user || sentalong[cptr->fd])
					continue;
#ifndef DEBUGMODE
				if (!len) /* This saves little cpu,
					     but breaks the debug code.. */
#endif
				    {
					va_list	va;
					va_start(va, pattern);
					len = vsendpreprep(cptr, user, pattern, va);
					va_end(va);
				    }

				(void)send_message(cptr, psendbuf, len);
				sentalong[cptr->fd]++;
			    }
		    }
	    }
	return;
}

/*
 * sendto_channel_butserv
 *
 * Send a message to all members of a channel that are connected to this
 * server.
 */
void	sendto_channel_butserv(aChannel *chptr, aClient *from, char *pattern, ...)
{
	Reg	Link	*lp;
	Reg	aClient	*acptr, *lfrm = from;
	long	len = 0;

	if (MyClient(from))
	    {	/* Always send to the client itself */
		va_list	va;
		va_start(va, pattern);
		vsendto_prefix_one(from, from, pattern, va);
		va_end(va);
		if (IsQuiet(chptr))	/* Really shut up.. */
			return;
	    }

	if (IsAnonymous(chptr) && IsClient(from))
	    {
		lfrm = &anon;
	    }

	for (lp = chptr->clist; lp; lp = lp->next)
		if (MyClient(acptr = lp->value.cptr) && acptr != from)
		    {
			/* only perform initialization once if needed */
			if (!len)
			    {
				va_list	va;
				va_start(va, pattern);
				len = vsendpreprep(acptr, lfrm, pattern, va);
				va_end(va);
			    }

			(void)send_message(acptr, psendbuf, len);
		    }

	return;
}

/*
** send a msg to all ppl on servers/hosts that match a specified mask
** (used for enhanced PRIVMSGs)
**
** addition -- Armin, 8jun90 (gruner@informatik.tu-muenchen.de)
*/

static	int	match_it(one, mask, what)
aClient *one;
char	*mask;
int	what;
{
	switch (what)
	{
	case MATCH_HOST:
		return (match(mask, one->sockhost) == 0);
	case MATCH_SERVER:
	default:
		return (match(mask, one->user->server)==0);
	}
}

/*
 * sendto_match_servs
 *
 * send to all servers which match the mask at the end of a channel name
 * (if there is a mask present) or to all if no mask.
 */
void	sendto_match_servs(aChannel *chptr, aClient *from, char *format, ...)
{
	Reg	int	i;
	Reg	long	len=0;
	Reg	aClient	*cptr;
	char	*mask;

	if (chptr)
	    {
		if (*chptr->chname == '&')
			return;
		if ((mask = (char *)rindex(chptr->chname, ':')))
			mask++;
	    }
	else
		mask = (char *)NULL;

	for (i = fdas.highest; i >= 0; i--)
	    {
		if (!(cptr = local[fdas.fd[i]]) || (cptr == from) ||
		    IsMe(cptr))
			continue;
		if (!BadPtr(mask) && match(mask, cptr->name))
			continue;
		if (!len)
		    {
			va_list	va;
			va_start(va, format);
			len = vsendprep(format, va);
			va_end(va);
		    }
		(void)send_message(cptr, sendbuf, len);
	    }
}

int	sendto_match_servs_v(aChannel *chptr, aClient *from, int ver,
		     char *format, ...)
{
	Reg	int	i, rc=0;
	Reg	long	len=0;
	Reg	aClient	*cptr;
	char	*mask;

	if (chptr)
	    {
		if (*chptr->chname == '&')
			return 0;
		if ((mask = (char *)rindex(chptr->chname, ':')))
			mask++;
	    }
	else
		mask = (char *)NULL;

	for (i = fdas.highest; i >= 0; i--)
	    {
		if (!(cptr = local[fdas.fd[i]]) || (cptr == from) ||
		    IsMe(cptr))
			continue;
		if (!BadPtr(mask) && match(mask, cptr->name))
			continue;
		if ((ver & cptr->serv->version) == 0)
		    {
			rc = 1;
			continue;
		    }
		if (!len)
		    {
			va_list	va;
			va_start(va, format);
			len = vsendprep(format, va);
			va_end(va);
		    }
		(void)send_message(cptr, sendbuf, len);
	    }
	return rc;
}

int	sendto_match_servs_notv(aChannel *chptr, aClient *from, int ver,
			char *format, ...)
{
	Reg	int	i, rc=0;
	Reg	long	len=0;
	Reg	aClient	*cptr;
	char	*mask;

	if (chptr)
	    {
		if (*chptr->chname == '&')
			return 0;
		if ((mask = (char *)rindex(chptr->chname, ':')))
			mask++;
	    }
	else
		mask = (char *)NULL;

	for (i = fdas.highest; i >= 0; i--)
	    {
		if (!(cptr = local[fdas.fd[i]]) || (cptr == from) ||
		    IsMe(cptr))
			continue;
		if (!BadPtr(mask) && match(mask, cptr->name))
			continue;
		if ((ver & cptr->serv->version) != 0)
		    {
			rc = 1;
			continue;
		    }
		if (!len)
		    {
			va_list	va;
			va_start(va, format);
			len = vsendprep(format, va);
			va_end(va);
		    }
		(void)send_message(cptr, sendbuf, len);
	    }
	return rc;
}

/*
 * sendto_match_butone
 *
 * Send to all clients which match the mask in a way defined on 'what';
 * either by user hostname or user servername.
 */
void	sendto_match_butone(aClient *one, aClient *from, char *mask, int what, 
		char *pattern, ...)
{
	int	i;
	aClient *cptr,
		*srch;
  
	for (i = 0; i <= highest_fd; i++)
	    {
		if (!(cptr = local[i]))
			continue;       /* that clients are not mine */
 		if (cptr == one)	/* must skip the origin !! */
			continue;
		if (IsServer(cptr))
		    {
			/*
			** we can save some CPU here by not searching the
			** entire list of users since it is ordered!
			** original idea/code from pht.
			** it could be made better by looping on the list of
			** servers to avoid non matching blocks in the list
			** (srch->from != cptr), but then again I never
			** bothered to worry or optimize this routine -kalt
			*/
			for (srch = cptr->prev; srch; srch = srch->prev)
			{
				if (!IsRegisteredUser(srch))
					continue;
				if (srch->from == cptr &&
				    match_it(srch, mask, what))
					break;
			}
			if (srch == NULL)
				continue;
		    }
		/* my client, does he match ? */
		else if (!(IsRegisteredUser(cptr) && 
			   match_it(cptr, mask, what)))
			continue;
		{
			va_list	va;
			va_start(va, pattern);
			vsendto_prefix_one(cptr, from, pattern, va);
			va_end(va);
		}

	    }
	return;
}

/*
** sendto_ops_butone
**	Send message to all operators.
** one - client not to send message to
** from- client which message is from *NEVER* NULL!!
*/
#ifndef WALLOPS_TO_CHANNEL
void	sendto_ops_butone(aClient *one, aClient *from, char *pattern, ...)
{
	Reg	int	i;
	Reg	aClient *cptr;
	bzero((char *)&sentalong[0], sizeof(u_int8_t) * MAXCONNECTIONS);
	for (cptr = client; cptr; cptr = cptr->next)
	    {
		if (IsService(cptr) || IsMe(cptr) || !IsRegistered(cptr))
			continue;
		if (IsPerson(cptr) && (!IsOper(cptr) || !SendWallops(cptr)))
			continue;
/*		if (MyClient(cptr) && !(IsServer(from) || IsMe(from)))
			continue;*/
		if (cptr->from == one)
			continue;	/* ...was the one I should skip */
		i = cptr->from->fd;	/* find connection oper is on */
		if (sentalong[i])	/* sent message along it already ? */
			continue;

		sentalong[i] = 1;
		{
			va_list	va;
			va_start(va, pattern);
			vsendto_prefix_one(cptr->from, from, pattern, va);
			va_end(va);
		}
	    }
	return;
}
#endif

/*
 * to - destination client
 * from - client which message is from
 *
 * NOTE: NEITHER OF THESE SHOULD *EVER* BE NULL!!
 * -avalon
 */
void	sendto_prefix_one(aClient *to, aClient *from, char *pattern, ...)
{
	long	len;

	va_list	va;
	va_start(va, pattern);
	len = vsendpreprep(to, from, pattern, va);
	va_end(va);
	send_message(to, psendbuf, len);
	return;
}

static void	vsendto_prefix_one(aClient *to, aClient *from, char *pattern, va_list va)
{
	long	len;

	len = vsendpreprep(to, from, pattern, va);
	send_message(to, psendbuf, len);
	return;
}


void	setup_svchans()
{
	int	i;
	SChan	*shptr;

	for (i = SCH_MAX, shptr = svchans + (i - 1); i > 0; i--, shptr--)
		shptr->svc_ptr = find_channel(shptr->svc_chname, NULL);
}

void	sendto_flag(u_int chan, char *pattern, ...)
{
	Reg	aChannel *chptr = NULL;
	SChan	*shptr;
	char	nbuf[2048];

	if (chan < 1 || chan > SCH_MAX)
		chan = SCH_NOTICE;
	shptr = svchans + (chan - 1);

	if ((chptr = shptr->svc_ptr))
	    {
		{
			va_list	va;
			va_start(va, pattern);
			(void)vsnprintf(nbuf, sizeof(nbuf), pattern, va);
			va_end(va);
		}
		sendto_channel_butserv(chptr, &me, ":%s NOTICE %s :%s", ME, chptr->chname, nbuf);

#ifdef LOG_EVENTS
		sendto_log(shptr->log, ME, chptr->chname, nbuf);
#endif

#ifdef	USE_SERVICES
		switch (chan)
		    {
		case SCH_ERROR:
			check_services_butone(SERVICE_WANT_ERRORS, NULL, &me,
					      "&ERRORS :%s", nbuf);
			break;
		case SCH_NOTICE:
			check_services_butone(SERVICE_WANT_NOTICES, NULL, &me,
					      "&NOTICES :%s", nbuf);
			break;
		case SCH_LOCAL:
			check_services_butone(SERVICE_WANT_LOCAL, NULL, &me,
					      "&LOCAL :%s", nbuf);
			break;
		case SCH_NUM:
			check_services_butone(SERVICE_WANT_NUMERICS, NULL, &me,
					      "&NUMERICS :%s", nbuf);
			break;
		    }
#endif
	    }

	return;
}

#ifdef LOG_EVENTS

void log_open(aConfItem *conf)
{
    aLogger	**log = NULL;
    aLogger	*newlog;
    int		logf;
#ifdef USE_SYSLOG
    int		prio;
#endif

    /* find the log type */
    if (!strcasecmp (conf->host, "CONN"))
	log = &log_conn;
    else if (!strcasecmp (conf->host, "USER"))
	log = &log_user;
	
    else for (logf = 0; log == NULL && logf < SCH_MAX; logf++)
       if (!strcasecmp (conf->host, &svchans[logf].svc_chname[1]))
           log = &svchans[logf].log;

    if (log == NULL) /* log type wasn't found */
	return;
    /* set up a log file */
    if (!BadPtr(conf->passwd))
    {
	char path[1024];
	size_t s;
	struct stat sb, newsb;

	strncpy(path, LOG_DIR "/", sizeof(path) - 1);
	s = strlen(path);
	strncpy(&path[s], conf->passwd, sizeof(path) - s - 1);
	path[sizeof(path) - 1] = '\0';

	if (stat(path, &newsb) == -1 && (errno != ENOENT))
		return;		/* invalid path choosen  -erra */

	/* now find if we already have an open fd for it  -erra */
	for (newlog = log_conn; newlog; newlog = newlog->next) {
		fstat(newlog->logf, &sb);
		if (sb.st_dev == newsb.st_dev && sb.st_ino == newsb.st_ino)
			break;
	}

	if (!newlog)
	    for (newlog = log_user; newlog; newlog = newlog->next) {
		fstat(newlog->logf, &sb);
		if (sb.st_dev == newsb.st_dev && sb.st_ino == newsb.st_ino)
			break;
	    }

	if (!newlog)
	  for (logf = 0; logf < SCH_MAX; logf++)
	    for (newlog = svchans[logf].log; newlog; newlog = newlog->next)
	    {
		fstat(newlog->logf, &sb);
		if (sb.st_dev == newsb.st_dev && sb.st_ino == newsb.st_ino)
			break;
	    }

	if (newlog)
	{
		logf = dup(newlog->logf);
		Debug((DEBUG_DEBUG, "fd %d = dup(fd %d) for log file \"%s\"",
					logf, newlog->logf, conf->passwd));
	}
	else
	{
		logf = open(path, O_WRONLY|O_APPEND|O_NDELAY|O_CREAT,
						S_IWUSR|S_IRUSR|S_IRGRP);
		if (logf == -1)
		{
		    Debug((DEBUG_DEBUG, "Failed to create log file \"%s\": %s",
							conf->passwd, strerror(errno)));
		}
	}
    }
    else
	logf = -1;
#ifdef USE_SYSLOG
    /* check for a syslog */
    if (conf->name && *conf->name)
    {
	for (prio = 0; priorities[prio].name; prio++)
	    if (!strcasecmp (priorities[prio].name, conf->name))
		break;
	prio = priorities[prio].level;
    }
    else
	prio = -1;
    if (logf == -1 && prio == -1)
#else
    if (logf == -1)
#endif
	return;
    newlog = (aLogger *)MyMalloc(sizeof(aLogger));
    newlog->next = (*log);
    *log = newlog;
    newlog->logf = logf;
#ifdef USE_SYSLOG
    newlog->syslog = prio;
#endif
}

static aLogger *log_recclose(aLogger *log)
{
    aLogger *next;

    while (log)
    {
	if (log->logf != -1)
	    (void)close(log->logf);
	next = log->next;
	MyFree(log);
	log = next;
    }
    return NULL;
}

void log_closeall(void)
{
    int i;

    for (i = 0; i < SCH_MAX; i++)
	svchans[i].log = log_recclose(svchans[i].log);
    log_conn = log_recclose(log_conn);
    log_user = log_recclose(log_user);
}


/*
 * sendto_flog
 *	cptr		used for firsttime, auth, exitc, send/receive M/B
 *	msg		exit code
 *	username	sometimes can't get it from cptr
 *	hostname	i.e.
 */
void	sendto_flog(aClient *cptr, char msg, char *username, char *hostname)
{

	/* 
	** One day we will rewrite linebuf to malloc()s, but for now
	** we are lazy. The longest linebuf I saw during last year
	** was 216. Max auth reply can be 1024, see rfc931_work() and
	** if iauth is disabled, read_authports() makes it max 513.
	** And the rest... just count, I got 154 --Beeth
	*/

	char	linebuf[1500];
	aLogger	*logfile;
	/*
	** This is a potential buffer overflow.
	** I mean, when you manage to keep ircd
	** running for almost 12 years ;-) --B.
	*/
#ifdef LOG_OLDFORMAT
	char	buf[12];
#endif

	/*
	** EXITC_REG == 0 means registered client quitting, so it goes to
	** userlog; otherwise it's rejection and goes to connlog --Beeth.
	*/
	logfile = (msg == EXITC_REG ? log_user : log_conn);

#if !defined(USE_SERVICES)
	if (logfile == NULL)
	{
		return;
	}
#endif
#ifdef LOG_OLDFORMAT
	if (msg == EXITC_REG)
	{
		time_t	duration;

		duration = timeofday - cptr->firsttime + 1;
		(void)sprintf(buf, "%3d:%02d:%02d",
			(int) (duration / 3600),
			(int) ((duration % 3600) / 60),
			(int) (duration % 60));
	}
	else
	{
		char *anyptr;

		switch(msg)
		{
			case EXITC_GHMAX:	anyptr="G IP  max"; break;
			case EXITC_GUHMAX:	anyptr="G u@h max"; break;
			case EXITC_LHMAX:	anyptr="L IP  max"; break;
			case EXITC_LUHMAX:	anyptr="L u@h max"; break;
			case EXITC_AREF:
			case EXITC_AREFQ:	anyptr=" Denied  "; break;
			case EXITC_KLINE:	anyptr=" K lined "; break;
			case EXITC_RLINE:	anyptr=" R lined "; break;
			case EXITC_CLONE:	anyptr=" ?Clone? "; break;
			case EXITC_YLINEMAX:	anyptr="   max   "; break;
			case EXITC_NOILINE:	anyptr=" No Auth "; break;
			case EXITC_AUTHFAIL:	anyptr="No iauth!"; break;
			case EXITC_AUTHTOUT:	anyptr="iauth t/o"; break;
			case EXITC_FAILURE:	anyptr=" Failure "; break;
			default:		anyptr=" Unknown ";
		}
		(void)sprintf(buf, "%s", anyptr);
	}
	snprintf(linebuf, sizeof(linebuf),
		"%s (%s): %s@%s [%s] %c %lu %luKb %lu %luKb ",
		myctime(cptr->firsttime), buf,
		username[0] ? username : "<none>", hostname,
		cptr->auth ? cptr->auth : "<none>",
		cptr->exitc, cptr->sendM, (long)(cptr->sendB>>10),
		cptr->receiveM, (long)(cptr->receiveB>>10));
#else
	/*
	** This is the content of loglines.
	*/
	snprintf(linebuf, sizeof(linebuf),
		"%c %d %s %s %s %s %d %lu[%.1fK] %lu[%.1fK]",
		/* exit code as defined in common/struct_def.h; some common:
		 * '0' normal exit, '-' unregistered client quit, 'k' k-lined,
		 * 'K' killed, 'X' x-lined, 'Y' max clients limit of Y-line,
		 * 'L' local @host limit, 'l' local user@host limit, 'P' ping
		 * timeout, 'Q' send queue exceeded, 'E' socket error */
		cptr->exitc,
		/* signon unix time */
		(u_int)(timeofday - cptr->firsttime)
		/* signoff unix time */,
		/* username (if ident is not working, it's from USER cmd) */
		username,
		/* hmm, let me take an educated guess... a hostname? */
		hostname,
		/* ident, if available */
		cptr->auth ? cptr->auth : "?",
		/* client IP */
#ifdef INET6
		inetntop(AF_INET6, (char *)&cptr->ip, mydummy, MYDUMMY_SIZE),
#else
		inetntoa((char *)&cptr->ip),
#endif
		/* client (remote) port */
		cptr->port,
		/* messages and bytes sent to client */
		cptr->sendM, (cptr->sendB/1024.0),
		/* messages and bytes received from client */
		cptr->receiveM, (cptr->receiveB/1024.0));
#endif /* LOG_OLDFORMAT */

#ifdef	USE_SERVICES
	if (msg == EXITC_REG)
	{
		check_services_butone(SERVICE_WANT_USERLOG, NULL, &me,
				      "USERLOG :%s", linebuf);
	}
	else
	{
		check_services_butone(SERVICE_WANT_CONNLOG, NULL, &me,
				      "CONNLOG :%s", linebuf);
	}
#endif
	sendto_log(logfile, "", "", linebuf);
}

#endif /* LOG_EVENTS */
#endif /* CLIENT_COMPILE */
