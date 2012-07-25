/************************************************************************
 *   IRC - Internet Relay Chat, ircd/ircd.c
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
#define IRCD_C
#include "s_externs.h"
#undef IRCD_C

#include <locale.h>

#ifdef USE_OLD8BIT

char	*rusnetfile = RUSNETCONF_PATH;
#endif

aClient me;			/* That's me */
aClient *client = &me;		/* Pointer to beginning of Client list */

static	void	/*open_debugfile(), */io_loop();

istat_t	istat;
char	**myargv;
int	rehashed = 0;
int	portnum = -1;		    /* Server port number, listening this */
char	*configfile = IRCDCONF_PATH;	/* Server configuration file */
#ifndef USE_OLD8BIT
char	*config_charset = CHARSET_8BIT;	/* Default charset for config & motd */
#endif
int	debuglevel = -1;		/* Server debug level */
int	bootopt = BOOT_PROT|BOOT_STRICTPROT;	/* Server boot option flags */
int     serverbooting = 1;
char	debugmode[4] = "";		/*  -"-    -"-   -"-   -"- */
char	*sbrk0;				/* initial sbrk(0) */
char	*tunefile = IRCDTUNE_PATH;
static volatile int restart_iauth = 0;
int 	dorehash = 0;
int	dorestart = 0;
#ifdef USE_SSL
char	*strerror_ssl = NULL;
#endif
#ifdef DELAY_CLOSE
time_t	nextdelayclose = 0;	/* time for next delayed close */
#endif
time_t	nextconnect = 1;	/* time for next try_connections call */
time_t	nextgarbage = 1;        /* time for next collect_channel_garbage call*/
time_t	nextping = 1;		/* same as above for check_pings() */
time_t	nextdnscheck = 0;	/* next time to poll dns to force timeouts */
time_t	nextexpire = 1;		/* next expire run on the dns cache */
time_t	nextiarestart = 1;	/* next time to check if iauth is alive */
time_t	nextisrestart = 1;	/* next time to check if iserv is alive */
time_t  nextlockscheck = 1;	/* next time to expire NDELAY locks -kmale */
time_t  nextcmapscheck = 1;	/* next time to expire collision maps -erra */
unsigned long invincible;	/* prepare crc for SERVICES_SERV */

aClient *ListenerLL = NULL;     /* Listeners linked list */

void	server_reboot(mesg)
char	*mesg;
{
	Reg	int	i;
	sendto_flag(SCH_NOTICE, "Restarting server because: %s (%u)", mesg,
		    (u_int)((char *)sbrk((size_t)0)-sbrk0));

	Debug((DEBUG_NOTICE,"Restarting server..."));
	flush_connections(me.fd);
	/*
	** fd 0 must be 'preserved' if either the -d or -i options have
	** been passed to us before restarting.
	*/
#ifdef LOG_EVENTS
#ifdef USE_SYSLOG
	(void)closelog();
#endif
	log_closeall();
#endif
	for (i = 3; i < MAXCONNECTIONS; i++)
		(void)close(i);
	if (!(bootopt & BOOT_TTY)) {
	    if (!(bootopt & BOOT_DEBUG))
	    {
	    	(void)close(2);
	    }
    	    (void)close(1);
	}
	ircd_writetune(tunefile);
	if (!(bootopt & (BOOT_INETD)))
	    {
		int	time_slave = timeofday;

		(void)close(0);
		/* Have to wait for our zombies --skold */
		Debug((DEBUG_DEBUG, "Waiting for slaves:"));

		if (wait(NULL) > -1)
		{
			Debug((DEBUG_DEBUG, "Some slaves were alive... But "
				"since we are here, they are already dead"));
		}
		Debug((DEBUG_DEBUG, "Time waited: %d secs", timeofday - time_slave));			   
		(void)execv(IRCD_PATH, myargv);
#ifdef USE_SYSLOG
		/* Have to reopen since it has been closed above */
		openlog(mybasename(myargv[0]), LOG_PID|LOG_NDELAY, LOG_FACILITY);
		syslog(LOG_CRIT, "execv(%s,%s) failed: %m\n", IRCD_PATH,
		       myargv[0]);
		closelog();
#endif
		Debug((DEBUG_FATAL,"Couldn't restart server: %s",
		       strerror(errno)));
	    }
	exit(-1);
}


#if 0
/*
** try_connections
**
**	Scan through configuration and try new connections.
**	Returns the calendar time when the next call to this
**	function should be made latest. (No harm done if this
**	is called earlier or later...)
*/
static	time_t	try_connections_old(time_t currenttime)
{
	static	time_t	lastsort = 0;
	Reg	aConfItem *aconf;
	Reg	aClient *cptr;
	aConfItem **pconf;
	int	confrq;
	time_t	next = 0;
	aClass	*cltmp;
	aConfItem *con_conf = NULL;
	double	f, f2;
	aCPing	*cp;

	Debug((DEBUG_NOTICE,"Connection check at   : %s",
		myctime(currenttime)));
	for (aconf = conf; aconf; aconf = aconf->next )
	    {
		/* Also when already connecting! (update holdtimes) --SRB */
		if (!(aconf->status & (CONF_CONNECT_SERVER|CONF_ZCONNECT_SERVER)))
			continue;
		/*
		** Skip this entry if the use of it is still on hold until
		** future. Otherwise handle this entry (and set it on hold
		** until next time). Will reset only hold times, if already
		** made one successfull connection... [this algorithm is
		** a bit fuzzy... -- msa >;) ]
		*/
		if ((aconf->hold > currenttime))
		    {
			if ((next > aconf->hold) || (next == 0))
				next = aconf->hold;
			continue;
		    }
		send_ping(aconf);
		if (aconf->port <= 0)
			continue;

		cltmp = Class(aconf);
		confrq = get_con_freq(cltmp);
		aconf->hold = currenttime + confrq;
		/*
		** Found a CONNECT config with port specified, scan clients
		** and see if this server is already connected?
		*/
		cptr = find_name(aconf->name, (aClient *)NULL);
		if (!cptr)
			cptr = find_mask(aconf->name, (aClient *)NULL);
		/*
		** It is not connected, scan clients and see if any matches
		** a D(eny) line.
		*/
		if (find_denied(aconf->name, Class(cltmp)))
			continue;
		/* We have a candidate, let's see if it could be the best. */
		if (!cptr && (Links(cltmp) < MaxLinks(cltmp)) &&
		    (!con_conf || con_conf->localpref < aconf->localpref ||
		     (con_conf->pref > aconf->pref && aconf->pref >= 0) ||
		     (con_conf->pref == -1 &&
		      Class(cltmp) > ConfClass(con_conf))))
			con_conf = aconf;
		if ((next > aconf->hold) || (next == 0))
			next = aconf->hold;
	    }
	if (con_conf)
	    {
		if (con_conf->next)  /* are we already last? */
		    {
			for (pconf = &conf; (aconf = *pconf);
			     pconf = &(aconf->next))
				/* put the current one at the end and
				 * make sure we try all connections
				 */
				if (aconf == con_conf)
					*pconf = aconf->next;
			(*pconf = con_conf)->next = 0;
		    }
		if (connect_server(con_conf, (aClient *)NULL,
				   (struct hostent *)NULL) == 0)
		{
			sendto_flag(SCH_NOTICE,
				    "Connection to %s[%s] activated.",
				    con_conf->name, con_conf->host);
		}
	    }
	Debug((DEBUG_NOTICE,"Next connection check : %s", myctime(next)));
	/*
	 * calculate preference value based on accumulated stats.
	 */
	if (!lastsort || lastsort < currenttime)
	    {
		for (aconf = conf; aconf; aconf = aconf->next)
			if (!(cp = aconf->ping) || !cp->seq || !cp->recvd)
				aconf->pref = -1;
			else
			    {
				f = (double)cp->recvd / (double)cp->seq;
				f2 = pow(f, (double)20.0);
				if (f2 < (double)0.001)
					f = (double)0.001;
				else
					f = f2;
				f2 = (double)cp->ping / (double)cp->recvd;
				f = f2 / f;
				if (f > 100000.0)
					f = 100000.0;
				aconf->pref = (u_int) (f * (double)100.0);
			    }
		lastsort = currenttime + 60;
	    }
	return (next);
}
#endif

/*
** try_connections
**
**	Scan through configuration and try new connections.
**	Returns the calendar time when the next call to this
**	function should be made latest. (No harm done if this
**	is called earlier or later...)
*/
static	time_t	try_connections(time_t currenttime)
{
	Reg	aConfItem *aconf;
	Reg	aClient *cptr;
	aConfItem **pconf;
	int	confrq;
	time_t	next = 0;
	aClass	*cltmp;
	aConfItem *con_conf = NULL;
	int	allheld = 1;
#ifdef DISABLE_DOUBLE_CONNECTS
	int	i;
#endif

	Debug((DEBUG_NOTICE,"Connection check at   : %s",
		myctime(currenttime)));
	for (aconf = conf; aconf; aconf = aconf->next )
	{
		/* not a C-line */
		if (!(aconf->status & (CONF_CONNECT_SERVER|CONF_ZCONNECT_SERVER)))
			continue;

		/* not a candidate for AC */
		if (aconf->port <= 0)
			continue;

		/* minimize next to lowest hold time of all AC-able C-lines */
		if (next > aconf->hold || next == 0)
			next = aconf->hold;

		/* skip conf if the use of it is on hold until future. */
		if (aconf->hold > currenttime)
			continue;

		/* at least one candidate not held for future, good */
		allheld = 0;

		cltmp = Class(aconf);
		/* see if another link in this conf is allowed */
		if (Links(cltmp) >= MaxLinks(cltmp))
			continue;
		
		/* next possible check after connfreq secs for this C-line */
		confrq = get_con_freq(cltmp);
		aconf->hold = currenttime + confrq;

		/* is this server already connected? */
		cptr = find_name(aconf->name, (aClient *)NULL);
		if (!cptr)
			cptr = find_mask(aconf->name, (aClient *)NULL);

		/* matching client already exists, no AC to it */
		if (cptr)
			continue;

		/* no such server, check D-lines */
		if (find_denied(aconf->name, Class(cltmp)))
			continue;

#ifdef DISABLE_DOUBLE_CONNECTS
		/* Much better would be traversing only unknown
		** connections, but this requires another global
		** variable, adding and removing from there in
		** proper places etc. Some day. --B. */
		for (i = highest_fd; i >= 0; i--)
		{
			if (!(cptr = local[i]) ||
				cptr->status > STAT_UNKNOWN)
			{
				continue;
			}
			/* an unknown traveller we have */
			if (
#ifndef INET6
				cptr->ip.s_addr == aconf->ipnum.s_addr
#else
				!memcmp(cptr->ip.s6_addr,
					aconf->ipnum.s6_addr, 16)
#endif
			)
			{
				/* IP the same. Coincidence? Maybe.
				** Do not cause havoc with double connect. */
				break;
			}
			cptr = NULL;
		}
		if (cptr)
		{
			sendto_flag(SCH_SERVER, "Autoconnect to %s postponed", aconf->name);
			continue;
		}
#endif
		/* we have a candidate! */

		/* choose the best. */
		if (!con_conf || (con_conf->localpref < aconf->localpref) ||
		     (con_conf->pref > aconf->pref && aconf->pref >= 0) ||
		     (con_conf->pref == -1 &&
		      Class(cltmp) > ConfClass(con_conf)))
		{
			con_conf = aconf;
		}
		/* above is my doubt: if we always choose best connection
		** and it always fails connecting, we may never try another,
		** even "worse"; what shall we do? --Beeth */
	}
	if (con_conf)
	{
		if (con_conf->next)  /* are we already last? */
		{
			for (pconf = &conf; (aconf = *pconf);
			     pconf = &(aconf->next))
				/* put the current one at the end and
				 * make sure we try all connections
				 */
				if (aconf == con_conf)
					*pconf = aconf->next;
			(*pconf = con_conf)->next = 0;
		}

		/* "Penalty" for being the best, so in next call of
		 * try_connections() other servers have chance. --B. */
		con_conf->hold += get_con_freq(Class(con_conf));

		if (connect_server(con_conf, (aClient *)NULL,
				   (struct hostent *)NULL) == 0)
		{
			sendto_flag(SCH_NOTICE,
				    "Connection to %s[%s] activated.",
				    con_conf->name, con_conf->host);
		}
	}
	else
	if (allheld == 0)	/* disable AC only when some C: got checked */
	{
		/* No suitable conf for AC was found, so why bother checking
		** again? If some server quits, it'd get reenabled --B. */
		next = 0;
	}
	Debug((DEBUG_NOTICE,"Next connection check : %s", myctime(next)));
	return (next);
}

/*
 * calculate preference value based on accumulated stats.
 */
time_t calculate_preference(time_t currenttime)
{
	aConfItem *aconf;
	aCPing	*cp;
	double	f, f2;

	for (aconf = conf; aconf; aconf = aconf->next)
	{
		/* not a C-line */
		if (!(aconf->status & (CONF_CONNECT_SERVER|CONF_ZCONNECT_SERVER)))
			continue;

		/* not a candidate for AC */
		if (aconf->port <= 0)
			continue;

		/* send (udp) pings for all AC-able C-lines, we'll use it to
		** calculate preferences */
		send_ping(aconf);

		if (!(cp = aconf->ping) || !cp->seq || !cp->recvd)
		{
			aconf->pref = -1;
		}
		else
		{
			f = (double)cp->recvd / (double)cp->seq;
			f2 = pow(f, (double)20.0);
			if (f2 < (double)0.001)
				f = (double)0.001;
			else
				f = f2;
			f2 = (double)cp->ping / (double)cp->recvd;
			f = f2 / f;
			if (f > 100000.0)
				f = 100000.0;
			aconf->pref = (u_int) (f * (double)100.0);
		}
	}
	return currenttime + 60;
}

static	time_t	check_pings(currenttime)
time_t	currenttime;
{
	static	time_t	lkill = 0;
	Reg	aClient	*cptr;
	Reg	int	kflag = 0, rflag = 0;
	int	ping = 0, i;
	time_t	oldest = 0, timeout;
	char	*reason;

	for (i = highest_fd; i >= 0; i--)
	    {
		if (!(cptr = local[i]) || IsListener(cptr))
			continue;

		if (rehashed)
		    {
			if (IsPerson(cptr))
			    {
				kflag = check_tlines(cptr, rehashed, &reason,
						cptr->name, &kconf, NULL);
				if (!kflag)
	    			    rflag = check_tlines(cptr, rehashed, &reason,
						cptr->name, &rconf, NULL);
				if (rflag)
				{
				    Debug((DEBUG_DEBUG,
					"Found R-line for user %s!%s@%s",
					cptr->name, cptr->user->username,
							cptr->sockhost));
				}
			    }
			else
			    {
				kflag = rflag = 0;
				reason = NULL;
			    }
		    }

		if (rflag && IsPerson(cptr) && (!IsRMode(cptr)))
		    {
			int old = (cptr->user->flags & ALL_UMODES);

			do_restrict(cptr);
			send_umode_out(cptr, cptr, old);			
			Debug((DEBUG_DEBUG, "R-Line active for %s!%s@%s",
				cptr->name, cptr->user->username,
							cptr->sockhost));
			sendto_flag(SCH_NOTICE, "R line active for %s",
					    get_client_name(cptr, FALSE));
		    }

		ping = IsRegistered(cptr) ? get_client_ping(cptr) :
					    ACCEPTTIMEOUT;

		Debug((DEBUG_DEBUG, "c(%s) %d p %d k %d R %d a %d",
			cptr->name, cptr->status, ping, kflag, rflag,
			currenttime - cptr->lasttime));
		/*
		 * Ok, so goto's are ugly and can be avoided here but this code
		 * is already indented enough so I think its justified. -avalon
		 */
		if (!kflag && IsRegistered(cptr) &&
		    (ping >= currenttime - cptr->lasttime))
			goto ping_timeout;
		/*
		 * If the server hasnt talked to us in 2*ping seconds
		 * and it has a ping time, then close its connection.
		 * If the client is a user and a KILL line was found
		 * to be active, close this connection too.
		 */
		if (kflag || 
		    ((currenttime - cptr->lasttime) >= (2 * ping) &&
		     (cptr->flags & FLAGS_PINGSENT)) ||
		    (!IsRegistered(cptr) &&
		     (currenttime - cptr->firsttime) >= ping))
		    {
			if (!IsRegistered(cptr) && 
			    (DoingDNS(cptr) || DoingAuth(cptr) ||
			     DoingXAuth(cptr)))
			    {
				if (cptr->authfd >= 0)
				    {
					(void)close(cptr->authfd);
					cptr->authfd = -1;
					cptr->count = 0;
					*cptr->buffer = '\0';
				    }
				Debug((DEBUG_NOTICE, "%s/%c%s timeout %s",
				       (DoingDNS(cptr)) ? "DNS" : "dns",
				       (DoingXAuth(cptr)) ? 'X' : 'x',
				       (DoingAuth(cptr)) ? "AUTH" : "auth",
				       get_client_name(cptr,TRUE)));
				del_queries((char *)cptr);
				ClearAuth(cptr);
#if defined(USE_IAUTH)
				if (DoingDNS(cptr) || DoingXAuth(cptr))
				    {
					if (DoingDNS(cptr) &&
					    (iauth_options & XOPT_EXTWAIT))
					    {
						/* iauth wants more time */
						sendto_iauth("%d d", cptr->fd);
						ClearDNS(cptr);
						cptr->lasttime = currenttime;
						continue;
					    }
					if (DoingXAuth(cptr) &&
					    (iauth_options & XOPT_NOTIMEOUT))
					    {
						cptr->exitc = EXITC_AUTHTOUT;
						sendto_iauth("%d T", cptr->fd);
						ereject_user(cptr, 'T',
						     "Authentication Timeout");
						continue;
					    }
					sendto_iauth("%d T", cptr->fd);
					SetDoneXAuth(cptr);
				    }
#endif
				ClearDNS(cptr);
				ClearXAuth(cptr);
				ClearWXAuth(cptr);
				cptr->firsttime = currenttime;
				cptr->lasttime = currenttime;
				continue;
			    }
			if (IsServer(cptr) || IsConnecting(cptr) ||
			    IsHandshake(cptr))
				sendto_flag(SCH_NOTICE,
					    "No response from %s closing link",
					    get_client_name(cptr, FALSE));
			/*
			 * this is used for KILL lines with time restrictions
			 * on them - send a message to the user being killed
			 * first.
			 */
			if (kflag && IsPerson(cptr))
			    {
				char buf[300];

				sendto_flag(SCH_NOTICE,
					    "Kill line active for %s",
					    get_client_name(cptr, FALSE));
				cptr->exitc = EXITC_KLINE;
				if (!BadPtr(reason))
					sprintf(buf, "Kill line active: %.256s",
						reason);
				(void)exit_client(cptr, cptr, &me, (reason) ?
						  buf : "Kill line active");
			    }
			else
			    {
				cptr->exitc = EXITC_PING;
				(void)exit_client(cptr, cptr, &me,
						  "Ping timeout");
			    }
			continue;
		    }
		else if (IsRegistered(cptr) &&
			 (cptr->flags & FLAGS_PINGSENT) == 0)
		    {
			/*
			 * if we havent PINGed the connection and we havent
			 * heard from it in a while, PING it to make sure
			 * it is still alive.
			 */
			cptr->flags |= FLAGS_PINGSENT;
			/* not nice but does the job */
			cptr->lasttime = currenttime - ping;
			sendto_one(cptr, "PING :%s", me.name);
		    }
ping_timeout:
		timeout = cptr->lasttime + ping;
		while (timeout <= currenttime)
			timeout += ping;
		if (timeout < oldest || !oldest)
			oldest = timeout;
	    }
	if (currenttime - lkill > 60)
		lkill = currenttime;
	if (!oldest || oldest < currenttime)
		oldest = currenttime + PINGFREQUENCY;
	if (oldest < currenttime + 2)
		oldest += 2;
	Debug((DEBUG_NOTICE,"Next check_ping() call at: %s, %d %d %d",
		myctime(oldest), ping, oldest, currenttime));
	return (oldest);
}

/*
** Check expired NDELAY locks -kmale
*/
static	time_t	check_locks(time)
time_t	time;
{
	remove_locks(time-DELAYCHASETIMELIMIT);
	return(time+KILLCHASETIMELIMIT);
}

/*
** Check expired collision maps  -erra
*/
static	time_t	check_cmaps(time)
time_t	time;
{
	expire_collision_map(time);
	return (time + CMAPCHECKEVERY);
}

static	void	setup_me(mp)
aClient	*mp;
{
	struct	passwd	*p;

	p = getpwuid(getuid());
	strncpyzt(mp->username, (p) ? p->pw_name : "unknown",
		  sizeof(mp->username));
	(void)get_my_name(mp, mp->sockhost, sizeof(mp->sockhost)-1);

	/* Setup hostp - fake record to resolve localhost. -Toor */
	mp->hostp = (struct hostent *)MyMalloc(sizeof(struct hostent));
	mp->hostp->h_name = MyMalloc(strlen(me.sockhost)+1);
	strcpy(mp->hostp->h_name, mp->sockhost);
	mp->hostp->h_aliases = (char **)MyMalloc(sizeof(char *));
	*mp->hostp->h_aliases = NULL;
	mp->hostp->h_addrtype = AFINET;
	mp->hostp->h_length = 
#ifdef	INET6
				IN6ADDRSZ;
#else
				sizeof(long);
#endif
	mp->hostp->h_addr_list = (char **)MyMalloc(2*sizeof(char *));
#ifdef	INET6
	mp->hostp->h_addr_list[0] = (char *)&in6addr_loopback;
#else
	mp->hostp->h_addr_list[0] = (void *)MyMalloc(mp->hostp->h_length);
	*(long *)(mp->hostp->h_addr_list[0]) = IN_LOOPBACKNET;
#endif
	mp->hostp->h_addr_list[1] = NULL ;

	if (mp->name[0] == '\0')
		strncpyzt(mp->name, mp->sockhost, sizeof(mp->name));
	if (me.info == DefInfo)
		me.info = mystrdup("IRCers United");
	mp->lasttime = mp->since = mp->firsttime = time(NULL);
	mp->hopcount = 0;
	mp->authfd = -1;
	mp->auth = mp->username;
	mp->confs = NULL;
	mp->flags = 0;
	mp->acpt = mp->from = mp;
	mp->next = NULL;
	mp->user = NULL;
	mp->fd = -1;
	SetMe(mp);
	(void) make_server(mp);
	mp->serv->snum = find_server_num (ME);
	(void) make_user(mp);
	istat.is_users++;	/* here, cptr->next is NULL, see make_user() */
	mp->user->flags |= FLAGS_OPER;
	mp->serv->up = mp;
	mp->serv->crc = gen_crc(mp->name);
	mp->user->server = find_server_string(mp->serv->snum);
	strncpyzt(mp->user->username, (p) ? p->pw_name : "unknown",
		  sizeof(mp->user->username));
	(void) strcpy(mp->user->host, mp->name);

	(void)add_to_client_hash_table(mp->name, mp);

	setup_server_channels(mp);
}

/*
** bad_command
**	This is called when the commandline is not acceptable.
**	Give error message and exit without starting anything.
*/
static	int	bad_command()
{
  (void)printf(
	 "Usage: ircd [-a] [-b] [-c]%s%s [-h servername] [-q] [-o] [-i] [-T tunefile] [-p (strict|on|off)] [-s] [-v] %s\n",
#ifdef CMDLINE_CONFIG
	 " [-f config]",
#else
	 "",
#endif
#ifndef USE_OLD8BIT
	" [-e config_charset]",
#else
	 "",
#endif
#ifdef DEBUGMODE
	 " [-x loglevel] [-t]"
#else
	 ""
#endif
	 );
  (void)printf("Server not started\n\n");
  exit(-1);
}

int	main(argc, argv)
int	argc;
char	*argv[];
{
	uid_t	uid, euid;

	sbrk0 = (char *)sbrk((size_t)0);
	uid = getuid();
	euid = geteuid();

#ifdef	CHROOTDIR
	ircd_res_init();
	if (chdir(ROOT_PATH)!=0)
	{
		perror("chdir");
		(void)fprintf(stderr,"%s: Cannot chdir: %s.\n", IRCD_PATH,
			ROOT_PATH);
		exit(5);
	}
	if (chroot(ROOT_PATH)!=0)
	    {
		perror("chroot");
		(void)fprintf(stderr,"%s: Cannot chroot: %s.\n", IRCD_PATH,
			      ROOT_PATH);
		exit(5);
	    }
#endif /*CHROOTDIR*/

#ifdef	ZIP_LINKS
	if (zlib_version[0] == '0')
	    {
		fprintf(stderr, "zlib version 1.0 or higher required\n");
		exit(1);
	    }
	if (zlib_version[0] != ZLIB_VERSION[0])
	    {
        	fprintf(stderr, "incompatible zlib version\n");
		exit(1);
	    }
	if (strcmp(zlib_version, ZLIB_VERSION) != 0)
	    {
		fprintf(stderr, "warning: different zlib version\n");
	    }
#endif

	myargv = argv;
	(void)umask(077);                /* better safe than sorry --SRB */
	bzero((char *)&me, sizeof(me));

	version = make_version();	/* Generate readable version string */
	/*
	** All command line parameters have the syntax "-fstring"
	** or "-f string" (e.g. the space is optional). String may
	** be empty. Flag characters cannot be concatenated (like
	** "-fxyz"), it would conflict with the form "-fstring".
	*/
	while (--argc > 0 && (*++argv)[0] == '-')
	    {
		char	*p = argv[0]+1;
		int	flag = *p++;

		if (flag == '\0' || *p == '\0')
		{
			if (argc > 1 && argv[1][0] != '-')
			    {
				p = *++argv;
				argc -= 1;
			    }
			else
				p = "";
		}

		switch (flag)
		    {
                    case 'a':
			bootopt |= BOOT_AUTODIE;
			break;
		    case 'b':
			bootopt |= BOOT_BADTUNE;
			break;
		    case 'q':
			bootopt |= BOOT_QUICK;
			break;
#ifdef CMDLINE_CONFIG
		    case 'f':
                        (void)setuid((uid_t)uid);
			configfile = p;
			break;
#endif
#ifndef USE_OLD8BIT
		    case 'e':
			{
			conversion_t *conv;
			if ((conv = conv_get_conversion((config_charset = p))))
			    conv_free_conversion(conv);
			else
			    bad_command();
			break;
			}
#endif
		    case 'h':
			if (*p == '\0')
				bad_command();
			strncpyzt(me.name, p, sizeof(me.name));
			break;
		    case 'i':
			bootopt |= BOOT_INETD|BOOT_AUTODIE;
		        break;
		    case 'p':
			if (!strcmp(p, "strict"))
				bootopt |= BOOT_PROT|BOOT_STRICTPROT;
			else if (!strcmp(p, "on"))
				bootopt |= BOOT_PROT;
			else if (!strcmp(p, "off"))
				bootopt &= ~(BOOT_PROT|BOOT_STRICTPROT);
			else
				bad_command();
			break;
		    case 's':
			bootopt |= BOOT_NOIAUTH;
			break;
		    case 'r':
			bootopt |= BOOT_NOISERV;
			break;
		    case 't':
#ifdef DEBUGMODE
                        (void)setuid((uid_t)uid);
#endif
			bootopt |= BOOT_TTY;
			break;
		    case 'T':
			if (*p == '\0')
				bad_command();
			tunefile = p;
			break;
		    case 'v':
			(void)printf("ircd %s %s\n\tzlib %s\n\t%s #%s\n",
				     version, serveropts,
#ifndef	ZIP_LINKS
				     "not used",
#else
				     zlib_version,
#endif
				     creation, generation);
			  exit(0);
		    case 'x':
#ifdef	DEBUGMODE
                        (void)setuid((uid_t)uid);
			debuglevel = atoi(p);
			strcat(debugmode, ".d");
			strncat(debugmode, *p ? p : "0", 1);
			bootopt |= BOOT_DEBUG;
			break;
#else
			(void)fprintf(stderr,
				"%s: DEBUGMODE must be defined for -x y\n",
				myargv[0]);
			exit(0);
#endif
		    default:
			bad_command();
		    }
	    }

	if (argc > 0)
		bad_command(); /* This exits out */

#ifndef IRC_UID
	if (!uid)
	    {
		(void)fprintf(stderr,
			"ERROR: do not run ircd as root. This is not safe.\n");
		exit(-1);
	    }

	if ((uid != euid) && !euid)
	    {
		(void)fprintf(stderr,
			"ERROR: do not run ircd setuid root. Make it setuid a\
 normal user.\n");
		exit(-1);
	    }
#endif

# if defined(IRC_UID) && defined(IRC_GID)
	if ((int)getuid() == 0)
	    {
		/* run as a specified user */
               (void)fprintf(stderr,"running ircd with uid = %d  gid = %d.\n",
                       IRC_UID,IRC_GID);

		(void)setgid(IRC_GID);
		(void)setuid(IRC_UID);
	    } 
# endif

#if defined(USE_IAUTH)
	if ((bootopt & BOOT_NOIAUTH) == 0) {
		restore_sigchld();  /* We need this if we expect valid response from wait()
				     * about iauth -X test results. (If in doubt - we do ;))  --skold
				     */
		switch (vfork())
		{
		case -1:
			fprintf(stderr, "%s: Unable to fork!", myargv[0]);
			exit(-1);
		case 0:
			close(0); close(1); close(3);
			if (execl(IAUTH_PATH, IAUTH, "-X", NULL) < 0)
				_exit(-1);
		default:
		    {
			int rc;
			
			(void)wait(&rc);
			if (rc != 0)
			    {
				fprintf(stderr,
					"%s: error: unable to find \"%s\".\n",
					myargv[0], IAUTH_PATH);
				exit(-1);
			    }
		    }
		}
	}
#endif

#if defined(USE_ISERV)
	if ((bootopt & BOOT_NOISERV) == 0) {
		restore_sigchld();  /* We need this if we expect valid response from wait()
				     * about iserv -X test results. (If in doubt - we do ;))  --skold
				     */
		switch (vfork())
		{
		case -1:
			fprintf(stderr, "%s: Unable to fork!", myargv[0]);
			exit(-1);
		case 0:
			close(0); close(1); close(3);
			if (execl(ISERV_PATH, ISERV, "-X", NULL) < 0)
			    _exit(-1);
		default:
		    {
			int rc;
			
			(void)wait(&rc);
			if (rc != 0)
			    {
				fprintf(stderr,
					"%s: error: unable to find \"%s\".\n",
					myargv[0], ISERV_PATH);
				exit(-1);
			    }
		    }
		}
	}
#endif

#ifndef USE_OLD8BIT
# ifdef LOCALE_PATH
	setenv("LOCPATH", LOCPATH, 1);
# endif
	setlocale(LC_ALL, LOCALE "." CHARSET_UNICODE); /* it's fixed for now */
	setlocale(LC_TIME, SYS_LOCALE);
	setlocale(LC_MESSAGES, SYS_LOCALE);
#endif
#if defined(USE_OLD8BIT) || defined(LOCALE_STRICT_NAMES)
	setup_validtab();
#endif
	setup_signals();

	/* didn't set debuglevel */
	/* but asked for debugging output to tty */
#ifdef DEBUMODE
	if ((debuglevel < 0) &&  (bootopt & BOOT_TTY))
	    {
		(void)fprintf(stderr,
			"you specified -t without -x. use -x <n>\n");
		exit(-1);
	    }
#endif
#ifdef USE_OLD8BIT
	initialize_rusnet(rusnetfile);
#endif
#ifdef USE_SSL
	if (!(sslable = ssl_init())) {
	    (void)fprintf(stderr,
		"SSL initialization failed.\n");
	}
#endif
	timeofday = time(NULL);
	initstats();
	ircd_readtune(tunefile);
	inithashtables();
	initlists();
	initclass();
	initwhowas();
	timeofday = time(NULL);
	open_debugfile();
	timeofday = time(NULL);
	(void)init_sys();

#ifdef USE_SYSLOG
	openlog(mybasename(myargv[0]), LOG_PID|LOG_NDELAY, LOG_FACILITY);
#endif
	timeofday = time(NULL);
	if (initconf(bootopt, 0) == -1)
	    {
		Debug((DEBUG_FATAL, "Failed in reading configuration file %s",
			configfile));
                (void)fprintf(stderr, "Couldn't open configuration file %s (%s)\n",
			configfile,strerror(errno));
		exit(-1);
	    }
	else
	    {
		aClient *acptr = NULL;
		int i;

                for (i = 0; i <= highest_fd; i++)
                    {   
                        if (!(acptr = local[i]))
                                continue;
			if (IsListener(acptr))
				break;
			acptr = NULL;
		    }
		/* exit if there is nothing to listen to */
		if (acptr == NULL && !(bootopt & BOOT_INETD))
                {
		        fprintf(stderr,
			"Fatal Error: No working P-line in ircd.conf\n");
			exit(-1);
		}
		/* Is there an M-line? */
		if (!find_me())
		{
                        fprintf(stderr,
                        "Fatal Error: No M-line in ircd.conf.\n");
                        exit(-1);
		}
#ifdef SEND_ISUPPORT
		isupport = make_isupport();     /* generate 005 numeric */
#endif
	    }

	setup_me(&me);

	motd = NULL;
	read_motd(IRCDMOTD_PATH);
	rmotd = NULL;
	read_rmotd(IRCDRMOTD_PATH);

	gen_crc32table();
	srand(timeofday ^ me.serv->crc);
	invincible = gen_crc(SERVICES_SERV);	/* prepare crc for SERVICES_SERV */
	check_class();
	ircd_writetune(tunefile);
	if (bootopt & BOOT_INETD)
	    {
		aClient	*tmp;
		aConfItem *aconf;

		tmp = make_client(NULL);

		tmp->fd = 0;
		tmp->flags = FLAGS_LISTEN;
		tmp->acpt = tmp;
		tmp->from = tmp;
	        tmp->firsttime = time(NULL);

                SetMe(tmp);

                (void)strcpy(tmp->name, "*");

                if (inetport(tmp, 0, "0.0.0.0", 0, 1))
                        tmp->fd = -1;
		if (tmp->fd == 0) 
		    {
			aconf = make_conf();
			aconf->status = CONF_LISTEN_PORT;
			aconf->clients++;
	                aconf->next = conf;
        	        conf = aconf;

                        tmp->confs = make_link();
        	        tmp->confs->next = NULL;
	               	tmp->confs->value.aconf = aconf;
	                add_fd(tmp->fd, &fdas);
	                add_fd(tmp->fd, &fdall);
	                set_non_blocking(tmp->fd, tmp);
		    }
		else
		    exit(5);
	    }

	Debug((DEBUG_NOTICE,"Server ready..."));
#ifdef USE_SYSLOG
	syslog(LOG_NOTICE, "Server Ready: v%s (%s #%s)", version, creation,
	       generation);
#endif
	timeofday = time(NULL);
        daemonize();
	serverbooting = 0;
	write_pidfile();
	dbuf_init();


	while (1)
		io_loop();
}


void	io_loop()
{
	static	time_t	delay = 0;
	int maxs = 4;

	/*
	** We only want to connect if a connection is due,
	** not every time through.  Note, if there are no
	** active C lines, this call to Tryconnections is
	** made once only; it will return 0. - avalon
	*/
	if (nextconnect && timeofday >= nextconnect)
		nextconnect = try_connections(timeofday);
#ifdef DELAY_CLOSE
	/* close all overdue delayed fds */
	if (nextdelayclose && timeofday >= nextdelayclose)
		nextdelayclose = delay_close(-1);
#endif

	/*
	** Every once in a while, hunt channel structures that
	** can be freed.
	*/
	if (timeofday >= nextgarbage)
		nextgarbage = collect_channel_garbage(timeofday);
	/*
	** DNS checks. One to timeout queries, one for cache expiries.
	*/
	if (timeofday >= nextdnscheck)
		nextdnscheck = timeout_query_list(timeofday);
	if (timeofday >= nextexpire)
		nextexpire = expire_cache(timeofday);
	/*
	** Expired NDELAY locks check. -kmale
	*/
	if (timeofday >= nextlockscheck)
		nextlockscheck = check_locks(timeofday);
	/*
	** Expired collision maps check. -erra
	*/
	if (timeofday >= nextcmapscheck)
		nextcmapscheck = check_cmaps(timeofday);
	/*
	** take the smaller of the two 'timed' event times as
	** the time of next event (stops us being late :) - avalon
	** WARNING - nextconnect can return 0!
	*/
	if (nextconnect)
		delay = MIN(nextping, nextconnect);
	else
		delay = nextping;
#ifdef DELAY_CLOSE
	if (nextdelayclose)
		delay = MIN(nextdelayclose, delay);
#endif
	delay = MIN(nextdnscheck, delay);
	delay = MIN(nextexpire, delay);
	delay -= timeofday;
	/*
	** Adjust delay to something reasonable [ad hoc values]
	** (one might think something more clever here... --msa)
	** We don't really need to check that often and as long
	** as we don't delay too long, everything should be ok.
	** waiting too long can cause things to timeout...
	** i.e. PINGS -> a disconnection :(
	** - avalon
	*/
	if (delay < 1)
		delay = 1;
	else
		delay = MIN(delay, TIMESEC);

	/*
	** First, try to drain traffic from servers (this includes listening
	** ports).  Give up, either if there's no traffic, or too many
	** iterations.
	*/
	while (maxs--)
		if (read_message(0, &fdas, 0))
			flush_fdary(&fdas);
		else
			break;

	Debug((DEBUG_DEBUG, "delay for %d", delay));
	/*
	** Second, deal with _all_ clients but only try to empty sendQ's for
	** servers.  Other clients are dealt with below..
	*/
	Debug((DEBUG_DEBUG, "try read_message(x,x,1)"));
	if (read_message(1, &fdall, 1) == 0 && delay > 1)
	    {
		Debug((DEBUG_DEBUG, "try read_message(x,x,0)"));
		/*
		** Timed out (e.g. *NO* traffic at all).
		** Try again but also check to empty sendQ's for all clients.
		*/
		(void)read_message(delay - 1, &fdall, 0);
	    }
	Debug((DEBUG_DEBUG, "try read_message(x,x,0&1) -- done"));
	timeofday = time(NULL);

	Debug((DEBUG_DEBUG ,"Got message(s)"));
	/*
	** ...perhaps should not do these loops every time,
	** but only if there is some chance of something
	** happening (but, note that conf->hold times may
	** be changed elsewhere--so precomputed next event
	** time might be too far away... (similarly with
	** ping times) --msa
	*/
	if (timeofday >= nextping)
	    {
		nextping = check_pings(timeofday);
		rehashed = 0;
	    }

	if (dorestart)
		server_reboot("Caught SIGINT");
	if (dorehash)
	    {	/* Only on signal, not on oper /rehash */
		ircd_writetune(tunefile);
		(void)rehash(1, REHASH_ALL);
		dorehash = 0;
	    }
	if (restart_iauth || timeofday >= nextiarestart)
	    {
		Debug((DEBUG_DEBUG ,"restart_iauth=%d", restart_iauth ));
		start_iauth(restart_iauth);
		restart_iauth = 0;
		nextiarestart = timeofday + 15;
	    }
	if (timeofday >= nextisrestart)
	    {
		start_iserv();
		nextisrestart = timeofday + 15;
	    }
	/*
	** Flush output buffers on all connections now if they
	** have data in them (or at least try to flush)
	** -avalon
	*/
	flush_connections(me.fd);

#ifdef	DEBUGMODE
	checklists();
#endif
}

/*
 * open_debugfile
 *
 * If the -t option is not given on the command line when the server is
 * started, all debugging output is sent to the file set by IRCDDBG_PATH.
 * Here we just open that file and make sure it is opened to fd 2 so that
 * any fprintf's to stderr also goto the logfile.  If the debuglevel is not
 * set from the command line by -x, use /dev/null as the dummy logfile as long
 * as DEBUGMODE has been defined, else don't waste the fd.
 */

void	open_debugfile(void)
{
#ifdef	DEBUGMODE
	int	fd;

	if (debuglevel >= 0)
	    {
		char *tty = ttyname(2);

		(void)printf("isatty = %d ttyname = %s (%#x)\n",
			isatty(2), tty, (unsigned long) tty);
		if (!(bootopt & BOOT_TTY)) /* leave debugging output on fd 2 */
		    {
			// (void)truncate(IRCDDBG_PATH, 0);
			if ((fd = open(IRCDDBG_PATH,O_WRONLY|O_CREAT|O_APPEND,0600))<0)
				if ((fd = open("/dev/null", O_WRONLY)) < 0)
					exit(-1);
			if (fd != 2)
			    {
				(void)dup2(fd, 2);
				(void)close(fd); 
			    }
		    }

		/* refresh terminal name */
		tty = ttyname(2);

		Debug((DEBUG_FATAL, "Debug: File <%s> Level: %d at %s",
			( (!(bootopt & BOOT_TTY)) ? IRCDDBG_PATH :
			(isatty(2) && tty) ? tty : "FD2-Pipe"),
			debuglevel, myctime(time(NULL))));
	    }
#endif
	return;
}

/*
 * Called from bigger_hash_table(), s_die(), server_reboot(),
 * main(after initializations), grow_history(), rehash(io_loop) signal.
 */
void ircd_writetune(filename)
char *filename;
{
	int fd;
	char buf[100];

	(void)truncate(filename, 0);
	if ((fd = open(filename, O_CREAT|O_WRONLY, 0600)) >= 0)
	    {
		(void)sprintf(buf, "%d\n%d\n%d\n%d\n%d\n%d\n", ww_size,
			       lk_size, _HASHSIZE, _CHANNELHASHSIZE,
			       _SERVERSIZE, poolsize);
		if (write(fd, buf, strlen(buf)) == -1)
			sendto_flag(SCH_ERROR,
				    "Failed (%d) to write tune file: %s.",
				    errno, mybasename(filename));
		else
			sendto_flag(SCH_NOTICE, "Updated %s.",
				    mybasename(filename));
		close(fd);
	    }
	else
		sendto_flag(SCH_ERROR, "Failed (%d) to open tune file: %s.",
			    errno, mybasename(filename));
}

/*
 * Called only from main() at startup.
 */
void ircd_readtune(filename)
char *filename;
{
	int fd, t_data[6];
	char buf[100];

	memset(buf, 0, sizeof(buf));
	if ((fd = open(filename, O_RDONLY)) != -1)
	    {
		read(fd, buf, 100);	/* no panic if this fails.. */
		if (sscanf(buf, "%d\n%d\n%d\n%d\n%d\n%d\n", &t_data[0],
                           &t_data[1], &t_data[2], &t_data[3],
                           &t_data[4], &t_data[5]) != 6)
		    {
			close(fd);
			if (bootopt & BOOT_BADTUNE)
				return;
			else
			    {
				fprintf(stderr,
					"ircd tune file %s: bad format\n",
					filename);
				exit(1);
			    }
		    }

		/*
		** Initiate the tune-values after successfully
		** reading the tune-file.
		*/
		ww_size = t_data[0];
		lk_size = t_data[1];
		_HASHSIZE = t_data[2];
		_CHANNELHASHSIZE = t_data[3];
		_SERVERSIZE = t_data[4];
		poolsize = t_data[5];

		/*
		** the lock array only grows if the whowas array grows,
		** I don't think it should be initialized with a lower
		** size since it will never adjust unless whowas array does.
		*/
		/*
		** For big and medium networks old lock management is
		** not so good, I've changed it. -kmale
		*/
#ifdef CONSERVATIVE_NDELAY_MALLOC
		if (lk_size < ww_size)
			lk_size = ww_size;
#else
		if (lk_size > MAXLOCKS)
			lk_size = MAXLOCKS;
		else if (lk_size < MINLOCKS)
			lk_size = MINLOCKS;
#endif
		close(fd);
	    }
}
