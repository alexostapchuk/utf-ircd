/************************************************************************
 *   IRC - Internet Relay Chat, ircd/s_user.c (formerly ircd/s_msg.c)
 *   Copyright (C) 1990 Jarkko Oikarinen and
 *		      University of Oulu, Computing Center
 *
 *   See file AUTHORS in IRC package for additional names of
 *   the programmers. 
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
#define S_USER_C
#include "s_externs.h"
#undef S_USER_C

#ifndef USE_OLD8BIT
static char buf[MB_LEN_MAX*BUFSIZE], buf2[MB_LEN_MAX*BUFSIZE];
#else
static char buf[BUFSIZE], buf2[BUFSIZE];
#endif

static int user_modes[]	     = { FLAGS_OPER, 'o',
				 FLAGS_LOCOP, 'O',
				 FLAGS_INVISIBLE, 'i',
				 FLAGS_WALLOP, 'w',
				 FLAGS_RESTRICTED, 'r',
				 FLAGS_AWAY, 'a',
				 FLAGS_VHOST, 'x',
#ifdef USE_SSL
				 FLAGS_SMODE, 's',
#endif
#ifdef RUSNET_RLINES
				 FLAGS_RMODE, 'b',
#endif
				/*
				 * we need these two to be last ones
				 * so that we can exclude them for
				 * older servers  --erra
				 */
				 FLAGS_REGISTERED, 'R',
				 FLAGS_IDENTIFIED, 'I',
				 0, 0 };

#define	ISTAT_BALANCE(sptr)	do {	\
	if (IsInvisible(sptr))		\
		istat.is_user[1]++;	\
	else				\
	istat.is_user[0]++;		\
	istat.is_unknown--;		\
	istat.is_myclnt++;		\
} while (0)

/*
** m_functions execute protocol messages on this server:
**
**	cptr	is always NON-NULL, pointing to a *LOCAL* client
**		structure (with an open socket connected!). This
**		identifies the physical socket where the message
**		originated (or which caused the m_function to be
**		executed--some m_functions may call others...).
**
**	sptr	is the source of the message, defined by the
**		prefix part of the message if present. If not
**		or prefix not found, then sptr==cptr.
**
**		(!IsServer(cptr)) => (cptr == sptr), because
**		prefixes are taken *only* from servers...
**
**		(IsServer(cptr))
**			(sptr == cptr) => the message didn't
**			have the prefix.
**
**			(sptr != cptr && IsServer(sptr) means
**			the prefix specified servername. (?)
**
**			(sptr != cptr && !IsServer(sptr) means
**			that message originated from a remote
**			user (not local).
**
**		combining
**
**		(!IsServer(sptr)) means that, sptr can safely
**		taken as defining the target structure of the
**		message in this server.
**
**	*Always* true (if 'parse' and others are working correct):
**
**	1)	sptr->from == cptr  (note: cptr->from == cptr)
**
**	2)	MyConnect(sptr) <=> sptr == cptr (e.g. sptr
**		*cannot* be a local connection, unless it's
**		actually cptr!). [MyConnect(x) should probably
**		be defined as (x == x->from) --msa ]
**
**	parc	number of variable parameter strings (if zero,
**		parv is allowed to be NULL)
**
**	parv	a NULL terminated list of parameter pointers,
**
**			parv[0], sender (prefix string), if not present
**				this points to an empty string.
**			parv[1]...parv[parc-1]
**				pointers to additional parameters
**			parv[parc] == NULL, *always*
**
**		note:	it is guaranteed that parv[0]..parv[parc-1] are all
**			non-NULL pointers.
*/

/*
** next_client
**	Local function to find the next matching client. The search
**	can be continued from the specified client entry. Normal
**	usage loop is:
**
**	for (x = client; x = next_client(x,mask); x = x->next)
**		HandleMatchingClient;
**	      
*/
aClient *next_client(next, ch)
Reg	aClient *next;	/* First client to check */
Reg	char	*ch;	/* search string (may include wilds) */
{
	Reg	aClient	*tmp = next;

	next = find_client(ch, tmp);
	if (tmp && tmp->prev == next)
		return NULL;
	if (next != tmp)
		return next;
	for ( ; next; next = next->next)
		if (!match(ch,next->name) || !match(next->name,ch))
			break;
	return next;
}

/*
** hunt_server
**
**	Do the basic thing in delivering the message (command)
**	across the relays to the specific server (server) for
**	actions.
**
**	Note:	The command is a format string and *MUST* be
**		of prefixed style (e.g. ":%s COMMAND %s ...").
**		Command can have only max 8 parameters.
**
**	server	parv[server] is the parameter identifying the
**		target server.
**
**	*WARNING*
**		parv[server] is replaced with the pointer to the
**		real servername from the matched client (I'm lazy
**		now --msa).
**
**	returns: (see #defines)
*/
int	hunt_server(cptr, sptr, command, server, parc, parv)
aClient	*cptr, *sptr;
char	*command, *parv[];
int	server, parc;
    {
	aClient *acptr;

	/*
	** Assume it's me, if no server
	*/
	if (parc <= server || BadPtr(parv[server]) ||
	    match(ME, parv[server]) == 0 ||
	    match(parv[server], ME) == 0)
		return (HUNTED_ISME);
	/*
	** These are to pickup matches that would cause the following
	** message to go in the wrong direction while doing quick fast
	** non-matching lookups.
	*/
	if ((acptr = find_client(parv[server], NULL)))
		if (acptr->from == sptr->from && !MyConnect(acptr))
			acptr = NULL;
	/* Match *.masked.servers */
	if (!acptr && (acptr = find_server(parv[server], NULL)))
		if (acptr->from == sptr->from && !MyConnect(acptr))
			acptr = NULL;
	/* Remote services@servers */
	if (!acptr && (acptr = find_service(parv[server], NULL)))
		if (acptr->from == sptr->from && !MyConnect(acptr))
			acptr = NULL;
	if (!acptr)
		for (acptr = client, (void)collapse(parv[server]);
		     (acptr = next_client(acptr, parv[server]));
		     acptr = acptr->next)
		    {
			if (acptr->from == sptr->from && !MyConnect(acptr))
				continue;
			/*
			 * Fix to prevent looping in case the parameter for
			 * some reason happens to match someone from the from
			 * link --jto
			 */
			if (IsRegistered(acptr) && (acptr != cptr))
				break;
		    }
	 if (acptr)
	    {
		if (!IsRegistered(acptr))
			return HUNTED_ISME;
		if (IsMe(acptr) || MyClient(acptr) || MyService(acptr))
			return HUNTED_ISME;
		if (match(acptr->name, parv[server]))
			parv[server] = acptr->name;
		if (IsService(sptr)
		    && (IsServer(acptr->from)
			&& match(sptr->service->dist,acptr->name) != 0))
		    {
			sendto_one(sptr, err_str(ERR_NOSUCHSERVER, parv[0]), 
				   parv[server]);
			return(HUNTED_NOSUCH);
		    }
		sendto_one(acptr, command, parv[0],
			   parv[1], parv[2], parv[3], parv[4],
			   parv[5], parv[6], parv[7], parv[8]);
		return(HUNTED_PASS);
	    } 
	sendto_one(sptr, err_str(ERR_NOSUCHSERVER, parv[0]), parv[server]);
	return(HUNTED_NOSUCH);
    }

/*
** 'do_nick_name' ensures that the given parameter (nick) is
** really a proper string for a nickname (note, the 'nick'
** may be modified in the process...)
**
**	RETURNS the length of the final NICKNAME (0, if
**	nickname is illegal)
**
**  Nickname characters are in range
**	'A'..'}', '_', '-', '0'..'9'
**  anything outside the above set will terminate nickname.
**  In addition, the first character cannot be '-'
**  or a Digit.
**  Finally forbid the use of "anonymous" because of possible
**  abuses related to anonymous channnels. -kalt
**
**  Note:
**	'~'-character should be allowed, but
**	a change should be global, some confusion would
**	result if only few servers allowed it...
*/

int	do_nick_name(nick, server)
unsigned char	*nick;
int		server;
{
	Reg	unsigned char	*ch;

	if (*nick == '-') /* first character '-' */
		return 0;

	if (isdigit(*nick) && !server) /* first character in [0..9] */
		return 0;

	if (!strncasecmp(nick, "anonymous", 9)) /* Do or do we not need anynimous? -skold*/
		return 0;

#ifndef USE_OLD8BIT
#ifdef LOCALE_STRICT_NAMES
	if (Force8bit || MB_CUR_MAX > 1)
#else
	if (Force8bit)
#endif
	{
	    /*
	    ** Unfortunately, locales support is very limited for now and we
	    ** cannot check it in native charset so we just do some hack here
	    **   note: conversion is always in use for CHARSET_8BIT
	    */
	    conversion_t *conv = conv_get_conversion(CHARSET_8BIT);
	    unsigned char nick8[NICKLEN+1];
	    register size_t sz;
	    unsigned char *ch2;

	    ch2 = nick8;
	    /* convert at most NICKLEN chars */
	    sz = conv_do_out(conv, nick, strlen(nick), &ch2, NICKLEN);
	    ch = ch2; /* converted nick ptr */
	    for ( ; sz; ch++, sz--) /* check it in forced charset */
		if (!isvalid(*ch))
		    break;
	    sz = ch - ch2; /* valid size */
	    if (sz && ch2 != nick) /* if no conversion descriptor then ch2==nick */
		sz = conv_do_in(conv, ch2, sz, &nick, strlen(nick));
	    ch = nick + sz;
	    conv_free_conversion(conv);
	}
	else
#if defined(IRCD_CHANGE_LOCALE) && !defined(LOCALE_STRICT_NAMES)
	if (UseUnicode && MB_CUR_MAX > 1) /* multibyte encoding */
	{
	    size_t sz = strlen(nick);
	    int wcs;
	    wchar_t wc;
	    int len = NICKLEN;

	    for (ch = nick; *ch && len; len--)
	    {
		wcs = mbtowc(&wc, ch, sz); /* extract unicode char */
		if (wcs <= 0 || !isvalid(wc)) /* stop at invalid char */
		    break;
		sz -= wcs;
		ch += wcs;
	    }
	}
	else
#endif
#endif
	for (ch = nick; *ch && (ch - nick) < NICKLEN; ch++)
		if (!isvalid(*ch))
			break;
#ifndef NO_DIRECT_VHOST
	if (!server && *ch == '!')	/* for +x users at startup  -skold */
		ch++;
#endif
	*ch = '\0';
	Debug((DEBUG_DEBUG, "do_nick_name(): corrected to \"%s\"", nick));

	return (ch - nick);
}

/*
** canonize
**
** reduce a string of duplicate list entries to contain only the unique
** items.  Unavoidably O(n^2).
*/
char	*canonize(buffer)
char	*buffer;
{
	static	char	cbuf[BUFSIZ];
	Reg	char	*s, *t, *cp = cbuf;
	Reg	int	l = 0;
	char	*p = NULL, *p2;

	*cp = '\0';

	for (s = strtoken(&p, buffer, ","); s; s = strtoken(&p, NULL, ","))
	    {
		if (l)
		    {
			for (p2 = NULL, t = strtoken(&p2, cbuf, ","); t;
			     t = strtoken(&p2, NULL, ","))
				if (!strcasecmp(s, t))
					break;
				else if (p2)
					p2[-1] = ',';
		    }
		else
			t = NULL;
		if (!t)
		    {
			if (l)
				*(cp-1) = ',';
			else
				l = 1;
			(void)strcpy(cp, s);
			if (p)
				cp += (p - s);
		    }
		else if (p2)
			p2[-1] = ',';
	    }
	return cbuf;
}

/*
** ereject_user
**	extracted from register_user for clarity
**	early rejection of a user connection, with logging.
*/
int
ereject_user(cptr, shortm, longm)
aClient *cptr;
char shortm, *longm;
{
#ifdef LOG_EVENTS
	sendto_flog(cptr, shortm, "<none>",
#ifdef UNIXPORT
		    (IsUnixSocket(cptr)) ? me.sockhost :
#endif
		    ((cptr->hostp) ? cptr->hostp->h_name : cptr->sockhost));
#endif
	return exit_client(cptr, cptr, &me, longm);
}

/*
** register_user
**	This function is called when both NICK and USER messages
**	have been accepted for the client, in whatever order. Only
**	after this the USER message is propagated.
**
**	NICK's must be propagated at once when received, although
**	it would be better to delay them too until full info is
**	available. Doing it is not so simple though, would have
**	to implement the following:
**
**	1) user telnets in and gives only "NICK foobar" and waits
**	2) another user far away logs in normally with the nick
**	   "foobar" (quite legal, as this server didn't propagate
**	   it).
**	3) now this server gets nick "foobar" from outside, but
**	   has already the same defined locally. Current server
**	   would just issue "KILL foobar" to clean out dups. But,
**	   this is not fair. It should actually request another
**	   nick from local user or kill him/her...
*/

int	register_user(cptr, sptr, nick, username)
aClient	*cptr;
aClient	*sptr;
char	*nick, *username;
{
	Reg	aConfItem *aconf;
	aClient	*acptr;
	aServer	*sp = NULL;
	anUser	*user = sptr->user;
	short	oldstatus = sptr->status;
	char	*parv[3], *s;
#ifndef NO_PREFIX
	char	prefix;
#endif
	int	i, pos = 0;

	user->last = timeofday;
	parv[0] = sptr->name;
	parv[1] = parv[2] = NULL;

	if (MyConnect(sptr))
	    {
		char *reason = NULL;

#if defined(USE_IAUTH)
		static time_t last = 0;
		static u_int count = 0;

		if (iauth_options & XOPT_EARLYPARSE && DoingXAuth(cptr))
		    {
			cptr->flags |= FLAGS_WXAUTH;
			/* fool check_pings() and give iauth more time! */
			cptr->firsttime = timeofday;
			cptr->lasttime = timeofday;
			strncpyzt(sptr->user->username, username, USERLEN+1);
			if (sptr->passwd[0])
				sendto_iauth("%d P %s", sptr->fd,sptr->passwd);
			sendto_iauth("%d U %s", sptr->fd, username);
			return 1;
		    }
		if (!DoneXAuth(sptr) && (iauth_options & XOPT_REQUIRED))
		    {
			char *reason;

			if (iauth_options & XOPT_NOTIMEOUT)
			    {
				count += 1;
				if (timeofday - last > 300)
				    {
					sendto_flag(SCH_AUTH, 
	    "iauth may not be running! (refusing new user connections)");
					last = timeofday;
				    }
				reason = "No iauth!";
				sptr->exitc = EXITC_AUTHFAIL;
			    }
			else {
				reason = "iauth t/o";
				sptr->exitc = EXITC_AUTHTOUT;
			}
			return ereject_user(cptr, sptr->exitc,
					    "Authentication failure!");
		    }
		if (timeofday - last > 300 && count)
		    {
			sendto_flag(SCH_AUTH, "%d users rejected.", count);
			count = 0;
		    }

		/* this should not be needed, but there's a bug.. -kalt */
		/* haven't seen any notice like this, ever.. no bug no more? */
		if (*cptr->username == '\0')
		    {
			sendto_flag(SCH_AUTH,
				    "Ouch! Null username for %s (%d %X)",
				    get_client_name(cptr, TRUE), cptr->fd,
				    cptr->flags);
			sendto_iauth("%d E Null username [%s] %X", cptr->fd,
				     get_client_name(cptr, TRUE), cptr->flags);
			return exit_client(cptr, sptr, &me,
					   "Fatal Bug - Try Again");
		    }
#endif

		/*
		** the following insanity used to be after check_client()
		** but check_client()->attach_Iline() now needs to know the
		** username for global u@h limits.
		** moving this shit here shouldn't be a problem. -krys
		** what a piece of $#@!.. restricted can only be known
		** *after* attach_Iline(), so it matters and I have to move
		** come of it back below.  so global u@h limits really suck.
		*/
#ifndef	NO_PREFIX
		/*
		** ident is fun.. ahem
		** prefixes used:
		** 	none	I line with ident
		**	^	I line with OTHER type ident
		**	~	I line, no ident
		** 	+	i line with ident
		**	=	i line with OTHER type ident
		**	-	i line, no ident
		**	%	R line (RusNet), any type of ident
		*/
		if (!(sptr->flags & FLAGS_GOTID))
			prefix = '~';
		else
			if (*sptr->username == '-' ||
			    index(sptr->username, '@'))
				prefix = '^';
			else
				prefix = '\0';

		/* OTHER type idents have '-' prefix (from s_auth.c),       */
		/* and they are not supposed to be used as userid (rfc1413) */
		/* @ isn't valid in usernames (m_user()) */
		if (sptr->flags & FLAGS_GOTID && *sptr->username != '-' &&
		    index(sptr->username, '@') == NULL)
			strncpyzt(buf2, sptr->username, USERLEN+1);
		else /* No ident, or unusable ident string */
		     /* because username may point to user->username */
			strncpyzt(buf2, username, USERLEN+1);

		if (prefix)
		    {
			pos = 1;
			*user->username = prefix;
			strncpy(&user->username[1], buf2, USERLEN);
		    }
		else
			strncpy(user->username, buf2, USERLEN+1);
		user->username[USERLEN] = '\0';
		/* eos */
#else
		strncpyzt(user->username, username, USERLEN+1);
#endif
		/* username cannot have control chars or wildcards so
		   truncate them.
		   note: parv[1] cannot be NULL after checks above  --LoSt */
		for (s = user->username + pos; *s; s++)
			if (*s < 0x2b || *s == 0x3f || *s == 0x7f)
			    {
				*s = 0;
				break;
			    }
		if (user->username[pos] == '\0')
		    {	/* control char in front  -erra */
			sendto_flag(SCH_AUTH,
				    "Ouch! Bad username for %s (%d %X)",
				    get_client_name(cptr, TRUE), cptr->fd,
				    cptr->flags);
			sendto_iauth("%d E Bad username [%s] %X", cptr->fd,
				     get_client_name(cptr, TRUE), cptr->flags);
			return exit_client(cptr, sptr, &me,
			   "Control chars and wildcards in username denied");
		    }

		if (sptr->exitc == EXITC_AREF || sptr->exitc == EXITC_AREFQ)
		    {
			if (sptr->exitc == EXITC_AREF)
				sendto_flag(SCH_LOCAL,
					    "Denied connection from %s (%s).",
					    get_client_host(sptr), sptr->name);

			return ereject_user(sptr, sptr->exitc,
						"Denied access");
		    }
		if ((i = check_client(sptr)))
		    {
			struct msg_set { char shortm; char *longm; };
			    
			static struct msg_set exit_msg[7] = {
			{ EXITC_GUHMAX, "Too many user connections (global)" },
			{ EXITC_GHMAX, "Too many host connections (global)" },
			{ EXITC_LUHMAX, "Too many user connections (local)" },
			{ EXITC_LHMAX, "Too many host connections (local)" },
			{ EXITC_YLINEMAX, "Too many connections" },
			{ EXITC_NOILINE, "Unauthorized connection" },
			{ EXITC_FAILURE, "Connect failure" } };

			i += 7;
			if (i < 0 || i > 6) /* in case.. */
				i = 6;

			ircstp->is_ref++;
			sptr->exitc = exit_msg[i].shortm;
			sendto_flag(SCH_LOCAL, "%s from %s.",
				    exit_msg[i].longm, get_client_host(sptr));
			return ereject_user(cptr, sptr->exitc,
					    exit_msg[i].longm);
		    }

#ifndef	NO_PREFIX
		if (IsRestricted(sptr))
		    {
			if (!(sptr->flags & FLAGS_GOTID))
				prefix = '-';
			else
				if (*sptr->username == '-' ||
				    index(sptr->username, '@'))
					prefix = '=';
				else
					prefix = '+';
			*user->username = prefix;
			strncpy(&user->username[1], buf2, USERLEN);
			user->username[USERLEN] = '\0';
		    }
#endif

		aconf = sptr->confs->value.aconf;
#ifdef UNIXPORT
		if (IsUnixSocket(sptr))
		{
			strncpyzt(user->host, me.sockhost, HOSTLEN+1);
		}
		else
#endif
		{		
			strncpyzt(user->host, sptr->sockhost, HOSTLEN+1);
		}
		if (!BadPtr(aconf->passwd) &&
		    !StrEq(sptr->passwd, aconf->passwd))
		    {
			ircstp->is_ref++;
			sendto_one(sptr, err_str(ERR_PASSWDMISMATCH, parv[0]));
			return exit_client(cptr, sptr, &me, "Bad Password");
		    }
		bzero(sptr->passwd, sizeof(sptr->passwd));
		/*
		 * following block for the benefit of time-dependent K:-lines
		 */
		if (check_tlines(sptr, 1, &reason, nick, &kconf, NULL))
		    {
			sendto_flag(SCH_LOCAL, "K-lined %s@%s.",
				    user->username, sptr->sockhost);
			ircstp->is_ref++;
			sptr->exitc = EXITC_KLINE;
#ifdef LOG_EVENTS
			sendto_flog(sptr, sptr->exitc, user->username,
				    user->host);
#endif
			if (reason)
				sprintf(buf, "K-lined: %.256s", reason);
			return exit_client(cptr, sptr, &me, (reason) ? buf :
					   "K-lined");
		    }
#ifdef RUSNET_RLINES
		if (check_tlines(sptr, 1, &reason, nick, &rconf, NULL))
		{
		    if (reason)
			    sprintf(buf, "R-lined: %.256s", reason);

		    sendto_flag(SCH_LOCAL, "R-lined %s@%s (%s)",
				user->username, sptr->sockhost,
				(reason) ? buf : "R-lined");
			SetRMode(sptr);
			/* No need to call do_restrict here
			 * since we are all set -skold */
#ifndef NO_PREFIX
		    *user->username = '%';
		    strncpy(&user->username[1], buf2, USERLEN);
		    user->username[USERLEN] = '\0';
#endif
#ifdef LOG_EVENTS
			sendto_flog(sptr, EXITC_RLINE, user->username,
				    user->host);
#endif
			Debug((DEBUG_DEBUG, "R-line active for user %s!%s@%s",
			     sptr->name, sptr->user->username, sptr->sockhost));
		}
#endif

#if defined(EXTRA_NOTICES) && defined(CLIENT_NOTICES)
                sendto_flag(SCH_OPER, "Client connecting: %s (%s@%s) %s%s",
                        nick,
			user->username,
			user->host
# ifdef USE_SSL			
			, IsSSL(sptr) ? "[SSL]" : ""
# else
			, ""
# endif
# ifdef RUSNET_RLINES
			, IsRMode(sptr) ? "[R]" : ""
# else
			, ""
# endif			
			);
#endif

		if (oldstatus == STAT_MASTER && MyConnect(sptr))
			(void)m_oper(&me, sptr, 1, parv);
/*		*user->tok = '1';
		user->tok[1] = '\0';*/
		sp = user->servp;
#ifndef NO_DIRECT_VHOST
		if (sptr->flags & FLAGS_XMODE)
		{
			sptr->flags &= ~FLAGS_XMODE;
			SetVHost(sptr);
		}
#endif
	    }
	else
	    {
		strncpyzt(user->username, username, USERLEN+1);
		strncpyzt(sptr->sockhost, user->host, HOSTLEN+1);
	    }

	SetClient(sptr);
	if (!MyConnect(sptr))
/* && IsServer(cptr)) -- obsolete, old 2.8 protocol;
   someone needs to clean all this 2.8 stuff --Beeth */
	    {
		acptr = find_server(user->server, NULL);
		if (acptr && acptr->from != cptr)
		    {
			sendto_one(cptr, ":%s KILL %s :%s (%s != %s[%s])",
				   ME, sptr->name, ME, user->server,
				   acptr->from->name, acptr->from->sockhost);
			sptr->flags |= FLAGS_KILLED;
			return exit_client(cptr, sptr, &me,
					   "USER server wrong direction");
		    }
	    }
	if (!IsRusnetServices(sptr))
	{	/*
		** false data for usermode +x. After really long discussion
		** it's been concluded to false last three octets of IP
		** address and first two parts of hostname, to provide
		** the reasonable compromise between security
		** and channels ban lists. Host.domain.tld is mapped to
		** crc32(host.domain.tld).crc32(domain.tld).domain.tld
		** and domain.tld to crc32(domain.tld).crc32(tld).domain.tld
		** respectively --erra
		**
		** some modification there: eggdrop masking compatibility
		** with the same hide availability
		** Let's say crcsum() is crc32 of complete hostname. Then:
		** a12.b34.sub.host.domain.tld -> crcsum().crc32(sub.host.domain.tld).host.domain.tld
		** a12-b34.sub.host.domain.tld -> crcsum().crc32(sub.host.domain.tld).host.domain.tld
		** comp.sub.host.domain.tld -> crcsum().crc32(sub.host.domain.tld).host.domain.tld
		** a12.b34.host.domain.tld -> crcsum().crc32(b34.host.domain.tld).host.domain.tld
		** a12-b34.host.domain.tld -> crcsum().crc32(crcsum()).host.domain.tld
		** sub.host.domain.tld -> crcsum().crc32(crcsum()).host.domain.tld
		** a12.b34.domain.tld -> crcsum().crc32(b34.domain.tld).domain.tld
		** a12-b34.domain.tld -> crcsum().crc32(crcsum()).domain.tld
		** host.domain.tld -> crcsum().crc32(crcsum()).domain.tld
		** a12.dom2usr.tld -> crcsum().crc32(crcsum()).dom2usr.tld
		** domain.tld -> crcsum().crc32(crcsum()).domain.tld
		** domain. -> crcsum().crc32(domain.crcsum()).domain
		**/
		char *s = sptr->sockhost;
		char *c;
		char *b = s + strlen(s);
		int n = 0;

		for (c = s; c < b; c++)
		{
			if (*c == ':' && (++n) == 2) break;
		}

		if (c < b)	/* IPv6 address received */
		{
			int offset = strncmp(s, "2002", 4) ? c - s : c - s - 2;
			char *ptr = user->host + offset;

			*ptr++ = ':';	/* fake IPv6 separator */
			*ptr++ = '\0';	/* cut the rest of string */

			strcat(user->host, b64enc(gen_crc(ptr)));
			strcat(user->host, ":");
			strcat(user->host, b64enc(gen_crc(sptr->sockhost)));
		}
		else	/* distinguish hostname from IPv4 address */
		{
			for (c--, n = 0; c > s; c--)
			{
				if (*c == '.' && (++n) == 3)	/* 4th dot reached */
					break;

				else if (*c <= '9' && *c >= '0')
					break;
			}

			if (n)	/* hostname second level or above... duh */
			{
				int len;

				 /* ignore digits in second-level domain part
				    when there are no minus signs  --erra
				  */
				if (c > s && n == 1)
					while (c > s && *c != '.') {
						if (*c == '-') {
							do c++;
							while (*c != '.');

							break;
						}
						c--;
					}
				else	/* *c cannot reach \0 - see above */
					while (*c != '.')
						c++;
				s = c;

				while (s > sptr->sockhost && *(--s) != '.');

				if (*s == '.')	/* s is part for second crc32 */
					s++;

				if (s == sptr->sockhost) /* it needs crc32(crcsum()) */
					s = user->host;

				/* finished gathering data, let's rock */
				strcpy(user->host, b64enc(gen_crc(sptr->sockhost)));
				strcat(user->host, ".");
				strcat(user->host, b64enc(gen_crc(s)));
				len = strlen(user->host);
				n = len + strlen(c) - HOSTLEN;

				if (n > 0)	/* overrun protection */
					user->host[len - n] = '\0';

				strcat(user->host, c);
			}
			else if (c != s) /* are there hosts w/o dots? Yes */
			{
					/* IP address.. reverse search */
				char *pfx;

				if (!(s = strrchr(user->host, '.')))
					goto kill_badhost;
				*s = '\0';
				DupString(pfx, b64enc(gen_crc(user->host)));
			 	/* keep 1st octet */
				s = strchr(user->host, '.');
				if (!s) {
					MyFree(pfx);
						kill_badhost:
					ircstp->is_kill++;
					sendto_one(cptr, ":%s KILL %s :%s "
							"(Bad hostmask)",
							ME, sptr->name, ME);
					sptr->flags |= FLAGS_KILLED;
					/* to save balance */
					ISTAT_BALANCE(sptr);
					return exit_client(NULL, sptr, &me,
								"Bad hostmask");
				}
				strcpy(++s, b64enc(gen_crc(sptr->sockhost)));
				strcat(user->host, ".");
				strcat(user->host, pfx);
				strcat(user->host, ".in-addr");
				MyFree(pfx);
			}
		}
	}
	send_umode(NULL, sptr, 0, SEND_UMODES, buf);

	/* Find my leaf servers and feed the new client to them */
	for (i = fdas.highest; i >= 0; i--)
	    {
		char *tok;

		if ((acptr = local[fdas.fd[i]]) == cptr || IsMe(acptr))
			continue;

		tok = ((aconf = acptr->serv->nline) &&
			!match(my_name_for_link(ME, aconf->port),
			user->server)) ? me.serv->tok : user->servp->tok;

		sendto_one(acptr, "NICK %s %d %s %s %s %s :%s",
				nick, sptr->hopcount+1, user->username,
				sptr->sockhost, tok,
				(*buf) ? buf : "+", sptr->info);
	    }	/* for(my-leaf-servers) */

	if (IsInvisible(sptr))		/* Can be initialized in m_user() */
		istat.is_user[1]++;	/* Local and server defaults +i */
	else
		istat.is_user[0]++;
#ifdef EXTRA_STATISTICS
	/*
	**  Keep track of maximum number of global users.
	*/
	if ((istat.is_user[1] + istat.is_user[0]) > istat.is_m_users)
	{
		istat.is_m_users = istat.is_user[1] + istat.is_user[0];
		if ((istat.is_m_users % 1000) == 0)
			sendto_flag(SCH_NOTICE,
				"New highest global client connection:  %d",
							istat.is_m_users);
	}
#endif
	if (MyConnect(sptr))
	    {
		istat.is_unknown--;
		istat.is_myclnt++;
#ifdef  EXTRA_STATISTICS
		/*
		**  Inform of highest client connection.
		*/
		if (istat.is_myclnt > istat.is_m_myclnt)
		{
			istat.is_m_myclnt = istat.is_myclnt;
			if ((istat.is_m_myclnt % 10) == 0)
				sendto_flag(SCH_NOTICE, "New highest local "
						"client connection:  %d",
							istat.is_m_myclnt);
		}
		/*
		**  Small cludge to try and warn of some fast clonebots.
		*/
		if ((istat.is_myclnt % 10) == 0) {
			if (istat.is_myclnt > istat.is_last_cnt) {
				if (istat.is_last_cnt_t == 0)
					istat.is_last_cnt_t = me.since;

				sendto_flag(SCH_NOTICE, "Local increase from "
					"%d to %d clients in %d second%s",
				(istat.is_myclnt - 10),istat.is_myclnt,
				(timeofday - istat.is_last_cnt_t),
				((timeofday - istat.is_last_cnt_t == 1) ? ""
				: "s"));
			}
			istat.is_last_cnt_t = timeofday;
			istat.is_last_cnt = istat.is_myclnt;
		}
#endif
		sprintf(buf, "%s!%s@%s", nick, user->username,
			(user->flags & FLAGS_VHOST) ?
						user->host : sptr->sockhost);
		sptr->exitc = EXITC_REG;
		sendto_one(sptr, rpl_str(RPL_WELCOME, nick), buf);
		/* This is a duplicate of the NOTICE but see below...*/
#ifdef RUSNET_RLINES
		if (!IsRMode(sptr)) {
#endif /* RUSNET_RLINES */
		sendto_one(sptr, rpl_str(RPL_YOURHOST, nick),
			   get_client_name(&me, FALSE), version);
		sendto_one(sptr, rpl_str(RPL_CREATED, nick), creation);
		sendto_one(sptr, rpl_str(RPL_MYINFO, parv[0]),
			   ME, version);
#ifdef SEND_ISUPPORT
		sendto_one(sptr, rpl_str(RPL_ISUPPORT, parv[0]), isupport);
#endif
		(void)m_lusers(sptr, sptr, 1, parv);
#ifdef RUSNET_RLINES
		}
#endif /* RUSNET_RLINES */
		(void)m_motd(sptr, sptr, 1, parv);
		if (IsRestricted(sptr))
			sendto_one(sptr, err_str(ERR_RESTRICTED, nick));
		send_umode(sptr, sptr, 0, ALL_UMODES, buf);
#ifndef USE_OLD8BIT
		if (sptr->conv)
			sendto_one(sptr, rpl_str(RPL_CODEPAGE, nick),
						conv_charset(sptr->conv));
#else
		if (sptr->transptr && sptr->transptr->id)
			sendto_one(sptr, rpl_str(RPL_CODEPAGE, nick),
							sptr->transptr->id);
#endif
		nextping = timeofday;
	    }
#ifdef	USE_SERVICES
	if (MyConnect(sptr))	/* all modes about local users */
		send_umode(NULL, sptr, 0, ALL_UMODES, buf);
	check_services_num(sptr, buf);
#endif
	return 1;
    }

/*
** m_nick
**	parv[0] = sender prefix
**	parv[1] = nickname
**	parv[2] = internal flag
** the following are only used between servers since version 2.9
**	parv[2] = hopcount
**	parv[3] = username (login name, account)
**	parv[4] = client host name
**	parv[5] = server token
**	parv[6] = users mode
**	parv[7] = users real name info
*/
int	m_nick(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	aClient *acptr;
	int	delayed = 0, l;
	char	nick[UNINICKLEN+2], *s, *user, *host, *reason;
	Link	*lp = NULL;

	if (IsService(sptr))
   	    {
		sendto_one(sptr, err_str(ERR_ALREADYREGISTRED, parv[0]));
		return 1;
	    }

	if (parc < 2)
	    {
		sendto_one(sptr, err_str(ERR_NONICKNAMEGIVEN, parv[0]));
		return 1;
	    }
	if (MyConnect(sptr) && (s = (char *)index(parv[1], '~')))
		*s = '\0';
	strncpyzt(nick, parv[1], UNINICKLEN+1);

	if (parc == 8 && cptr->serv)
	    {
		user = parv[3];
		host = parv[4];
	    }
	else
	    {
		if (sptr->user)
		    {
			user = sptr->user->username;
			host = sptr->sockhost;
		    }
		else
			user = host = "";
	    }
	/*
	 * if do_nick_name() returns a null name OR if the server sent a nick
	 * name and do_nick_name() changed it in some way (due to rules of nick
	 * creation) then reject it. If from a server and we reject it,
	 * and KILL it. -avalon 4/4/92
	 */
	l = do_nick_name(nick, IsServer(cptr) || IsMe(cptr));
#ifndef NO_DIRECT_VHOST
	if (l && nick[l - 1] == '!')		/* wants usermode +x  -skold */
	{
		l--;
		nick[l] = '\0';

		if (!IsServer(sptr) && !sptr->name[0])
			sptr->flags |= FLAGS_XMODE;
	}
#endif
	if (l == 0 || (IsServer(cptr) && strcmp(nick, parv[1])))
	    {
		sendto_one(sptr, err_str(ERR_ERRONEOUSNICKNAME, parv[0]),
			   parv[1]);

		if (IsServer(cptr))
		    {
			ircstp->is_kill++;
			sendto_flag(SCH_KILL, "Bad Nick: %s From: %s %s",
				   parv[1], parv[0],
				   get_client_name(cptr, FALSE));
			sendto_one(cptr, ":%s KILL %s :%s (%s <- %s[%s])",
				   ME, parv[1], ME, parv[1],
				   nick, cptr->name);
			if (sptr != cptr) /* bad nick change */
			    {
				sendto_serv_butone(cptr,
					":%s KILL %s :%s (%s <- %s!%s@%s)",
					ME, parv[0], ME,
					get_client_name(cptr, FALSE),
					parv[0], user, host);
				sptr->flags |= FLAGS_KILLED;
				return exit_client(cptr,sptr,&me,"BadNick");
			    }
		    }
		return 2;
	    }

	if (MyClient(cptr))
	{
		if ((parc == 3) && parv[2] && (*parv[2] == '1')) {
		/* Internal call, do not apply sanity checks and restrictions */
		}
		else {
			Reg aChannel *chptr;
#ifdef RUSNET_RLINES

			if (IsRMode(sptr))
			{
				sendto_one(sptr,
				err_str(ERR_RESTRICTED, nick));
				return 2;
			}
#endif
			chptr = rusnet_zmodecheck(cptr, nick);
			if (chptr != NULL)
			{
				sendto_one(sptr, err_str(ERR_7BIT, parv[0]),
								chptr->chname);
				return 2; /* NICK message ignored */
			}
		}

		if (check_tlines(sptr, 1, &reason, nick, &kconf, NULL))
		{
			sendto_flag(SCH_NOTICE,
				"Kill line active for %s",
				get_client_name(cptr, FALSE));
			sptr->exitc = EXITC_KLINE;
#ifdef LOG_EVENTS
			sendto_flog(sptr, sptr->exitc, user, host);
#endif
			if (!BadPtr(reason))
				sprintf(buf, "Kill line active: %.256s", reason);
			(void)exit_client(cptr, sptr, &me, (reason) ? buf :
					   "Kill line active");
			return 20;	/* KILLed NICK entered */
		}
#ifdef RUSNET_RLINES
		if (check_tlines(sptr, 1, &reason, nick, &rconf, NULL))
		{
			int old = (cptr->user->flags & ALL_UMODES);
			do_restrict(cptr);
			send_umode_out(cptr, cptr, old);			
			Debug((DEBUG_DEBUG, "Active R-Line for user %s!%s@%s", cptr->name,
				                        cptr->user->username, cptr->sockhost));
			sendto_flag(SCH_NOTICE,
					"R line active for %s",
					get_client_name(cptr, FALSE));
			
		}
#endif
	}

	/*
	** Check against nick name collisions.
	**
	** Put this 'if' here so that the nesting goes nicely on the screen :)
	** We check against server name list before determining if the nickname
	** is present in the nicklist (due to the way the below for loop is
	** constructed). -avalon
	*/
	if ((acptr = find_server(nick, NULL)))
		if (MyConnect(sptr))
		    {
			sendto_one(sptr, err_str(ERR_NICKNAMEINUSE, parv[0]),
				   nick);
			return 2; /* NICK message ignored */
		    }
	/*
	** acptr already has result from previous find_server()
	*/
	if (acptr)
	    {
		/*
		** We have a nickname trying to use the same name as
		** a server. Send out a nick collision KILL to remove
		** the nickname. As long as only a KILL is sent out,
		** there is no danger of the server being disconnected.
		** Ultimate way to jupiter a nick ? >;-). -avalon
		*/
		sendto_flag(SCH_KILL,
			    "Nick collision on %s (%s@%s)%s <- (%s@%s)%s",
			    sptr->name,
			    (acptr->user) ? acptr->user->username : "???",
			    acptr->sockhost, acptr->from->name, user, host,
			    get_client_name(cptr, FALSE));
		ircstp->is_kill++;
		sendto_one(cptr, ":%s KILL %s :%s (%s <- %s)",
			   ME, sptr->name, ME, acptr->from->name,
			   /* NOTE: Cannot use get_client_name
			   ** twice here, it returns static
			   ** string pointer--the other info
			   ** would be lost
			   */
			   get_client_name(cptr, FALSE));
		sptr->flags |= FLAGS_KILLED;
		return exit_client(cptr, sptr, &me, "Nick/Server collision");
	    }
	if (!(acptr = find_client(nick, NULL)))
	    {
#ifdef LOCKRECENTCHANGE
		aClient	*acptr2;
#endif
		if (IsServer(cptr) || !(bootopt & BOOT_PROT))
			goto nickkilldone;
#ifdef LOCKRECENTCHANGE
		if ((acptr2 = get_history(nick, (long)(KILLCHASETIMELIMIT))) &&
		    !MyConnect(acptr2))
			/*
			** Lock nick for KCTL so one cannot nick collide
			** (due to kill chase) people who recently changed
			** their nicks. --Beeth
			*/
			delayed = 1;
		else
#endif
			/*
			** Let ND work
			*/
			delayed = find_history(nick, (long)(DELAYCHASETIMELIMIT));
		if (!delayed)
			goto nickkilldone;  /* No collisions, all clear... */
	    }
	/*
	** If acptr == sptr, then we have a client doing a nick
	** change between *equivalent* nicknames as far as server
	** is concerned (user is changing the case of his/her
	** nickname or somesuch)
	*/
	if (acptr == sptr)
		if (strcmp(acptr->name, nick) != 0)
			/*
			** Allows change of case in his/her nick
			*/
			goto nickkilldone; /* -- go and process change */
		else
                        /* 
			** This is collision renaming coming from
			** acptr owner direction (not the owner itself). Just delete this collision
			** and forget it.
			**/
			if (acptr->flags & FLAGS_COLLMAP)
			{
				acptr->flags ^= FLAGS_COLLMAP;
				if (!MyConnect(acptr) && acptr->from->serv)
					del_from_collision_map(acptr->name, acptr->from->serv->crc);
				else
					sendto_flag(SCH_ERROR, "Found local %s in collision map",
						acptr->name);
				return 2;
			} else
			/*
			** This is just ':old NICK old' type thing.
			** Just forget the whole thing here. There is
			** no point forwarding it to anywhere,
			** especially since servers prior to this
			** version would treat it as nick collision.
			*/
			return 2; /* NICK Message ignored */
	/*
	** Note: From this point forward it can be assumed that
	** acptr != sptr (point to different client structures).
	*/
	/*
	** If the older one is "non-person", the new entry is just
	** allowed to overwrite it. Just silently drop non-person,
	** and proceed with the nick. This should take care of the
	** "dormant nick" way of generating collisions...
	*/
	if (acptr && IsUnknown(acptr) && MyConnect(acptr))
	    {
		(void) exit_client(acptr, acptr, &me, "Overridden");
		goto nickkilldone;
	    }
	/*
	** Decide, we really have a nick collision and deal with it
	*/
	if (!IsServer(cptr))
	    {
		/*
		** NICK is coming from local client connection. Just
		** send error reply and ignore the command.
		*/
		sendto_one(sptr, err_str((delayed) ? ERR_UNAVAILRESOURCE
						   : ERR_NICKNAMEINUSE,
					 parv[0]), nick);
		return 2; /* NICK message ignored */
	    }
	/*
	** NICK was coming from a server connection. Means that the same
	** nick is registered for different user by different server.
	** This is either a race condition (two users coming online about
	** same time, or net reconnecting) or just two net fragments becoming
	** joined and having same nicks in use. We cannot have TWO users with
	** same nick--purge this NICK from the system with a KILL... >;)
	*/
	else
	    {
		    /*
		    ** Recursive collision resolving is dangerous. We do not allow
		    ** someone to collide interim nick change.
		    ** Just kill that guy.
		    **
		    ** Assuming that :NICK new is only possible for sptr here.
		    ** But who knows.. We need some workaround for :old NICK :new thing anyway
		    */
		    if (acptr->flags & FLAGS_COLLMAP && sptr == cptr) {
			sendto_flag(SCH_HASH,
			    "Dropping fake collision %s (%s@%s) brought by %s",
			    acptr->name, user, host, sptr->from->name);
			sendto_flag(SCH_KILL,
			    "Fake collision on %s (%s@%s)%s",
			    acptr->name, user, host, sptr->from->name);
			ircstp->is_kill++;
			/*
			** We should send KILL back only, because this would kill 
			** collided nick otherwise
			*/
			sendto_one(cptr,
			    ":%s KILL %s :%s (Fake collision)", ME, acptr->name, ME);
			sptr->flags |= FLAGS_KILLED;
			(void) exit_client(cptr, sptr, &me, "Fake collision");
			return -3;
		    }
		    /*
		    ** Only :old NICK :new for sptr does make sense here (definitely)
		    */
		    if (sptr->flags & FLAGS_COLLMAP) {
			sendto_flag(SCH_HASH,
			    "Collision nick change for %s (%s@%s)%s has collided with %s (%s@%s)%s",
			    sptr->name, user, host, get_client_name(cptr, FALSE),
			    acptr->name, (acptr->user) ? acptr->user->username : "unknown",
			    acptr->sockhost, acptr->from->name);
			sendto_flag(SCH_KILL,
			    "Recursive collision on %s (%s@%s)%s",
			    acptr->name,
			    (acptr->user) ? acptr->user->username : "unknown",
			    acptr->sockhost, acptr->from->name);
			sendto_one(acptr, err_str(ERR_NICKCOLLISION, acptr->name),
			    acptr->name, user, host);
			ircstp->is_kill++;
			sendto_serv_butone(NULL,
			    ":%s KILL %s :%s (Recursive collision)", ME, acptr->name, ME);
			acptr->flags |= FLAGS_KILLED;
			(void)exit_client(NULL, acptr, &me, "Recursive collision");
			goto nickkilldone;
		    }

		/* Force nick change  -erra
		** There are two options we have: either to change to
		** nick + base64(crc32) the same as all interim changes we 
		** already made or to nick + 5 random digits.
		** The latter is in force now.
		** They are compatible to each other and can work in the same net together.
		*/
		if (MyConnect(acptr))	
		{
			int i;
			char *ch;
			char *chasenick = MyMalloc(UNINICKLEN + 1);
			char *newnick = MyMalloc(UNINICKLEN + 1);
			char *pparv[] = { acptr->name, newnick, "1", NULL };
			
			*chasenick = '1';
			strncpy(chasenick + 1, acptr->name, UNINICKLEN - 6);
			chasenick[UNINICKLEN - 5] = '\0';
			strcat(chasenick, b64enc(me.serv->crc));

			strncpy(newnick, acptr->name, UNINICKLEN);

			/*
			** This is an ugly hack. We have to mark collided nick
			** off in history as being mapped not actually hashing
			** and renaming our client because we expect kill chase. -skold
			*/
			sendto_flag(SCH_HASH, "Adding history for %s", chasenick);
			strncpyzt(acptr->name, chasenick, UNINICKLEN + 1);
			add_history(acptr, acptr);
			strncpyzt(acptr->name, newnick, UNINICKLEN + 1);
			MyFree(chasenick);
			
			/* Reserve 5-digits space */
			newnick[UNINICKLEN - 4] = '\0';
			ch = newnick + strlen(newnick);

		/* cut last five digits only if there are exactly five */
		    for (i = 0, ch--; i < 5 && isdigit(*ch); i++, ch--);
		    if (i >= 5) 
			ch[1] = '\0';
		    sprintf (newnick + strlen(newnick), "%d",
			10000 + (int) (60000.0 * rand() / (RAND_MAX + 10000.0)));
		    
			m_nick(acptr, acptr, 3, pparv);
			MyFree(newnick);
		}
		else
			if (!IsRusnetServices(acptr))
		{
		/*
		** Here goes interim nick change to nick + base64(crc32) based
		** on the server name owning this nick.
		** For one-time collision renaming we need to comment out the true part above
		** and uncomment what is commented out below.
		*/
			char *newnick = MyMalloc(UNINICKLEN + 1);
			char *pparv[] = { acptr->name, newnick, "1", NULL };
		
		    *newnick = '1';
		    /* guard against too long nick */
		    strncpy(newnick + 1, acptr->name, UNINICKLEN - 6);
		    newnick[UNINICKLEN - 5] = '\0';
		    strcat(newnick, b64enc(acptr->user->servp->crc));
		    /*
		    ** Before rising collision we need to check if this
		    ** collision can cycle. If it cycles we should send kill
		    ** for parv[1] to everyone except cptr instead of collision
		    ** resolving and let the current nick change complete -skold
		    **/
		    if (cptr != sptr) /* We need this workaround for NICK 2 form only */
		    {
			    aClient *bcptr;
			    if ((bcptr = find_client(newnick, (aClient *)NULL)) &&
									bcptr == sptr)
			    {
				sendto_flag(SCH_HASH, "Collision cycles on nick "
				    "%s(%s@%s)%s when renaming to %s(%s@%s)%s",
				    sptr->name, user, host,
				    get_client_name(cptr, FALSE), acptr->name,
				    (acptr->user) ?
					acptr->user->username : "unknown",
				    acptr->sockhost, acptr->from->name);
				sendto_flag(SCH_KILL, "Collision deadlock on "
				    "%s -> %s(%s@%s)%s -> %s", sptr->name,
				    acptr->name, (acptr->user) ?
					acptr->user->username : "unknown",
				    acptr->sockhost, acptr->from->name, bcptr->name);
				    ircstp->is_kill++;
				    sendto_serv_butone(cptr, ":%s KILL %s :%s "
					"(Collision deadlock)", ME, acptr->name, ME);
				    acptr->flags |= FLAGS_KILLED;
				    (void)exit_client(NULL, acptr, &me,
							"Collision deadlock");
				MyFree(newnick);
				goto nickkilldone;
			    }
			}
		    
			add_to_collision_map(acptr->name, newnick,
						acptr->from->serv->crc);
			acptr->flags |= FLAGS_COLLMAP;
			m_nick(cptr, acptr, 3, pparv);
			MyFree(newnick);
		}
		/* Services part */
			else
		{
			char *newnick = MyMalloc(UNINICKLEN + 1);
			/*
			** We should send kill to cptr for mapped nick because acptr->name
			** is already owned by someone else (e.g. NickServ ;) -slcio
			*/
			*newnick = '1';
			strncpy(newnick + 1, acptr->name, UNINICKLEN - 6);
			newnick[UNINICKLEN - 5] = '\0';
			if (cptr != sptr) {
				strcat(newnick, b64enc(sptr->user->servp->crc));
			} else {
				aServer	*asptr = NULL;
				if (!(asptr = find_tokserver(atoi(parv[5]), cptr, NULL))) {
					sendto_flag(SCH_ERROR,
                        		    "ERROR: USER:%s without SERVER:%s from %s",
					    parv[0], parv[5], get_client_name(cptr, FALSE));
					ircstp->is_nosrv++;
					MyFree(newnick);
					return exit_client(NULL, sptr, &me, "No Such Server");
				} else {
					strcat(newnick, b64enc(asptr->crc));
					sendto_flag(SCH_HASH, "Sending KILL for service collision victim: %s on %s", 
					    newnick, find_server_string(asptr->snum));
				}
			}
			sendto_flag(SCH_KILL,
			    "Services collision %s (%s@%s)%s",
			    newnick, user, sptr->sockhost, sptr->from->name);
			ircstp->is_kill++;
			sendto_one(cptr,
			    ":%s KILL %s :%s (Services collision)", ME, newnick, ME);
			MyFree(newnick);

			if (cptr != sptr) {
				sendto_serv_butone(NULL,
				    ":%s KILL %s :%s (Services collision)", ME, sptr->name, ME);
				sptr->flags |= FLAGS_KILLED;
				return exit_client(cptr, sptr, &me, "Services collision");		    
			} else {
				return -3;
			}
		}
		/* End of services part */
			/* propagate to common channels and servers
			 * excluding source server direction for this nick (cptr)
			 */
	    }

nickkilldone:
	if (IsServer(sptr))
	    {
		char	*pv[7];

		if (parc != 8)
		    {
			sendto_flag(SCH_NOTICE,
			    "Bad NICK param count (%d) for %s from %s via %s",
				    parc, parv[1], sptr->name,
				    get_client_name(cptr, FALSE));
			sendto_one(cptr, ":%s KILL %s :%s (Bad NICK %d)",
				   ME, nick, ME, parc);
			return 0;
		    }
		/* A server introducing a new client, change source */
		sptr = make_client(cptr);
		add_client_to_list(sptr);
		if (parc > 2)
			sptr->hopcount = atoi(parv[2]);
		(void)strcpy(sptr->name, nick);

		pv[0] = sptr->name;
		pv[1] = parv[3];
		pv[2] = parv[4];
		pv[3] = parv[5];
		pv[4] = parv[7];
		pv[5] = parv[6];
		pv[6] = NULL;
		(void)add_to_client_hash_table(nick, sptr);
		return m_user(cptr, sptr, 6, pv);
	    }
	else if (sptr->name[0])		/* NICK received before, changing */
	    {
		if (MyConnect(sptr))
		{
			if (!IsPerson(sptr))    /* Unregistered client */
				return 2;       /* Ignore new NICKs */
			if ((parc == 3) && parv[2] && (*parv[2] == '1')) {
				/* nick was enforced, hold it for a minute */
				sptr->held = time(NULL) + 60;
			}
			else if (IsRestricted(sptr))
			    {
				sendto_one(sptr, err_str(ERR_RESTRICTED,
								sptr->name));
				return 2;
			    }
			else if (sptr->held > time(NULL))
			    {
				sendto_one(sptr, err_str(ERR_NICKTOOFAST,
								sptr->name));
				return 2;
			    }
			/* Can the user speak on all channels? */
			for (lp = sptr->user->channel; lp; lp = lp->next)
				if (can_send(sptr, lp->value.chptr) &&
				    !IsQuiet(lp->value.chptr))
					break;
		}
		/*
		** Client just changing his/her nick. If he/she is
		** on a channel, send note of change to all clients
		** on that channel. Propagate notice to other servers.
		*/
		/*
		** :old NICK :new does not make sense when nick is collided -skold
		*/
		sendto_common_channels(sptr, ":%s NICK :%s", sptr->name, nick);

#if defined(EXTRA_NOTICES) && defined(NCHANGE_NOTICES)
                if (MyConnect(sptr) && IsRegisteredUser(sptr))
                        sendto_flag(SCH_OPER,
					"Nick change: From %s to %s (%s@%s)",
					parv[0], nick, sptr->user->username,
						   sptr->sockhost);
#endif
                                                
		if (sptr->user) /* should always be true.. */
		    {
			add_history(sptr, sptr);
#ifdef	USE_SERVICES
			check_services_butone(SERVICE_WANT_NICK,
					      sptr->user->server, sptr,
					      ":%s NICK :%s", parv[0], nick);
#endif
		    }
		else
			sendto_flag(SCH_NOTICE,
				    "Illegal NICK change: %s -> %s from %s",
				    parv[0], nick, get_client_name(cptr,TRUE));

		if (sptr->flags & FLAGS_COLLMAP) /* dealing with collision */
		{
			if (cptr == sptr->from)
			{
				sptr->flags ^= FLAGS_COLLMAP;
				if (!MyConnect(sptr) && sptr->from->serv)
					del_from_collision_map(sptr->name, sptr->from->serv->crc);
				else
					sendto_flag(SCH_ERROR, "Found local %s in collision map",
						sptr->name);
			}
			else
				cptr = sptr->from;
		}

		sendto_serv_butone(cptr, ":%s NICK :%s", sptr->name, nick);

		if (sptr->name[0])
			(void)del_from_client_hash_table(sptr->name, sptr);
		(void)strcpy(sptr->name, nick);
	    }
	else
	    {
		/* Client setting NICK the first time */

		/* This had to be copied here to avoid problems.. */
		(void)strcpy(sptr->name, nick);
		if (sptr->user)
			/*
			** USER already received, now we have NICK.
			** *NOTE* For servers "NICK" *must* precede the
			** user message (giving USER before NICK is possible
			** only for local client connection!). register_user
			** may reject the client and call exit_client for it
			** --must test this and exit m_nick too!!!
			*/
			if (register_user(cptr, sptr, nick,
					  sptr->user->username)
			    == FLUSH_BUFFER)
				return FLUSH_BUFFER;
	    }
	/*
	**  Finally set new nick name.
	*/
	(void)add_to_client_hash_table(nick, sptr);
	if (lp)
		return 15;
	else
		return 3;
}

/*
** m_message (used in m_private() and m_notice())
** the general function to deliver MSG's between users/channels
**
**	parv[0] = sender prefix
**	parv[1] = receiver list
**	parv[2] = message text
**
** massive cleanup
** rev argv 6/91
**
*/

static	int	m_message(cptr, sptr, parc, parv, notice)
aClient *cptr, *sptr;
char	*parv[];
int	parc, notice;
{
	Reg	aClient	*acptr;
	Reg	char	*s;
	aChannel *chptr;
	char	*nick, *server, *p, *cmd, *user, *host;
	int	count = 0, penalty = 0, flag = 0;

	cmd = notice ? MSG_NOTICE : MSG_PRIVATE;

	if (parc < 2 || *parv[1] == '\0')
	    {
		sendto_one(sptr, err_str(ERR_NORECIPIENT, parv[0]), cmd);
		return 1;
	    }

	if (parc < 3 || *parv[2] == '\0')
	    {
		sendto_one(sptr, err_str(ERR_NOTEXTTOSEND, parv[0]));
		return 1;
	    }

	if (MyConnect(sptr))
		parv[1] = canonize(parv[1]);
	for (p = NULL, nick = strtoken(&p, parv[1], ","); nick;
	     nick = strtoken(&p, NULL, ","), penalty++)
	    {
		/*
		** restrict destination list to MAXPENALTY/2 recipients to
		** solve SPAM problem --Yegg 
		*/ 
		if (2*penalty >= MAXPENALTY) {
		    if (!notice)
			    sendto_one(sptr, err_str(ERR_TOOMANYTARGETS,
						     parv[0]),
				       "Too many",nick,"No Message Delivered");
		    continue;      
		}   
#ifdef RUSNET_RLINES
		if (MyClient(sptr) && IsRMode(sptr) && penalty >= 1) {
		    continue;      		
		}
#endif
		/*
		** nickname addressed?
		*/
		if ((acptr = find_person(nick, NULL)))
		    {
#ifdef RUSNET_RLINES
			if (MyClient(sptr) && IsRMode(sptr)) {
				if (IsOper(acptr) || IsRusnetServices(acptr)) {
					sendto_prefix_one(acptr, sptr, ":%s %s %s :%s",
					    parv[0], cmd, nick, parv[2]);
					penalty += 10;
					continue;
				} else {
					if (!notice) 
						sendto_one(sptr, err_str(ERR_RESTRICTED, parv[0]));
					continue;
		    		}
			}
#endif
			if (acptr->user->flags & FLAGS_REGISTERED &&
				!(sptr->user->flags & FLAGS_IDENTIFIED))
			{
				sendto_one(sptr, err_str(ERR_REGONLY,
							parv[0]));
				continue;
			}

			if (!notice && MyConnect(sptr) &&
			    acptr->user && (acptr->user->flags & FLAGS_AWAY))
				sendto_one(sptr, rpl_str(RPL_AWAY, parv[0]),
					   acptr->name,
					   (acptr->user->away) ? 
					   acptr->user->away : "Gone");

			sendto_prefix_one(acptr, sptr, ":%s %s %s :%s",
					  parv[0], cmd, nick, parv[2]);
			continue;
		    }
		/*
		** channel msg?
		*/
		if ((IsPerson(sptr) || IsService(sptr) || IsServer(sptr)) &&
		    (chptr = find_channel(nick, NullChn)))
		    {
#ifdef RUSNET_RLINES
			if (MyClient(sptr) && IsRMode(sptr)) {
				if (!notice)
				sendto_one(sptr, err_str(ERR_RESTRICTED,
					   parv[0]));
				continue;
			}
#endif
		    	if (IsServer(sptr) || IsRusnetServices(sptr) ||
					(flag = can_send(sptr, chptr)) == 0)
				sendto_channel_butone(cptr, sptr, chptr,
						      (*nick == '@') ? 1 : 0,
						      ":%s %s %s :%s",
						      parv[0], cmd, nick,
						      parv[2]);

			else if (flag == MODE_NOCOLOR) 
				if(strchr(parv[2],0x03))
					sendto_one(sptr, err_str(ERR_NOCOLOR, 
						   parv[0]), nick);
				else 
				    sendto_channel_butone(cptr, sptr, chptr,
							  (*nick == '@') ? 1 : 0,
							  ":%s %s %s :%s",
							  parv[0], cmd, nick,
							  parv[2]);

			else if (!notice)
				sendto_one(sptr, err_str(ERR_CANNOTSENDTOCHAN,
					   parv[0]), nick);
			continue;
		    }
	
		/*
		** the following two cases allow masks in NOTICEs
		** (for OPERs only)
		**
		** Armin, 8Jun90 (gruner@informatik.tu-muenchen.de)
		*/
		if ((*nick == '$' || *nick == '#') && IsAnOper(sptr))
		    {
			if (!(s = (char *)rindex(nick, '.')))
			    {
				sendto_one(sptr, err_str(ERR_NOTOPLEVEL,
					   parv[0]), nick);
				continue;
			    }
			while (*++s)
				if (*s == '.' || *s == '*' || *s == '?')
					break;
			if (*s == '*' || *s == '?')
			    {
				sendto_one(sptr, err_str(ERR_WILDTOPLEVEL,
					   parv[0]), nick);
				continue;
			    }
			sendto_match_butone(IsServer(cptr) ? cptr : NULL, 
					    sptr, nick + 1,
					    (*nick == '#') ? MATCH_HOST :
							     MATCH_SERVER,
					    ":%s %s %s :%s", parv[0],
					    cmd, nick, parv[2]);
			continue;
		    }
		
		/*
		** nick!user@host addressed?
		*/
		if ((user = (char *)index(nick, '!')) &&
		    (host = (char *)index(nick, '@')))
		    {
			*user = '\0';
			*host = '\0';
			if ((acptr = find_person(nick, NULL)) &&
			    !strcasecmp(user+1, acptr->user->username) &&
			    !strcasecmp(host+1,
					(acptr->user->flags & FLAGS_VHOST) ?
					acptr->user->host : acptr->sockhost))

			    {
				if (acptr->user->flags & FLAGS_REGISTERED &&
					!(sptr->user->flags & FLAGS_IDENTIFIED))
				{
					sendto_one(sptr, err_str(ERR_REGONLY,
								parv[0]));
					continue;
				}

				sendto_prefix_one(acptr, sptr, ":%s %s %s :%s",
						  parv[0], cmd, nick, parv[2]);
				*user = '!';
				*host = '@';
				continue;
			    }
			*user = '!';
			*host = '@';
		    }

		/*
		** user[%host]@server addressed?
		*/
		if ((server = (char *)index(nick, '@')) &&
		    (acptr = find_server(server + 1, NULL)))
		    {
			/*
			** Not destined for a user on me :-(
			*/
			if (!IsMe(acptr))
			    {
				sendto_one(acptr,":%s %s %s :%s", parv[0],
					   cmd, nick, parv[2]);
				continue;
			    }
			*server = '\0';

			if ((host = (char *)index(nick, '%')))
				*host++ = '\0';

			/*
			** Look for users which match the destination host
			** (no host == wildcard) and if one and one only is
			** found connected to me, deliver message!
			*/
			acptr = find_userhost(nick, host, NULL, &count);
			if (server)
				*server = '@';
			if (host)
				*--host = '%';
			if (acptr)
			    {
				if (count == 1)
				{
					if (acptr->user->flags & FLAGS_REGISTERED && !(sptr->user->flags & FLAGS_IDENTIFIED))
					{
						sendto_one(sptr, err_str(ERR_REGONLY, nick));
						continue;
					}
					sendto_prefix_one(acptr, sptr,
							  ":%s %s %s :%s",
					 		  parv[0], cmd,
							  nick, parv[2]);
				}
				else if (!notice)
					sendto_one(sptr, err_str(
						   ERR_TOOMANYTARGETS,
						   parv[0]), "Duplicate", nick,
						   "No Message Delivered");
				continue;
			    }
		    }
		else if ((host = (char *)index(nick, '%')))
		    {
			/*
			** user%host addressed?
			*/
			*host++ = '\0';
			acptr = find_userhost(nick, host, NULL, &count);
			*--host = '%';
			if (acptr)
			    {
				if (count == 1)
				{
					if (acptr->user->flags & FLAGS_REGISTERED && !(sptr->user->flags & FLAGS_IDENTIFIED))
					{
						sendto_one(sptr,
							err_str(ERR_REGONLY,
									nick));
						continue;
					}
					sendto_prefix_one(acptr, sptr,
							  ":%s %s %s :%s",
					 		  parv[0], cmd,
							  nick, parv[2]);
				}
				else if (!notice)
					sendto_one(sptr, err_str(
						   ERR_TOOMANYTARGETS,
						   parv[0]), "Duplicate", nick,
						   "No Message Delivered");
				continue;
			    }
		    }
		if (!notice)
			sendto_one(sptr, err_str(ERR_NOSUCHNICK, parv[0]),
				   nick);
	    }
    return penalty;
}

/*
** m_private
**	parv[0] = sender prefix
**	parv[1] = receiver list
**	parv[2] = message text
*/

int	m_private(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	return m_message(cptr, sptr, parc, parv, 0);
}

/*
** m_notice
**	parv[0] = sender prefix
**	parv[1] = receiver list
**	parv[2] = notice text
*/

int	m_notice(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	return m_message(cptr, sptr, parc, parv, 1);
}

/*
** who_one
**	sends one RPL_WHOREPLY to sptr concerning acptr & repchan
*/
static	void	who_one(sptr, acptr, repchan, lp)
aClient *sptr, *acptr;
aChannel *repchan;
Link *lp;
{
	char	status[6];
	int	i = 0;

	if (acptr->user->flags & FLAGS_AWAY)
		status[i++] = 'G';
	else
		status[i++] = 'H';
	if (acptr->user->flags & FLAGS_VHOST)
		status[i++] = 'x';
	if (IsRMode(acptr))
		status[i++] = 'b';
#ifdef USE_SSL
	if (IsSMode(acptr))
		status[i++] = 's';
#endif
	if (IsAnOper(acptr))
		status[i++] = '*';
	if ((repchan != NULL) && (lp == NULL))
		lp = find_user_link(repchan->members, acptr);
	if (lp != NULL)
	    {
		if (lp->flags & CHFL_CHANOP)
			status[i++] = '@';
		else if (lp->flags & CHFL_HALFOP)
			status[i++] = '%';
		else if (lp->flags & CHFL_VOICE)
			status[i++] = '+';
	    }
	status[i] = '\0';

	sendto_one(sptr, rpl_str(RPL_WHOREPLY, sptr->name),
			(repchan) ? (repchan->chname) : "*",
			acptr->user->username,
			(acptr->user->flags & FLAGS_VHOST) ?
			acptr->user->host : acptr->sockhost,
			acptr->user->server, acptr->name,
			status, acptr->hopcount, acptr->info);
}


/*
** who_channel
**	lists all users on a given channel
*/
static	void	who_channel(sptr, chptr, oper)
aClient *sptr;
aChannel *chptr;
int oper;
{
	Reg	Link	*lp;
	int	member;

	if (!IsAnonymous(chptr))
	    {
		member = IsAnOper(sptr) | IsMember(sptr, chptr);
		if (member || !SecretChannel(chptr))
			for (lp = chptr->members; lp; lp = lp->next)
			    {
				if (oper && !IsAnOper(lp->value.cptr))
					continue;
				if (IsInvisible(lp->value.cptr) && !member)
					continue;
				who_one(sptr, lp->value.cptr, chptr, lp);
			    }
	    }
	else if ((lp = find_user_link(chptr->members, sptr)))
		who_one(sptr, lp->value.cptr, chptr, lp);
}

/*
** who_find
**	lists all (matching) users.
**	CPU intensive, but what can be done?
*/
static	void	who_find(sptr, mask, oper)
aClient *sptr;
char *mask;
int oper;
{
	aChannel *chptr, *ch2ptr;
	Link	*lp;
	int	member;
	int	showperson, isinvis, isop;
	aClient	*acptr;
//	FILE	*aa;
	
	isop	= IsAnOper(sptr);

	for (acptr = client; acptr; acptr = acptr->next)
	    {
		ch2ptr = NULL;
			
		if (!IsPerson(acptr))
			continue;
		if (oper && !IsAnOper(acptr))
			continue;
		showperson = 0;
		/*
		 * Show user if they are on the same channel, or not
		 * invisible and on a non secret channel (if any).
		 * Do this before brute force match on all relevant
		 * fields since these are less cpu intensive (I
		 * hope :-) and should provide better/more shortcuts
		 * -avalon
		 */
		isinvis = (isop) ? 0 : IsInvisible(acptr);
		
		for (lp = acptr->user->channel; lp; lp = lp->next)
		    {
			chptr = lp->value.chptr;
			if (IsAnonymous(chptr))
				continue;
			member = isop | IsMember(sptr, chptr);
			if (isinvis && !member)
				continue;
			if (member || (!isinvis && PubChannel(chptr)))
			    {
				showperson = 1;
				if (!IsAnonymous(chptr) ||
				    acptr != sptr)
				    {
					ch2ptr = chptr;
					break;
				    }
			    }
			if (HiddenChannel(chptr) &&
			    !SecretChannel(chptr) && !isinvis)
				showperson = 1;
		    }
		if (!acptr->user->channel && !isinvis)
			showperson = 1;
		/*
		** This is brute force solution, not efficient...? ;( 
		** Show entry, if no mask or any of the fields match
		** the mask. --msa
		*/
		if ((isop || showperson) &&
		    (!mask ||
		     match(mask, acptr->name) == 0 ||
		     match(mask, acptr->user->username) == 0 ||
		     match(mask, acptr->user->host) == 0 ||
		     match(mask, acptr->sockhost) == 0 ||
		     match(mask, acptr->user->server) == 0 ||
		     match(mask, acptr->info) == 0))
			who_one(sptr, acptr, ch2ptr, NULL);
	    }
}

/*
** m_who
**	parv[0] = sender prefix
**	parv[1] = nickname mask list
**	parv[2] = additional selection flag, only 'o' for now.
*/
int	m_who(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	aChannel *chptr;
	int	oper = parc > 2 ? (*parv[2] == 'o' ): 0; /* Show OPERS only */
	int	penalty = 0;
	char	*p, *mask, *channame;

	if (parc < 2)
	{
		who_find(sptr, NULL, oper);
		sendto_one(sptr, rpl_str(RPL_ENDOFWHO, parv[0]), "*");
		/* it was very CPU intensive */
		return MAXPENALTY;
	}

	/* get rid of duplicates */
	parv[1] = canonize(parv[1]);

	for (p = NULL, mask = strtoken(&p, parv[1], ",");
	    mask && penalty <= MAXPENALTY;
		mask = strtoken(&p, NULL, ","))
	{ 
		channame = NULL;
		penalty += 1;

		/* find channel user last joined, we might need it later */
		if (sptr->user && sptr->user->channel)
			channame = sptr->user->channel->value.chptr->chname;

#if 0
		/* I think it's useless --Beeth */
		clean_channelname(mask);
#endif

		/* simplify mask */
		(void)collapse(mask);

		/*
		** We can never have here !mask 
		** or *mask == '\0', since it would be equal
		** to parc == 1, that is 'WHO' and/or would not
		** pass through above for loop.
		*/
		if (mask[1] == '\0' && mask[0] == '0')
		{
			/*
			** 'WHO 0' - do who_find() later
			*/
			mask = NULL;
			channame = NULL;
		}
		else if (mask[1] == '\0' && mask[0] == '*')
		{
			/*
			** 'WHO *'
			** If user was on any channel, list the one
			** he joined last.
			*/
			mask = NULL;
		}
		else
		{
			/*
			** Try if mask was channelname and if yes, do
			** who_channel, else if mask was nick, do who_one.
			** Else do horrible who_find()
			*/
			channame = mask;
		}
		
		if (IsChannelName(channame))
		{
			chptr = find_channel(channame, NULL);
			if (chptr)
			{
				who_channel(sptr, chptr, oper);
			}
			else
			{
				/*
				** 'WHO #nonexistant'.
				*/
				penalty += 1;
			}
		}
		else 
		{
			aClient	*acptr = NULL;

			if (mask)
			{
				/*
				** Here mask can be NULL. It doesn't matter,
				** since find_client would return NULL.
				** Just saving one function call. ;)
				*/
				acptr = find_client(mask, NULL);
				if (acptr && !IsClient(acptr))
				{
					acptr = NULL;
				}
			}
			if (acptr)
			{
				/* We found client, so send WHO for it */
				who_one(sptr, acptr, NULL, NULL);
			}
			else
			{
				/*
				** All nice chances lost above. 
				** We must hog our server with that.
				*/
				who_find(sptr, mask, oper);
				penalty += MAXPENALTY;
			}
		}
		sendto_one(sptr, rpl_str(RPL_ENDOFWHO, parv[0]),
			   BadPtr(mask) ?  "*" : mask);
	}
	return penalty;
}

/* send_whois() is used by m_whois() to send whois reply to sptr, for acptr */
static void
send_whois(sptr, acptr, parc, parv)
aClient	*sptr, *acptr;
int     parc;
char    *parv[];


{
	static anUser UnknownUser =
	    {
		NULL,	/* channel */
		NULL,   /* invited */
		NULL,	/* uwas */
		NULL,	/* away */
		0,	/* last */
		1,      /* refcount */
		0,	/* joined */
		0,	/* flags */
		NULL,	/* servp */
		NULL,	/* next, prev, bcptr */
		"<Unknown>",	/* user */
		"<Unknown>",	/* host */
		"<Unknown>",	/* server */
	    };
	Link	*lp;
	anUser	*user;
	aChannel *chptr;
	aClient *a2cptr;
	int len, mlen;
	char *name;

	user = acptr->user ? acptr->user : &UnknownUser;
	name = (!*acptr->name) ? "?" : acptr->name;

	a2cptr = find_server(user->server, NULL);

	if (acptr->user && acptr->user->flags & FLAGS_VHOST)
	{
	    sendto_one(sptr, rpl_str(RPL_WHOISUSER, sptr->name), name,
			user->username, user->host, acptr->info);

	    if (sptr == acptr || IsOper(sptr))
		sendto_one(sptr, rpl_str(RPL_WHOISHOST, sptr->name),
					   acptr->name, acptr->sockhost);
	}
	else
	    sendto_one(sptr, rpl_str(RPL_WHOISUSER, sptr->name), name,
			user->username, acptr->sockhost, acptr->info);

	mlen = strlen(ME) + strlen(sptr->name) + 6 + strlen(name);
	*buf = '\0';
	/* Mark IRC Services from being at any channel */
	if (!IsRusnetServices(acptr))
	    for (len = 0, lp = user->channel; lp; lp = lp->next)
	    {
		chptr = lp->value.chptr;
		if (((!IsAnonymous(chptr) || acptr == sptr) &&
		    ShowChannel(sptr, chptr)) || IsAnOper(sptr))
		    {
			if (len + strlen(chptr->chname)
			    > (size_t) BUFSIZE - 4 - mlen)
			    {
				sendto_one(sptr, ":%s %d %s %s :%s", ME,
					   RPL_WHOISCHANNELS, sptr->name, name,
					   buf);
				*buf = '\0';
				len = 0;
			    }
			if (is_chan_op(acptr, chptr))
				*(buf + len++) = '@';
			else if (is_chan_anyop(acptr, chptr))
				*(buf + len++) = '%';
			else if (has_voice(acptr, chptr))
				*(buf + len++) = '+';
			if (len)
				*(buf + len) = '\0';
			(void)strcpy(buf + len, chptr->chname);
			len += strlen(chptr->chname);
			(void)strcat(buf + len, " ");
			len++;
		    }
	    }

	if (buf[0] != '\0')
		sendto_one(sptr, rpl_str(RPL_WHOISCHANNELS, sptr->name), name,
			   buf);

	sendto_one(sptr, rpl_str(RPL_WHOISSERVER, sptr->name),
		   name, user->server,
		   a2cptr ? a2cptr->info:"*Not On This Net*");

	if (user->flags & FLAGS_AWAY)
		sendto_one(sptr, rpl_str(RPL_AWAY, sptr->name), name,
			   (user->away) ? user->away : "Gone");
#ifndef USE_OLD8BIT
	/* conversion charset name cannot be empty */
	if (MyConnect(acptr) && acptr->user && acptr->conv)
		sendto_one(sptr, rpl_str(RPL_CHARSET, sptr->name),
					acptr->name, conv_charset(acptr->conv));
#else
        if (MyConnect(acptr) && acptr->user && acptr->transptr &&
						acptr->transptr->id)
		sendto_one(sptr, rpl_str(RPL_CHARSET, sptr->name),
					acptr->name, acptr->transptr->id);
#endif
# ifdef RUSNET_RLINES
	if (IsRMode(acptr))
	    sendto_one(sptr, rpl_str(RPL_WHOISRMODE, sptr->name), name);
# endif /* RUSNET_RLINES */
#ifdef USE_SSL
	if (IsSMode(acptr))
	    sendto_one(sptr, rpl_str(RPL_USINGSSL, sptr->name), name);
#endif
	if (IsAnOper(acptr))
	    sendto_one(sptr, rpl_str(RPL_WHOISOPERATOR, sptr->name), name);

#if defined(EXTRA_NOTICES) && defined(WHOIS_NOTICES)
	if (IsAnOper(acptr) && acptr != sptr)
	    sendto_one(acptr, ":%s NOTICE %s :WHOIS on YOU requested by %s "
			"(%s@%s) [%s]", ME, acptr->name, parv[0],
			sptr->user->username, sptr->sockhost,
			sptr->user->server);
#endif
	if (acptr->user && MyConnect(acptr))
		sendto_one(sptr, rpl_str(RPL_WHOISIDLE, sptr->name),
			   name, timeofday - user->last, acptr->firsttime);
}

/*
** m_whois
**	parv[0] = sender prefix
**	parv[1] = nickname masklist
*/
int	m_whois(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	Link	*lp;
	aClient *acptr;
	aChannel *chptr;
	char	*nick, *tmp, *tmp2;
	char	*p = NULL;
	int	isop, found = 0;

    	if (parc < 2)
	    {
		sendto_one(sptr, err_str(ERR_NONICKNAMEGIVEN, parv[0]));
		return 1;
	    }

	if (parc > 2)
	    {
		if (hunt_server(cptr,sptr,":%s WHOIS %s :%s", 1,parc,parv) !=
		    HUNTED_ISME)
			return 3;
		parv[1] = parv[2];
	    }

	isop = IsAnOper(sptr);
	tmp = mystrdup(parv[1]);

	for (tmp2 = canonize(tmp); (nick = strtoken(&p, tmp2, ",")); 
		tmp2 = NULL)
	    {
		int	invis, showperson, member, wilds;

		found &= 0x0f;	/* high/boolean, low/counter */
		(void)collapse(nick);
		wilds = (index(nick, '?') || index(nick, '*'));
		/*
		 * We're no longer allowing remote users to generate
		 * requests with wildcard, nor local users with more
		 * than one wildcard target per command.
		 * Max 3 targets per command allowed.
		 */
		if ((wilds && (!MyConnect(sptr) || p)) || found++ > 3)
			break;

		if (!wilds)
		    {
			acptr = hash_find_client(nick, (aClient *)NULL);
			if (!acptr || !IsPerson(acptr))
				sendto_one(sptr,
					   err_str(ERR_NOSUCHNICK, parv[0]),
					   nick);
			else
				send_whois(sptr, acptr, parc, parv);
			continue;
		    }

		for (acptr = client; (acptr = next_client(acptr, nick));
		     acptr = acptr->next)
		    {
			if (IsServer(acptr) || IsService(acptr))
				continue;
			/*
			 * I'm always last :-) and acptr->next == NULL!!
			 */
			if (IsMe(acptr))
				break;
			/*
			 * 'Rules' established for sending a WHOIS reply:
			 * - if wildcards are being used don't send a reply if
			 *   the querier isnt any common channels and the
			 *   client in question is invisible and wildcards are
			 *   in use (allow exact matches only);
			 * - only send replies about common or public channels
			 *   the target user(s) are on;
			 */
			invis = (isop) ? 0 : (acptr->user) ?
				(acptr->user->flags & FLAGS_INVISIBLE) : 0;
			member = (isop ||
				  acptr->user && acptr->user->channel) ? 1 : 0;
			showperson = (wilds && !invis && !member) || !wilds;
			for (lp = (acptr->user) ? acptr->user->channel : NULL;
			     lp; lp = lp->next)
			    {
				chptr = lp->value.chptr;
				if (IsAnOper(sptr))
				    {
					showperson = 1;
					break;
				    }
				if (IsAnonymous(chptr))
					continue;
				member = IsMember(sptr, chptr);
				if (invis && !member)
					continue;
				if (member || (!invis && PubChannel(chptr)))
				    {
					showperson = 1;
					break;
				    }
				if (!invis && HiddenChannel(chptr) &&
				    !SecretChannel(chptr))
					showperson = 1;
			    }
			if (!showperson)
				continue;

			found |= 0x10;

			send_whois(sptr, acptr, parc, parv);
		    }
		if (!(found & 0x10))
		    {
			if (strlen(nick) > (size_t) UNINICKLEN)
				nick[UNINICKLEN] = '\0';
			sendto_one(sptr, err_str(ERR_NOSUCHNICK, parv[0]),
				   nick);
		    }
		if (p)
			p[-1] = ',';
	    }
	sendto_one(sptr, rpl_str(RPL_ENDOFWHOIS, parv[0]), parv[1]);

	MyFree(tmp);

	return 2;
}

/*
** m_user
**	parv[0] = sender prefix
**	parv[1] = username (login name, account)
**	parv[2] = client host name (used only from other servers)
**	parv[3] = server host name (used only from other servers)
**	parv[4] = users real name info
**	parv[5] = users mode (is only used internally by the server,
**		  NULL otherwise)
*/
int	m_user(cptr, sptr, parc, parv)
aClient	*cptr, *sptr;
int	parc;
char	*parv[];
{
#define	UFLAGS	(FLAGS_INVISIBLE|FLAGS_WALLOP|FLAGS_VHOST|FLAGS_RMODE)
	char	*username, *host, *server, *realname;
	anUser	*user;

	/* Reject new USER */
	if (IsServer(sptr) || IsService(sptr) || sptr->user)
	    {
		sendto_one(sptr, err_str(ERR_ALREADYREGISTRED, parv[0]));
		return 1;
   	    }
	if (parc > 2 && (username = (char *)index(parv[1],'@')))
		*username = '\0'; 
	if (parc < 5 || *parv[1] == '\0' || *parv[2] == '\0' ||
	    *parv[3] == '\0' || *parv[4] == '\0')
	    {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]), "USER");
		if (IsServer(cptr))
		    {
			/* send error */
			sendto_flag(SCH_NOTICE,
				    "bad USER param count for %s from %s",
				    parv[0], get_client_name(cptr, FALSE));
			/*
			** and kill it, as there's no reason to expect more
			** USER messages about it, or we'll create a ghost.
			*/
			sendto_one(cptr,
				   ":%s KILL %s :%s (bad USER param count)",
				   ME, parv[0], ME);
			sptr->flags |= FLAGS_KILLED;
			exit_client(NULL, sptr, &me, "bad USER param count");
		    }
		return 1;
	    }

	/* Copy parameters into better documenting variables */

	username = (parc < 2 || BadPtr(parv[1])) ? "<bad-boy>" : parv[1];
	host     = (parc < 3 || BadPtr(parv[2])) ? "<nohost>" : parv[2];
	server   = (parc < 4 || BadPtr(parv[3])) ? "<noserver>" : parv[3];
	realname = (parc < 5 || BadPtr(parv[4])) ? "<bad-realname>" : parv[4];

 	user = make_user(sptr);

	if (MyConnect(sptr))
	    {
		user->servp = me.serv;
		me.serv->refcnt++;
#ifndef	NO_DEFAULT_INVISIBLE
		SetInvisible(sptr);
#endif
#ifdef USE_SSL
		if (IsSSL(sptr)) {
			SetSMode(sptr);
			Debug((DEBUG_DEBUG, "Setting S-Mode for user %s", sptr->sockhost));
		}
#endif
		if (sptr->flags & FLAGS_RILINE)
			sptr->user->flags |= FLAGS_RESTRICTED;
#ifndef	NO_DEFAULT_VHOST
		else SetVHost(sptr);
#endif
		/* What did it mean to be? Mark it out  --erra */
#if 0
		sptr->user->flags |= (UFLAGS & atoi(host));
#endif
		user->server = find_server_string(me.serv->snum);
	    }
	else
	    {
		aClient	*acptr = NULL;
		aServer	*sp = NULL;

		if (!(sp = find_tokserver(atoi(server), cptr, NULL)))
		    {
			/*
			** Why? Why do we keep doing this?
			** s_service.c had the same kind of kludge.
			** Can't we get rid of this? - krys
			*/
			acptr = find_server(server, NULL);
			if (acptr)
				sendto_flag(SCH_ERROR,
			    "ERROR: SERVER:%s uses wrong syntax for NICK (%s)",
					    get_client_name(cptr, FALSE),
					    parv[0]);
		    }
		if (acptr)
			sp = acptr->serv;
		else if (!sp)
		    {
			sendto_flag(SCH_ERROR,
                        	    "ERROR: USER:%s without SERVER:%s from %s",
				    parv[0], server,
				    get_client_name(cptr, FALSE));
			ircstp->is_nosrv++;
			return exit_client(NULL, sptr, &me, "No Such Server");
		    }
		user->servp = sp;
		user->servp->refcnt++;

		Debug((DEBUG_DEBUG, "from %s user %s server %s -> %#x %s",
			parv[0], username, server, sp, sp->bcptr->name));
		user->server = find_server_string(sp->snum);
	    }
	strncpyzt(user->host, host, sizeof(user->host));
	reorder_client_in_list(sptr);
	if (sptr->info != DefInfo)
		MyFree(sptr->info);
	unistrcut(realname, REALLEN);
	sptr->info = mystrdup(realname);
	if (sptr->name[0]) /* NICK already received, now we have USER... */
	    {
		if ((parc == 6) && IsServer(cptr)) /* internal m_user() */
		    {
			char	*pv[4];

			pv[0] = ME;
			pv[1] = sptr->name;
			pv[2] = parv[5];
			pv[3] = NULL;
			m_umode(NULL, sptr, 3, pv);/*internal fake call again*/
			/* The internal m_umode does NOT propagate to 2.8
			** servers. (it can NOT since NICK/USER hasn't been
			** sent yet). See register_user()
			*/
		    }
		return register_user(cptr, sptr, sptr->name, username);
	    }
	else
		strncpyzt(sptr->user->username, username, USERLEN+1);
	return 2;
}

/*
** m_quit
**	parv[0] = sender prefix
**	parv[1] = comment
*/
int	m_quit(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
    {
	static	char	quitc[] = "I Quit";
	char *newcomment;
	int newlen, result;
	register char *comment = (parc > 1 && parv[1]) ? parv[1] : quitc;

	if (IsServer(sptr))
		return 0;
	else {
		if (MyClient(sptr) || MyService(sptr))
			if (!strncmp("Local Kill", comment, 10) ||
				!strncmp(comment, "Killed", 6))
			        comment = quitc;
#ifdef RUSNET_RLINES
		if (MyClient(sptr) && IsRMode(sptr))
			comment = quitc;
#endif
		unistrcut(comment, TOPICLEN - 2);
		if (!MyClient(sptr))
			return exit_client(cptr, sptr, sptr, comment);
		else {
			newlen = strlen(comment);
			newcomment = MyMalloc(newlen + 3);
		        *newcomment = '"';
		        strcpy(newcomment + 1, comment);
		        newcomment[newlen + 1] = '"';
		        newcomment[newlen + 2] = '\0';
		        result = exit_client(cptr, sptr, sptr, newcomment);
		        MyFree(newcomment);
		        return result;
		}    
	}
    }

/*
** m_kill
**	parv[0] = sender prefix
**	parv[1] = kill victim
**	parv[2] = kill path
*/
int	m_kill(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	aClient *acptr;
	char	*inpath = get_client_name(cptr,FALSE);
	char	*user, *path, *killer;
	int	chasing = 0;

	if (parc < 2 || *parv[1] == '\0')
	    {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]), "KILL");
		return 1;
	    }

	user = parv[1];
	path = parv[2]; /* Either defined or NULL (parc >= 2!!) */

	if (IsAnOper(cptr))
	    {
		if (BadPtr(path))
		    {
			sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]),
				   "KILL");
			return 1;
		    }
		unistrcut(path, TOPICLEN);
	    }

	if (!(acptr = find_client(user, NULL)))
	    {
		/*
		** If the user has recently changed nick, we automaticly
		** rewrite the KILL for this new nickname--this keeps
		** servers in synch when nick change and kill collide
		*/
		if (!(acptr = get_history(user, (long)KILLCHASETIMELIMIT)))
		    {
			if (!IsServer(sptr))
				sendto_one(sptr, err_str(ERR_NOSUCHNICK, 
							 parv[0]), user);
			return 1;
		    }
		/*
		** Just trying to keep away from kill collision, lock the
		** nick kill message was recieved for with lock_nick(nick)
		** -kmale
		*/
#ifdef LOCKRECENTCHANGE
		lock_nick(user, sptr->name);
#endif
		sendto_one(sptr,":%s NOTICE %s :KILL changed from %s to %s",
			   ME, parv[0], user, acptr->name);
		chasing = 1;
	    }
	if (!MyConnect(acptr) && IsLocOp(cptr))
	    {
		sendto_one(sptr, err_str(ERR_NOPRIVILEGES, parv[0]));
		return 1;
	    }
	if (IsServer(acptr) || IsMe(acptr))
	    {
		sendto_flag(SCH_ERROR, "%s tried to KILL server %s: %s %s %s",
			    sptr->name, acptr->name, parv[0], parv[1], parv[2]);
		sendto_one(sptr, err_str(ERR_CANTKILLSERVER, parv[0]),
			   acptr->name);
		return 1;
	    }

#ifdef	LOCAL_KILL_ONLY
	if (MyOper(sptr) && !MyConnect(acptr))
	    {
		sendto_one(sptr, ":%s NOTICE %s :Nick %s isnt on your server",
			   ME, parv[0], acptr->name);
		return 1;
	    }
#endif
	if (!IsServer(cptr))
	    {
		/*
		** The kill originates from this server, initialize path.
		** (In which case the 'path' may contain user suplied
		** explanation ...or some nasty comment, sigh... >;-)
		**
		**	...!operhost!oper
		**	...!operhost!oper (comment)
		*/
#ifdef UNIXPORT
		if (IsUnixSocket(cptr)) /* Don't use get_client_name syntax */
			inpath = me.sockhost;
		else
#endif
			inpath = cptr->sockhost;
		if (!BadPtr(path))
		    {
			sprintf(buf, "%s%s (%s)",
				cptr->name, IsOper(sptr) ? "" : "(L)", path);
			path = buf;
		    }
		else
			path = cptr->name;
	    }
	else if (BadPtr(path))
		 path = "*no-path*"; /* Bogus server sending??? */
	/*
	** Notify all *local* opers about the KILL (this includes the one
	** originating the kill, if from this server--the special numeric
	** reply message is not generated anymore).
	**
	** Note: "acptr->name" is used instead of "user" because we may
	**	 have changed the target because of the nickname change.
	*/
	if (IsLocOp(sptr) && !MyConnect(acptr))
	    {
		sendto_one(sptr, err_str(ERR_NOPRIVILEGES, parv[0]));
		return 1;
	    }
	sendto_flag(SCH_KILL,
		    "Received KILL message for %s (%s@%s)%s. From %s Path: %s!%s",
		    acptr->name, (acptr->user) ? acptr->user->username : "unknown", 
		    acptr->sockhost, (acptr->user) ? acptr->user->server : "unknown", 
		    parv[0], inpath, path);
	/*
	** And pass on the message to other servers. Note, that if KILL
	** was changed, the message has to be sent to all links, also
	** back.
	** Suicide kills are NOT passed on --SRB
	*/
	if (!MyConnect(acptr) || !MyConnect(sptr) || !IsAnOper(sptr))
	    {
		sendto_serv_butone(cptr, ":%s KILL %s :%s!%s",
				   parv[0], acptr->name, inpath, path);
		if (chasing && !IsClient(cptr))
			sendto_one(cptr, ":%s KILL %s :%s!%s",
				   ME, acptr->name, inpath, path);
		acptr->flags |= FLAGS_KILLED;
	    }
#ifdef	USE_SERVICES
	check_services_butone(SERVICE_WANT_KILL, NULL, sptr, 
			      ":%s KILL %s :%s!%s", parv[0], acptr->name,
			      inpath, path);
#endif

	/*
	** Tell the victim she/he has been zapped, but *only* if
	** the victim is on current server--no sense in sending the
	** notification chasing the above kill, it won't get far
	** anyway (as this user don't exist there any more either)
	*/
	if (MyConnect(acptr))
		sendto_prefix_one(acptr, sptr,":%s KILL %s :%s!%s",
				  parv[0], acptr->name, inpath, path);
	/*
	** Set FLAGS_KILLED. This prevents exit_one_client from sending
	** the unnecessary QUIT for this. (This flag should never be
	** set in any other place)
	*/
	if (MyConnect(acptr) && MyConnect(sptr) && IsAnOper(sptr))
	    {
		acptr->exitc = EXITC_KILL;
		sprintf(buf2, "Local Kill by %s (%s)", sptr->name,
			BadPtr(parv[2]) ? sptr->name : parv[2]);
	    }
	else
	    {
		if ((killer = index(path, ' ')))
		    {
			while (killer > path && *killer != '!')
				killer--;
			if (killer != path)
				killer++;
		    }
		else
			killer = path;
		sprintf(buf2, "Killed (%s)", killer);
	    }
	return exit_client(cptr, acptr, sptr, buf2);
}

/***********************************************************************
 * m_away() - Added 14 Dec 1988 by jto. 
 *	    Not currently really working, I don't like this
 *	    call at all...
 *
 *	    ...trying to make it work. I don't like it either,
 *	      but perhaps it's worth the load it causes to net.
 *	      This requires flooding of the whole net like NICK,
 *	      USER, MODE, etc messages...  --msa
 ***********************************************************************/

/*
** m_away
**	parv[0] = sender prefix
**	parv[1] = away message
*/
int	m_away(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	Reg	char	*away, *awy2 = parv[1];
	int	len;

	away = sptr->user->away;

#ifdef RUSNET_RLINES
	if (MyClient(sptr) && IsRMode(sptr))
	    return 5;	
#endif
	if (parc < 2 || !*awy2)	/* Marking as not away */
	    {
		if (away)
		    {
			istat.is_away--;
			istat.is_awaymem -= (strlen(away) + 1);
			MyFree(away);
			sptr->user->away = NULL;
		    }
		if (sptr->user->flags & FLAGS_AWAY)
			sendto_serv_butone(cptr, ":%s MODE %s :-a", parv[0],
					   parv[0]);
		/* sendto_serv_butone(cptr, ":%s AWAY", parv[0]); */
		if (MyConnect(sptr))
			sendto_one(sptr, rpl_str(RPL_UNAWAY, parv[0]));
#ifdef	USE_SERVICES
		check_services_butone(SERVICE_WANT_AWAY, NULL, sptr,
				      ":%s AWAY", parv[0]);
#endif
		sptr->user->flags &= ~FLAGS_AWAY;
		return 1;
	    }

	/* Marking as away */

	len = unistrcut(awy2, TOPICLEN);
	len++;
	/* sendto_serv_butone(cptr, ":%s AWAY :%s", parv[0], awy2); */
#ifdef	USE_SERVICES
	check_services_butone(SERVICE_WANT_AWAY, NULL, sptr,
			      ":%s AWAY :%s", parv[0], awy2);
#endif

	if (away)
	    {
		istat.is_awaymem -= (strlen(away) + 1);
		away = (char *)MyRealloc(away, len);
		istat.is_awaymem += len;
	    }
	else
	    {
		istat.is_away++;
		istat.is_awaymem += len;
		away = (char *)MyMalloc(len);
		sendto_serv_butone(cptr, ":%s MODE %s :+a", parv[0], parv[0]);
	    }

	sptr->user->flags |= FLAGS_AWAY;
	if (MyConnect(sptr))
	    {
		sptr->user->away = away;
		(void)strcpy(away, awy2);
		sendto_one(sptr, rpl_str(RPL_NOWAWAY, parv[0]));
	    }
	return 2;
}

/*
** m_ping
**	parv[0] = sender prefix
**	parv[1] = origin
**	parv[2] = destination
*/
int	m_ping(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	aClient *acptr;
	char	*origin, *destination;

	if (parc < 2 || *parv[1] == '\0')
	    {
		sendto_one(sptr, err_str(ERR_NOORIGIN, parv[0]));
		return 1;
	    }
	origin = parv[1];
	destination = parv[2]; /* Will get NULL or pointer (parc >= 2!!) */

	acptr = find_client(origin, NULL);
	if (!acptr)
		acptr = find_server(origin, NULL);
	if (!acptr || acptr != sptr)
		origin = cptr->name;
	if (!BadPtr(destination) && match(destination, ME) != 0)
	    {
		if ((acptr = find_server(destination, NULL)))
			sendto_one(acptr,":%s PING %s :%s", parv[0],
				   origin, destination);
	    	else
		    {
			sendto_one(sptr, err_str(ERR_NOSUCHSERVER, parv[0]),
				   destination);
			return 1;
		    }
	    }
	else
		sendto_one(sptr, ":%s PONG %s :%s", ME,
			   (destination) ? destination : ME, origin);
	return 1;
    }

/*
** m_pong
**	parv[0] = sender prefix
**	parv[1] = origin
**	parv[2] = destination
*/
int	m_pong(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	aClient *acptr;
	char	*origin, *destination;

	if (parc < 2 || *parv[1] == '\0')
	    {
		sendto_one(sptr, err_str(ERR_NOORIGIN, parv[0]));
		return 1;
	    }

	origin = parv[1];
	destination = parv[2];
	cptr->flags &= ~FLAGS_PINGSENT;
	sptr->flags &= ~FLAGS_PINGSENT;

	if (!BadPtr(destination) && strcasecmp(destination, ME) != 0)
	    {
		if ((acptr = find_client(destination, NULL)) ||
		    (acptr = find_server(destination, NULL))) {
			if (!(MyClient(sptr) && strcasecmp(origin, sptr->name)))
				sendto_one(acptr,":%s PONG %s %s",
					   parv[0], origin, destination);
		} else
			sendto_one(sptr, err_str(ERR_NOSUCHSERVER, parv[0]),
				   destination);
		return 2;
	    }
#ifdef	DEBUGMODE
	else
		Debug((DEBUG_NOTICE, "PONG: %s %s", origin,
		      destination ? destination : "*"));
#endif
	return 1;
    }


/*
** m_oper
**	parv[0] = sender prefix
**	parv[1] = oper name
**	parv[2] = oper password
*/
int	m_oper(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
    {
	aConfItem *aconf;
	char	*name, *password, *encr;

	name = parc > 1 ? parv[1] : NULL;
	password = parc > 2 ? parv[2] : NULL;

	if (!IsServer(cptr) && (BadPtr(name) || BadPtr(password)))
	    {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]), "OPER");
		return 1;
	    }
	
	/* if message arrived from server, trust it, and set to oper */
	    
	if ((IsServer(cptr) || IsMe(cptr)) && !IsOper(sptr))
	    {
		/* we never get here, do we?? (counters!) -krys */
		sptr->user->flags |= FLAGS_OPER;
		sendto_serv_butone(cptr, ":%s MODE %s :+o", parv[0], parv[0]);
		if (IsMe(cptr))
			sendto_one(sptr, rpl_str(RPL_YOUREOPER, parv[0]));
#ifdef	USE_SERVICES
		check_services_butone(SERVICE_WANT_OPER, sptr->user->server, 
				      sptr, ":%s MODE %s :+o", parv[0], 
				      parv[0]);
#endif
		return 1;
	    }
	else if (IsAnOper(sptr))
	    {
		if (MyConnect(sptr))
			sendto_one(sptr, rpl_str(RPL_YOUREOPER, parv[0]));
		return 1;
	    }
	if (!(aconf = find_Oline(name, sptr)))
	    {
#ifdef EXTRA_NOTICES
		sendto_flag(SCH_OPER, "No O:line for the name %s [%s!%s@%s] from %s(%s)", 
			parv[1], sptr->name, sptr->user->username, 
			(sptr->user->flags & FLAGS_VHOST) ?
			sptr->user->host : sptr->sockhost,
			get_client_name(cptr, TRUE), sptr->user->server);
#endif /* EXTRA_NOTICES */
		sendto_one(sptr, err_str(ERR_NOOPERHOST, parv[0]));
		return 1;
	    }
#ifdef CRYPT_OPER_PASSWORD
	/* pass whole aconf->passwd as salt, let crypt() deal with it */

	if (password && aconf->passwd)
	    {
		extern	char *crypt();

		encr = crypt(password, aconf->passwd);
		if (encr == NULL)
		    {
			sendto_flag(SCH_ERROR, "crypt() returned NULL");
#ifdef EXTRA_NOTICES
			sendto_flag(SCH_OPER, "crypt() failed on O:line for the name %s \
				[%s!%s@%s] from %s(%s)",
				parv[1], sptr->name, sptr->user->username, 
				(sptr->user->flags & FLAGS_VHOST) ?
				sptr->user->host : sptr->sockhost,
				get_client_name(cptr, TRUE), sptr->user->server);
#endif /* EXTRA_NOTICES */
			sendto_one(sptr,err_str(ERR_PASSWDMISMATCH, parv[0]));
			return 3;
		    }
	    }
	else
		encr = "";
#else
	encr = password;
#endif  /* CRYPT_OPER_PASSWORD */

	if ((aconf->status & CONF_OPS) &&
	    StrEq(encr, aconf->passwd) && !attach_conf(sptr, aconf))
	    {
		int old = (sptr->user->flags & ALL_UMODES);
		char *s;

		s = index(aconf->host, '@');
		*s++ = '\0';
#ifdef	OPER_REMOTE
		if (aconf->status == CONF_LOCOP)
#else
		if ((match(s,me.sockhost) && !IsLocal(sptr)) ||
		    aconf->status == CONF_LOCOP)
#endif
			SetLocOp(sptr);
		else
			SetOper(sptr);
		*--s =  '@';
		sendto_flag(SCH_NOTICE, "%s (%s@%s) is now operator (%c)",
			parv[0], sptr->user->username, 
			(sptr->user->flags & FLAGS_VHOST) ?
			sptr->user->host : sptr->sockhost,
			IsOper(sptr) ? 'O' : 'o');
		send_umode_out(cptr, sptr, old);
 		sendto_one(sptr, rpl_str(RPL_YOUREOPER, parv[0]));
#ifndef CRYPT_OPER_PASSWORD
		encr = "";
#endif
#ifdef LOG_EVENTS
		sendto_flag(SCH_OPER, "OPER (%s) (%s) by (%s!%s@%s) [%s@%s]",
			name, encr,
		       parv[0], sptr->user->username, sptr->user->host,
		       sptr->auth, 
# ifdef UNIXPORT
		       IsUnixSocket(sptr) ? sptr->sockhost :
# endif
# ifdef INET6
                       inet_ntop(AF_INET6, (char *)&sptr->ip,
						mydummy, MYDUMMY_SIZE)
# else
                       inetntoa((char *)&sptr->ip)
# endif /* INET6 */
							);
#endif /* LOG_EVENTS */
#ifdef	USE_SERVICES
		check_services_butone(SERVICE_WANT_OPER, sptr->user->server, 
				      sptr, ":%s MODE %s :+%c", parv[0],
				      parv[0], IsOper(sptr) ? 'O' : 'o');
#endif
		if (IsAnOper(sptr))
			istat.is_oper++;
	    }
	else
	    {
		(void)detach_conf(sptr, aconf);
#ifdef EXTRA_NOTICES
			sendto_flag(SCH_OPER, "Incorrect OPER password for the name %s \
				[%s!%s@%s] from %s(%s)",
				parv[1], sptr->name, sptr->user->username, 
				(sptr->user->flags & FLAGS_VHOST) ?
				sptr->user->host : sptr->sockhost,
				get_client_name(cptr, TRUE), sptr->user->server);
#endif /* EXTRA_NOTICES */
    
		sendto_one(sptr,err_str(ERR_PASSWDMISMATCH, parv[0]));
	    }
	return 3;
    }

/***************************************************************************
 * m_pass() - Added Sat, 4 March 1989
 ***************************************************************************/

/*
** m_pass
**	parv[0] = sender prefix
**	parv[1] = password
**	parv[2] = protocol & server versions (server only)
**	parv[3] = server id & options (server only)
**	parv[4] = (optional) link options (server only)                  
*/
int	m_pass(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
    {
	char *password = parc > 1 ? parv[1] : NULL;

	if (BadPtr(password))
	    {
		sendto_one(cptr, err_str(ERR_NEEDMOREPARAMS, parv[0]), "PASS");
		return 1;
	    }
	/* Temporarily store PASS pwd *parameters* into info field */
	if (parc > 2 && parv[2])
	    {
		strncpyzt(buf, parv[2], 15); 
		if (parc > 3 && parv[3])
		    {
			strcat(buf, " ");
			strncat(buf, parv[3], 100);
			if (parc > 4 && parv[4])
			    {
				strcat(buf, " ");
				strncat(buf, parv[4], 6);
			    }
		    }
		if (cptr->info != DefInfo)
			MyFree(cptr->info);
		cptr->info = mystrdup(buf);
	    }
	strncpyzt(cptr->passwd, password, sizeof(cptr->passwd));
	return 1;
    }

/*
 * m_userhost added by Darren Reed 13/8/91 to aid clients and reduce
 * the need for complicated requests like WHOIS. It returns user/host
 * information only (no spurious AWAY labels or channels).
 */
int	m_userhost(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	char	*p = NULL;
	aClient	*acptr;
	Reg	char	*s;
	Reg	int	i, len;
	int	idx = 1;

	if (parc < 2)
	    {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]),
			   "USERHOST");
		return 1;
	    }

	(void)strcpy(buf, rpl_str(RPL_USERHOST, parv[0]));
	len = strlen(buf);
	*buf2 = '\0';

	for (i = 5, s = strtoken(&p, parv[idx], " "); i && s; i--)
	     {
		if ((acptr = find_person(s, NULL)))
		    {
			if (*buf2)
				(void)strcat(buf, " ");
			sprintf(buf2, "%s%s=%c%s@%s", acptr->name,
				IsAnOper(acptr) ? "*" : "",
				(acptr->user->flags & FLAGS_AWAY) ? '-' : '+',
				acptr->user->username,
					(acptr->user->flags & FLAGS_VHOST) ?
					acptr->user->host : acptr->sockhost);
			(void)strncat(buf, buf2, sizeof(buf) - len);
			len += strlen(buf2);
			if (len > BUFSIZE - (UNINICKLEN + 5 + HOSTLEN + USERLEN))
			    {
				sendto_one(sptr, "%s", buf);
				(void)strcpy(buf, rpl_str(RPL_USERHOST,
					     parv[0]));
				len = strlen(buf);
				*buf2 = '\0';
			    }
		    }
		s = strtoken(&p, (char *)NULL, " ");
		if (!s && parv[++idx])
		    {
			p = NULL;
			s = strtoken(&p, parv[idx], " ");
		    }
	    }
	sendto_one(sptr, "%s", buf);
	return 1;
}

/*
 * m_ison added by Darren Reed 13/8/91 to act as an efficent user indicator
 * with respect to cpu/bandwidth used. Implemented for NOTIFY feature in
 * clients. Designed to reduce number of whois requests. Can process
 * nicknames in batches as long as the maximum buffer length.
 *
 * format:
 * ISON :nicklist
 */

int	m_ison(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	Reg	aClient *acptr;
	Reg	char	*s, **pav = parv;
	Reg	int	len = 0, i;
	char	*p = NULL;

	if (parc < 2)
	    {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]), "ISON");
		return 1;
	    }

	(void)strcpy(buf, rpl_str(RPL_ISON, *parv));
	len = strlen(buf);

	for (s = strtoken(&p, *++pav, " "); s; s = strtoken(&p, NULL, " "))
		if ((acptr = find_person(s, NULL)))
		    {
			i = strlen(acptr->name);
			if (len + i > sizeof(buf) - 4)	
			{
				/* leave room for " \r\n\0" */
				break;
			}
			(void) strcpy(buf + len, acptr->name);
			len += i;
			(void) strcpy(buf + len++, " ");
		    }
	sendto_one(sptr, "%s", buf);
	return 1;
}

/*
 * m_umode() added 15/10/91 By Darren Reed.
 * parv[0] - sender (can be NULL, see below..)
 * parv[1] - username to change mode for
 * parv[2] - modes to change
 */
int	m_umode(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	Reg	int	flag;
	Reg	int	*s;
	Reg	char	**p, *m;
	aClient	*acptr = NULL;
	int	what, setflags, penalty = 0;

	what = MODE_ADD;

	if (parc < 2)
	    {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]), "MODE");
		return 1;
	    }

	if (cptr && !(acptr = find_person(parv[1], NULL)))
	    {
		if (MyConnect(sptr))
			sendto_one(sptr, err_str(ERR_NOSUCHCHANNEL, parv[0]),
				   parv[1]);
		return 1;
	    }
	if (cptr == NULL)
		/* internal call which has to be handled in a special way */
		acptr = sptr;

	else if ((IsServer(sptr) || sptr != acptr || acptr->from != sptr->from))
	    {
		if (IsServer(cptr))
			sendto_flag(SCH_ERROR,
				  "%s MODE for User %s From %s!%s",
				  ME, parv[1],
				  get_client_name(cptr, FALSE), sptr->name);
		else
			sendto_one(sptr, err_str(ERR_USERSDONTMATCH, parv[0]));
			return 1;
	    }
 
	if (parc < 3)
	    {
		m = buf;
		*m++ = '+';
		for (s = user_modes; (flag = *s) && (m - buf < BUFSIZE - 4);
		     s += 2)
			if (sptr->user->flags & flag)
				*m++ = (char)(*(s+1));
		*m = '\0';
		sendto_one(sptr, rpl_str(RPL_UMODEIS, parv[0]), buf);
		return 1;
	    }

	/* find flags already set for user */
	setflags = 0;
	for (s = user_modes; (flag = *s); s += 2)
		if (sptr->user->flags & flag)
			setflags |= flag;

	/*
	 * parse mode change string(s)
	 */
    for (p = &parv[2]; p && *p; p++ )
	for (m = *p; *m; m++) {
            if (*m == '+') {
                what = MODE_ADD;
		continue;
            } else if (*m == '-') {
                what = MODE_DEL;
		continue;
            } else if (*m == ' '|| *m == '\n' || *m == '\r' || *m == '\t' ) {
            /* we may not get these,
    	     * but they shouldnt be in default
             */
		continue;
            } else if (*m == 's') {
            /* Don't even think to set +S whoever you are ;) */
		continue;
            } else if (*m == 'x') {
            /* No +x if restricted */
		if (IsRMode(sptr)
#ifdef NO_DIRECT_VHOST
			|| cptr == NULL
#endif
				)
		    continue;
            }  
	    else if (*m == 'a' ) {
            /* fall through case */
            /* users should use the AWAY message */
        	if (cptr && !IsServer(cptr))
            	    continue;
        	if (what == MODE_DEL && sptr->user->away) {
            	    istat.is_away--;
            	    istat.is_awaymem -= (strlen(sptr->user->away) + 1);
            	    MyFree(sptr->user->away);
            	    sptr->user->away = NULL;
#ifdef  USE_SERVICES
            	    check_services_butone(SERVICE_WANT_AWAY,
                            sptr->user->server, sptr,
                            ":%s AWAY", parv[0]);
#endif
        	}
#ifdef  USE_SERVICES
        	if (what == MODE_ADD)
            	    check_services_butone(SERVICE_WANT_AWAY,
                    	    sptr->user->server, sptr,
                    	    ":%s AWAY :", parv[0]);
#endif
            }	
            for (s = user_modes; (flag = *s); s += 2)
                if (*m == (char)(*(s + 1))) {
                    if (what == MODE_ADD)
                        sptr->user->flags |= flag;
                    else
                        sptr->user->flags &= ~flag;
            	    penalty += 1;
                    break;
                }
            if (flag == 0 && MyConnect(sptr))
                sendto_one(sptr, err_str(
            	    ERR_UMODEUNKNOWNFLAG, parv[0]), *m);
    }
	/*
	 * stop users making themselves operators too easily
	 */
	if (cptr)
	    {
		if (!(setflags & FLAGS_OPER) && IsOper(sptr) &&
		    !IsServer(cptr))
			ClearOper(sptr);

		if (!(setflags & FLAGS_IDENTIFIED) && !IsServer(cptr))
			ClearIdentified(sptr);
#ifdef NO_DIRECT_VHOST
		if (!(setflags & FLAGS_VHOST) && HasVHost(sptr) &&
		    !(IsServer(cptr) || IsMe(cptr) || IsOper(sptr)))
			ClearVHost(sptr);
#endif
#ifdef WALLOPS_TO_CHANNEL
		if (!(setflags & FLAGS_WALLOP) && 
		    (sptr->user->flags & FLAGS_WALLOP) &&
		    !(IsServer(cptr) || IsMe(cptr)))
		    {
			if (IsOper(sptr))
			    sendto_one(sptr, err_str(ERR_NOWALLOP, parv[0]));
			ClearWallops(sptr);
		    }
#else
		if (!(setflags & FLAGS_WALLOP) && 
		    (sptr->user->flags & FLAGS_WALLOP) &&
		    !(IsServer(cptr) || IsMe(cptr) || IsOper(sptr)))
		    {
			ClearWallops(sptr);
		    }

#endif
		if (!(setflags & FLAGS_LOCOP) && IsLocOp(sptr))
			sptr->user->flags &= ~FLAGS_LOCOP;
		if ((setflags & FLAGS_RESTRICTED) &&
		    !(IsServer(cptr) || IsMe(cptr)) &&
		    !(sptr->user->flags & FLAGS_RESTRICTED))
		    {
			sendto_one(sptr, err_str(ERR_RESTRICTED, parv[0]));
			SetRestricted(sptr);
			/* Can't return; here since it could mess counters */
		    }
		if (!(setflags & FLAGS_RMODE) && IsRMode(sptr))
		    {
			ClearVHost(sptr);
		    }
		if ((setflags & FLAGS_RMODE) &&
		    !(IsRMode(sptr)))
		    {
			do_restrict(sptr);
		    }		
		if ((setflags & (FLAGS_OPER|FLAGS_LOCOP)) && !IsAnOper(sptr) &&
		    MyConnect(sptr))
			det_confs_butmask(sptr, CONF_CLIENT);

		/*
		 * compare new flags with old flags and send string which
		 * will cause servers to update correctly.
		 */
		if (!IsInvisible(sptr) && (setflags & FLAGS_INVISIBLE))
		    {
			istat.is_user[1]--;
			istat.is_user[0]++;
		    }
		if (IsInvisible(sptr) && !(setflags & FLAGS_INVISIBLE))
		    {
			istat.is_user[1]++;
			istat.is_user[0]--;
		    }
		if (IsMe(cptr))	/* bump ME out to let RMODE work  -erra */
			cptr = sptr;
		send_umode_out(cptr, sptr, setflags);
	    }

	/* update counters */	   
	if (IsOper(sptr) && !(setflags & FLAGS_OPER))
	    {
		istat.is_oper++;
#ifdef	USE_SERVICES
		check_services_butone(SERVICE_WANT_OPER, sptr->user->server, 
				      sptr, ":%s MODE %s :+o", parv[0],
				      parv[0]);
#endif
	    }
	else if (!IsOper(sptr) && (setflags & FLAGS_OPER))
	    {
		istat.is_oper--;
#ifdef	USE_SERVICES
		check_services_butone(SERVICE_WANT_OPER, sptr->user->server,
				      sptr, ":%s MODE %s :-o", parv[0],
				      parv[0]);
#endif
	    }
	else if (MyConnect(sptr) && !IsLocOp(sptr) && (setflags & FLAGS_LOCOP))
	    {
		istat.is_oper--;
#ifdef USE_SERVICES
		check_services_butone(SERVICE_WANT_OPER, sptr->user->server,
				      sptr, ":%s MODE %s :-O", parv[0],     
				      parv[0]);
#endif                               
	    }                         

	return penalty;
}
	
/*
 * send the MODE string for user (user) to connection cptr
 * -avalon
 */
void	send_umode(cptr, sptr, old, sendmask, umode_buf)
aClient *cptr, *sptr;
int	old, sendmask;
char	*umode_buf;
{
	Reg	int	*s, flag;
	Reg	char	*m;
	int	what = MODE_NULL;

	if (!sptr->user)
		return;
	/*
	 * build a string in umode_buf to represent the change in the user's
	 * mode between the new (sptr->flag) and 'old'.
	 */
	m = umode_buf;
	*m = '\0';
	for (s = user_modes; (flag = *s); s += 2)
	    {
		if (MyClient(sptr) && !(flag & sendmask))
			continue;
		if ((flag & old) && !(sptr->user->flags & flag))
		    {
			if (what == MODE_DEL)
				*m++ = *(s+1);
			else
			    {
				what = MODE_DEL;
				*m++ = '-';
				*m++ = *(s+1);
			    }
		    }
		else if (!(flag & old) && (sptr->user->flags & flag))
		    {
			if (what == MODE_ADD)
				*m++ = *(s+1);
			else
			    {
				what = MODE_ADD;
				*m++ = '+';
				*m++ = *(s+1);
			    }
		    }
	    }
	*m = '\0';
	if (*umode_buf && cptr)
		sendto_one(cptr, ":%s MODE %s :%s",
			   sptr->name, sptr->name, umode_buf);
}

/*
 * added Sat Jul 25 07:30:42 EST 1992
 */
void	send_umode_out(cptr, sptr, old)
aClient *cptr, *sptr;
int	old;
{
	Reg	int	i;
	Reg	aClient	*acptr;

	/* just build modeline first */
	send_umode(NULL, sptr, old, SEND_UMODES, buf);

	if (*buf)
	{
		Reg	char *old_modes = mystrdup(buf);

		/* cut off new modes for old servers */
		for (i = 0; old_modes[i]; i++)
			if (old_modes[i] == 'I' || old_modes[i] == 'R')
			{
				if (i == 1)
					*old_modes = '\0';
				else
					old_modes[i] = '\0';

				break;
			}

		for (i = fdas.highest; i >= 0; i--)
		    {
			if (!(acptr = local[fdas.fd[i]]) || !IsServer(acptr))
				continue;
			if (acptr == cptr || acptr == sptr)
				continue;

			if (acptr->serv->version & SV_RUSNET2)
				sendto_one(acptr, ":%s MODE %s :%s",
					   sptr->name, sptr->name, buf);
			else if (*old_modes)
				sendto_one(acptr, ":%s MODE %s :%s",
					   sptr->name, sptr->name, old_modes);
		    }

		free(old_modes);
	}

	if (cptr && MyClient(cptr))
		send_umode(cptr, sptr, old, ALL_UMODES, buf);
#ifdef USE_SERVICES
	/* buf contains all modes for local users, and iow only for remotes */
	if (*buf)
		check_services_butone(SERVICE_WANT_UMODE, NULL, sptr,
				      ":%s MODE %s :%s", sptr->name,
				      sptr->name, buf);
#endif
}
