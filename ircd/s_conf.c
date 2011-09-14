/************************************************************************
 *   IRC - Internet Relay Chat, ircd/s_conf.c
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
 
/*
 * End of story. Mind the ChangeLog.
 */
 
/* -- avalon -- 20 Feb 1992
 * Reversed the order of the params for attach_conf().
 * detach_conf() and attach_conf() are now the same:
 * function_conf(aClient *, aConfItem *)
 */

/* -- Jto -- 20 Jun 1990
 * Added gruner's overnight fix..
 */

/* -- Jto -- 16 Jun 1990
 * Moved matches to ../common/match.c
 */

/* -- Jto -- 03 Jun 1990
 * Added Kill fixes from gruner@lan.informatik.tu-muenchen.de
 * Added jarlek's msgbase fix (I still don't understand it... -- Jto)
 */

/* -- Jto -- 13 May 1990
 * Added fixes from msa:
 * Comments and return value to init_conf()
 */

/*
 * -- Jto -- 12 May 1990
 *  Added close() into configuration file (was forgotten...)
 */


#include "os.h"
#include "s_defines.h"
#define S_CONF_C
#include "s_externs.h"
#undef S_CONF_C
#ifdef USE_OLD8BIT
extern  char    *rusnetfile;
#endif

static	int	check_time_interval(char *, char *);
static	int	lookup_confhost(aConfItem *);

#include "config_read.c"

aConfItem	*conf = NULL;
aConfItem	*kconf = NULL;
aConfItem	*econf = NULL;
#ifdef RUSNET_RLINES
aConfItem	*rconf = NULL;
#endif

/*
 * remove all conf entries from the client except those which match
 * the status field mask.
 */
void	det_confs_butmask(cptr, mask)
aClient	*cptr;
int	mask;
{
	Reg	Link	*tmp, *tmp2;

	for (tmp = cptr->confs; tmp; tmp = tmp2)
	    {
		tmp2 = tmp->next;
		if ((tmp->value.aconf->status & mask) == 0)
			(void)detach_conf(cptr, tmp->value.aconf);
	    }
}

/*
 * Match address by #IP bitmask (10.11.12.128/27)
 * Now should work for IPv6 too.
 * returns -1 on error, 0 on match, 1 when NO match.
 */
int    match_ipmask(mask, cptr)
char   *mask;
aClient *cptr;
{
	int	m = 0;
	char	*p;
	struct  IN_ADDR addr;
	char	dummy[128];
	u_long	lmask;
#ifdef	INET6
	int	j;
#endif
 
	strncpyzt(dummy, mask, sizeof(dummy));
	mask = dummy;
	if ((p = index(mask, '@')))
	{
		*p = '\0';
		if (match(mask, cptr->username))
			return 1;
		if (*(++p) == '=')
		    mask = p + 1;
		else
		    mask = p;
	}
	if ((p = index(mask, '/')) != NULL)
	{
		*p = '\0';
		
		if (sscanf(p + 1, "%d", &m) != 1)
			goto badmask;
#ifndef	INET6
		if (m < 0 || m > 32)
#else
		if (m < 0 || m > 128)
#endif
			goto badmask;
	}

	if (!m)
#ifndef	INET6
		m = 32;
	lmask = htonl((u_long)0xffffffffL << (32 - m));
	addr.s_addr = inetaddr(mask);
	return ((addr.s_addr ^ cptr->ip.s_addr) & lmask) ? 1 : 0;
#else
		m = 128;

	if (inetpton(AF_INET6, mask, (void *)addr.s6_addr) != 1)
	{
		return -1;
	}

	/* Make sure that the ipv4 notation still works. */
	if (IN6_IS_ADDR_V4MAPPED(&addr) && m < 96)
	{
		m += 96;
	}

	j = m & 0x1F;	/* number not mutliple of 32 bits */
	m >>= 5;	/* number of 32 bits */

	if (m && memcmp((void *)(addr.s6_addr), 
		(void *)(cptr->ip.s6_addr), m << 2))
		return 1;

	if (j)
	{
		lmask = htonl((u_long)0xffffffffL << (32 - j));
		if ((((u_int32_t *)(addr.s6_addr))[m] ^
			((u_int32_t *)(cptr->ip.s6_addr))[m]) & lmask)
			return 1;
	}

	return 0;
#endif
badmask:
	sendto_flag(SCH_ERROR, "Ignoring bad mask: %s", mask);
	return -1;
}

/*
 * find the first (best) I line to attach.
 */
int	attach_Iline(cptr, hp, sockhost)
aClient *cptr;
Reg	struct	hostent	*hp;
char	*sockhost;
{
	Reg	aConfItem	*aconf;
	Reg	char	*hname;
	Reg	int	i;
	static	char	uhost[HOSTLEN+USERLEN+3];
	static	char	fullname[HOSTLEN+1];

	for (aconf = conf; aconf; aconf = aconf->next)
	    {
		if ((aconf->status != CONF_CLIENT) &&
		    (aconf->status != CONF_RCLIENT))
			continue;
		if (aconf->port && aconf->port != cptr->acpt->port)
			continue;
		if (!aconf->host || !aconf->name)
			goto attach_iline;
		if (hp)
			for (i = 0, hname = hp->h_name; hname;
			     hname = hp->h_aliases[i++])
			    {
				strncpyzt(fullname, hname,
					sizeof(fullname));
				add_local_domain(fullname,
						 HOSTLEN - strlen(fullname));
				Debug((DEBUG_DNS, "a_il: %s->%s",
				      sockhost, fullname));
				if (index(aconf->name, '@'))
				    {
					(void)strcpy(uhost, cptr->username);
					(void)strcat(uhost, "@");
				    }
				else
					*uhost = '\0';
				(void)strncat(uhost, fullname,
					sizeof(uhost) - strlen(uhost));
				if (!match(aconf->name, uhost))
					goto attach_iline;
			    }

		if (index(aconf->host, '@'))
		    {
			strncpyzt(uhost, cptr->username, sizeof(uhost));
			(void)strcat(uhost, "@");
		    }
		else
			*uhost = '\0';
		(void)strncat(uhost, sockhost, sizeof(uhost) - strlen(uhost));
		if (strchr(aconf->host, '/'))		/* 1.2.3.0/24 */
		    {
			if (match_ipmask(aconf->host, cptr))
				continue;
                } else if (match(aconf->host, uhost))	/* 1.2.3.* */
			continue;
		if (*aconf->name == '\0' && hp)
		    {
			strncpyzt(uhost, hp->h_name, sizeof(uhost));
			add_local_domain(uhost, sizeof(uhost) - strlen(uhost));
		    }
attach_iline:
		if (aconf->status & CONF_RCLIENT)
			SetRestricted(cptr);

		cptr->flood = aconf->localpref;

		get_sockhost(cptr, uhost);
		if ((i = attach_conf(cptr, aconf)) < -1) {
			if (find_bounce(cptr, ConfClass(aconf), -1) == 0)
				continue;	/* disregard attach when */
		}		/* bounce server not in network  -erra */
		return i;
	    }
	find_bounce(cptr, 0, -2);
	return -2; /* used in register_user() */
}

/*
 * Find the single N line and return pointer to it (from list).
 * If more than one then return NULL pointer.
 */
aConfItem	*count_cnlines(lp)
Reg	Link	*lp;
{
	Reg	aConfItem	*aconf, *cline = NULL, *nline = NULL;

	for (; lp; lp = lp->next)
	    {
		aconf = lp->value.aconf;
		if (!(aconf->status & CONF_SERVER_MASK))
			continue;
		if ((aconf->status == CONF_CONNECT_SERVER ||
		     aconf->status == CONF_ZCONNECT_SERVER) && !cline)
			cline = aconf;
		else if (aconf->status == CONF_NOCONNECT_SERVER && !nline)
			nline = aconf;
	    }
	return nline;
}

/*
** detach_conf
**	Disassociate configuration from the client.
**      Also removes a class from the list if marked for deleting.
*/
int	detach_conf(cptr, aconf)
aClient *cptr;
aConfItem *aconf;
{
	Reg	Link	**lp, *tmp;
	aConfItem **aconf2,*aconf3;

	lp = &(cptr->confs);

	while (*lp)
	    {
		if ((*lp)->value.aconf == aconf)
		    {
			if ((aconf) && (Class(aconf)))
			    {
				if (aconf->status & CONF_CLIENT_MASK)
					if (ConfLinks(aconf) > 0)
						--ConfLinks(aconf);
       				if (ConfMaxLinks(aconf) == -1 &&
				    ConfLinks(aconf) == 0)
		 		    {
					free_class(Class(aconf));
					Class(aconf) = NULL;
				    }
			     }
			if (aconf && !--aconf->clients && IsIllegal(aconf))
			{
				/* Remove the conf entry from the Conf linked list */
				for (aconf2 = &conf; (aconf3 = *aconf2); )
				{
					if (aconf3 == aconf)
					{
						*aconf2 = aconf3->next;
						aconf3->next = NULL;
						free_conf(aconf);
					}
					else
					{
						aconf2 = &aconf3->next;
					}
				}
			}
			tmp = *lp;
			*lp = tmp->next;
			free_link(tmp);
			istat.is_conflink--;
			return 0;
		    }
		else
			lp = &((*lp)->next);
	    }
	return -1;
}

static	int	is_attached(aconf, cptr)
aConfItem *aconf;
aClient *cptr;
{
	Reg	Link	*lp;

	for (lp = cptr->confs; lp; lp = lp->next)
		if (lp->value.aconf == aconf)
			break;

	return (lp) ? 1 : 0;
}

/*
** attach_conf
**	Associate a specific configuration entry to a *local*
**	client (this is the one which used in accepting the
**	connection). Note, that this automaticly changes the
**	attachment if there was an old one...
*/
int	attach_conf(cptr, aconf)
aConfItem *aconf;
aClient *cptr;
{
	Reg	Link	*lp;

	if (is_attached(aconf, cptr))
		return 1;
	if (IsIllegal(aconf))
		return -1;
	if ((aconf->status & (CONF_LOCOP | CONF_OPERATOR | CONF_CLIENT |
			      CONF_RCLIENT)))
	    {
		if (aconf->clients >= ConfMaxLinks(aconf) &&
		    ConfMaxLinks(aconf) > 0)
			return -3;    /* Use this for printing error message */
	    }
	if ((aconf->status & (CONF_CLIENT | CONF_RCLIENT)))
	    {
		int	hcnt = 0, ucnt = 0;

		/* check on local/global limits per host and per user@host */

		/*
		** local limits first to save CPU if any is hit.
		**	host check is done on the IP address.
		**	user check is done on the IDENT reply.
		*/
		if (ConfMaxHLocal(aconf) > 0 || ConfMaxUHLocal(aconf) > 0) {
			Reg     aClient *acptr;
			Reg     int     i;

			for (i = highest_fd; i >= 0; i--)
				if ((acptr = local[i]) && (cptr != acptr) &&
				    !IsListener(acptr) &&
				    !bcmp((char *)&cptr->ip,(char *)&acptr->ip,
					  sizeof(cptr->ip)))
				    {
					hcnt++;
					if (!strncasecmp(acptr->auth,
							 cptr->auth, USERLEN))
						ucnt++;
				    }
			if ((ConfMaxHLocal(aconf) > 0 &&
			    hcnt >= ConfMaxHLocal(aconf))
#ifdef RUSNET_RLINES
			    || (IsRMode(cptr) && hcnt >= 2 )
#endif			    
			    )
				return -4;	/* for error message */
			if ((ConfMaxUHLocal(aconf) > 0 &&
			    ucnt >= ConfMaxUHLocal(aconf))
#ifdef RUSNET_RLINES
			    || (IsRMode(cptr) && ucnt >= 2 )
#endif			    
			    
			    )
				return -5;      /* for error message */
		}
		/*
		** Global limits
		**	host check is done on the hostname (IP if unresolved)
		**	user check is done on username
		*/
		if (ConfMaxHGlobal(aconf) > 0 || ConfMaxUHGlobal(aconf) > 0)
		    {
			Reg     aClient *acptr;
			Reg     int     ghcnt = hcnt, gucnt = ucnt;

			for (acptr = client; acptr; acptr = acptr->next)
			    {
				if (!IsPerson(acptr))
					continue;
				if (MyConnect(acptr) &&
				    (ConfMaxHLocal(aconf) > 0 ||
				     ConfMaxUHLocal(aconf) > 0))
					continue;

				if (!strcmp(cptr->sockhost, acptr->sockhost))
				    {
					if ((ConfMaxHGlobal(aconf) > 0 &&
					    ++ghcnt >= ConfMaxHGlobal(aconf))
#ifdef RUSNET_RLINES
					    || (IsRMode(cptr) && ghcnt >= 2)
#endif
					    )
						return -6;
					if (ConfMaxUHGlobal(aconf) > 0 &&
					    !strcmp(cptr->user->username,
						    acptr->user->username) &&
					    (++gucnt >=ConfMaxUHGlobal(aconf)
#ifdef RUSNET_RLINES
					    || (IsRMode(cptr) && gucnt >= 2)
#endif			    
					    ))
						return -7;
				    }
			    }
		    }
	    }

	lp = make_link();
	istat.is_conflink++;
	lp->next = cptr->confs;
	lp->value.aconf = aconf;
	cptr->confs = lp;
	aconf->clients++;
	if (aconf->status & CONF_CLIENT_MASK)
		ConfLinks(aconf)++;
	return 0;
}


aConfItem *find_admin()
    {
	Reg	aConfItem	*aconf;

	for (aconf = conf; aconf; aconf = aconf->next)
		if (aconf->status & CONF_ADMIN)
			break;
	
	return (aconf);
    }

aConfItem *find_me()
    {
	Reg	aConfItem	*aconf;
	for (aconf = conf; aconf; aconf = aconf->next)
		if (aconf->status & CONF_ME)
			break;
	
	return (aconf);
    }

/*
 * attach_confs
 *  Attach a CONF line to a client if the name passed matches that for
 * the conf file (for non-C/N lines) or is an exact match (C/N lines
 * only).  The difference in behaviour is to stop C:*::* and N:*::*.
 */
aConfItem *attach_confs(cptr, name, statmask)
aClient	*cptr;
char	*name;
int	statmask;
{
	Reg	aConfItem	*tmp;
	aConfItem	*first = NULL;
	int	len = strlen(name);
  
	if (!name || len > HOSTLEN)
		return NULL;
	for (tmp = conf; tmp; tmp = tmp->next)
	    {
		if ((tmp->status & statmask) && !IsIllegal(tmp) &&
		    ((tmp->status & (CONF_SERVER_MASK|CONF_HUB)) == 0) &&
		    tmp->name && !match(tmp->name, name))
		    {
			if (!attach_conf(cptr, tmp) && !first)
				first = tmp;
		    }
		else if ((tmp->status & statmask) && !IsIllegal(tmp) &&
			 (tmp->status & (CONF_SERVER_MASK|CONF_HUB)) &&
			 tmp->name && !strcasecmp(tmp->name, name))
		    {
			if (!attach_conf(cptr, tmp) && !first)
				first = tmp;
		    }
	    }
	return (first);
}

/*
 * Added for new access check    meLazy
 */
aConfItem *attach_confs_host(cptr, host, statmask)
aClient *cptr;
char	*host;
int	statmask;
{
	Reg	aConfItem *tmp;
	aConfItem *first = NULL;
	int	len = strlen(host);
  
	if (!host || len > HOSTLEN)
		return NULL;

	for (tmp = conf; tmp; tmp = tmp->next)
	    {
		if ((tmp->status & statmask) && !IsIllegal(tmp) &&
		    (tmp->status & CONF_SERVER_MASK) == 0 &&
		    (!tmp->host || match(tmp->host, host) == 0))
		    {
			if (!attach_conf(cptr, tmp) && !first)
				first = tmp;
		    }
		else if ((tmp->status & statmask) && !IsIllegal(tmp) &&
	       	    (tmp->status & CONF_SERVER_MASK) &&
	       	    (tmp->host && strcasecmp(tmp->host, host) == 0))
		    {
			if (!attach_conf(cptr, tmp) && !first)
				first = tmp;
		    }
	    }
	return (first);
}

/*
 * find a conf entry which matches the hostname and has the same name.
 */
aConfItem *find_conf_exact(name, user, host, statmask)
char	*name, *host, *user;
int	statmask;
{
	Reg	aConfItem *tmp;
	char	userhost[USERLEN+HOSTLEN+3];

	sprintf(userhost, "%s@%s", user, host);

	for (tmp = conf; tmp; tmp = tmp->next)
	    {
		if (!(tmp->status & statmask) || !tmp->name || !tmp->host ||
		    strcasecmp(tmp->name, name))
			continue;
		/*
		** Accept if the *real* hostname (usually sockecthost)
		** socket host) matches *either* host or name field
		** of the configuration.
		*/
		if (match(tmp->host, userhost))
			continue;
		if (tmp->status & (CONF_OPERATOR|CONF_LOCOP))
		    {
			if (tmp->clients < MaxLinks(Class(tmp)))
				return tmp;
			else
				continue;
		    }
		else
			return tmp;
	    }
	return NULL;
}

/*
 * find an O-line which matches the hostname and has the same "name".
 */
aConfItem *find_Oline(name, cptr)
char	*name;
aClient	*cptr;
{
	Reg	aConfItem *tmp;
	char	userhost[USERLEN+HOSTLEN+3];
	char	userip[USERLEN+HOSTLEN+3];

	sprintf(userhost, "%s@%s", cptr->username, cptr->sockhost);
	sprintf(userip, "%s@%s", cptr->username, 
#ifdef INET6
		(char *)inetntop(AF_INET6, (char *)&cptr->ip, mydummy,
			MYDUMMY_SIZE)
#else
		(char *)inetntoa((char *)&cptr->ip)
#endif
	);


	for (tmp = conf; tmp; tmp = tmp->next)
	    {
		if (!(tmp->status & (CONF_OPS)) || !tmp->name || !tmp->host ||
			strcasecmp(tmp->name, name))
			continue;
		/*
		** Accept if the *real* hostname matches the host field or
		** the ip does.
		*/
		if (match(tmp->host, userhost) && match(tmp->host, userip) &&
			(!strchr(tmp->host, '/') 
			|| match_ipmask(tmp->host, cptr)))
			continue;
		if (tmp->clients < MaxLinks(Class(tmp)))
			return tmp;
	    }
	return NULL;
}


aConfItem *find_conf_name(name, statmask)
char	*name;
int	statmask;
{
	Reg	aConfItem *tmp;
 
	for (tmp = conf; tmp; tmp = tmp->next)
	    {
		/*
		** Accept if the *real* hostname (usually sockecthost)
		** matches *either* host or name field of the configuration.
		*/
		if ((tmp->status & statmask) &&
		    (!tmp->name || match(tmp->name, name) == 0))
			return tmp;
	    }
	return NULL;
}

aConfItem *find_conf(lp, name, statmask)
char	*name;
Link	*lp;
int	statmask;
{
	Reg	aConfItem *tmp;
	int	namelen = name ? strlen(name) : 0;
  
	if (namelen > HOSTLEN)
		return (aConfItem *) 0;

	for (; lp; lp = lp->next)
	    {
		tmp = lp->value.aconf;
		if ((tmp->status & statmask) &&
		    (((tmp->status & (CONF_SERVER_MASK|CONF_HUB)) &&
	 	     tmp->name && !strcasecmp(tmp->name, name)) ||
		     ((tmp->status & (CONF_SERVER_MASK|CONF_HUB)) == 0 &&
		     tmp->name && !match(tmp->name, name))))
			return tmp;
	    }
	return NULL;
}

/*
 * Added for new access check    meLazy
 */
aConfItem *find_conf_host(lp, host, statmask)
Reg	Link	*lp;
char	*host;
Reg	int	statmask;
{
	Reg	aConfItem *tmp;
	int	hostlen = host ? strlen(host) : 0;
  
	if (hostlen > HOSTLEN || BadPtr(host))
		return (aConfItem *)NULL;
	for (; lp; lp = lp->next)
	    {
		tmp = lp->value.aconf;
		if (tmp->status & statmask &&
		    (!(tmp->status & CONF_SERVER_MASK || tmp->host) ||
	 	     (tmp->host && !match(tmp->host, host))))
			return tmp;
	    }
	return NULL;
}

/*
 * find_conf_ip
 *
 * Find a conf line using the IP# stored in it to search upon.
 * Added 1/8/92 by Avalon.
 */
aConfItem *find_conf_ip(lp, ip, user, statmask)
char	*ip, *user;
Link	*lp;
int	statmask;
{
	Reg	aConfItem *tmp;
	Reg	char	*s;
  
	for (; lp; lp = lp->next)
	    {
		tmp = lp->value.aconf;
		if (!(tmp->status & statmask))
			continue;
		s = index(tmp->host, '@');
		*s = '\0';
		if (match(tmp->host, user))
		    {
			*s = '@';
			continue;
		    }
		*s = '@';
		if (!bcmp((char *)&tmp->ipnum, ip, sizeof(struct IN_ADDR)))
			return tmp;
	    }
	return NULL;
}

/*
 * find_conf_entry
 *
 * - looks for a match on all given fields.
 */
aConfItem *find_conf_entry(aconf, mask)
aConfItem *aconf;
u_int	mask;
{
	Reg	aConfItem *bconf;

	for (bconf = conf, mask &= ~CONF_ILLEGAL; bconf; bconf = bconf->next)
	    {
		if (!(bconf->status & mask) || (bconf->port != aconf->port))
			continue;

		if ((BadPtr(bconf->host) && !BadPtr(aconf->host)) ||
		    (BadPtr(aconf->host) && !BadPtr(bconf->host)))
			continue;
		if (!BadPtr(bconf->host) && strcasecmp(bconf->host, aconf->host))
			continue;

		if ((BadPtr(bconf->passwd) && !BadPtr(aconf->passwd)) ||
		    (BadPtr(aconf->passwd) && !BadPtr(bconf->passwd)))
			continue;
		if (!BadPtr(bconf->passwd) &&
		    strcasecmp(bconf->passwd, aconf->passwd))
			continue;

		if ((BadPtr(bconf->name) && !BadPtr(aconf->name)) ||
		    (BadPtr(aconf->name) && !BadPtr(bconf->name)))
			continue;
		if (!BadPtr(bconf->name) && strcasecmp(bconf->name, aconf->name))
			continue;
		break;
	    }
	return bconf;
}

#ifndef USE_OLD8BIT
static int find_conf_listener (char *charset)
{
	Reg	aConfItem *aconf;

	for (aconf = conf; aconf; aconf = aconf->next)
	    if (aconf->status == CONF_LISTEN_PORT &&
		!strcasecmp (charset, aconf->passwd))
			return aconf->port;
	return 0;
}
#endif

/*
 * rehash
 *
 * Actual REHASH service routine. Called with sig == 0 if it has been called
 * as a result of an operator issuing this command, else assume it has been
 * called as a result of the server receiving a HUP signal.
 */
int	rehash(sig, status)
int	sig;
int	status;
{
	Reg	aConfItem **tmp = &conf, *tmp2 = NULL;
	Reg	aClass	*cltmp;
	Reg	aClient	*acptr;
	Reg	int	i;
	int	ret = 0;
	int	rehash_flags = 0;	
	
    	if (sig == 1)
	    {
		sendto_flag(SCH_NOTICE,
			    "Got signal SIGHUP, reloading ircd.conf file");
#ifdef	ULTRIX
		if (fork() > 0)
			exit(0);
		write_pidfile();
#endif
		status = REHASH_ALL;
#ifdef DEBUGMODE
		close(2);	/* close debugfile */
		open_debugfile();
#endif
	    }
    if (! (status & REHASH_ALL))
    {
	if (status & REHASH_I)
	    rehash_flags |= CONF_CLIENT | CONF_RCLIENT;
	if (status & REHASH_O)
	    rehash_flags |= CONF_OPS;
	if (status & REHASH_K)
	    rehash_flags |= CONF_KILL;
	if (status & REHASH_E)
	    rehash_flags |= CONF_EXEMPT;
	if (status & REHASH_C)
	    rehash_flags |= CONF_SERVER_MASK | CONF_HUB |
			    CONF_LEAF | CONF_SERVICE | CONF_INTERFACE;
	if (status & REHASH_B)
	    rehash_flags |= CONF_BOUNCE;
	if (status & REHASH_Y)
	    rehash_flags |= CONF_CLASS;
	if (status & REHASH_P)
	    rehash_flags |= CONF_LISTEN_PORT
#ifdef USE_SSL
	     | CONF_SSL_LISTEN_PORT
#endif
	     ;
#ifdef RUSNET_RLINES
	if (status & REHASH_R)
	    rehash_flags |= CONF_RUSNETRLINE;
#endif
    }
    if ((status & REHASH_ALL) || (status & REHASH_DNS))
	for (i = 0; i <= highest_fd; i++)
		if ((acptr = local[i]) && !IsMe(acptr))
		    {
			/*
			 * Nullify any references from client structures to
			 * this host structure which is about to be freed.
			 * Could always keep reference counts instead of
			 * this....-avalon
			 * The same is done in flush_cache(). We need this
			 * if only we have references from  client structures
			 * not registered in cache...why? -skold
			 */
			acptr->hostp = NULL;
		    }

	/*
	** We consider the following:
	** 1. Drop from conf and mark as ILLEGAL those in use except I/P/p
	** 2. Do not drop but mark only: I/P/p
	** 3. Drop and free memory for the rest
	**    --skold
	*/

	while ((tmp2 = *tmp))
	    if ((status & REHASH_ALL) || (tmp2->status & rehash_flags))
		if (tmp2->clients || (tmp2->status & CONF_LISTEN_PORT) 
#ifdef USE_SSL
			|| (tmp2->status & CONF_SSL_LISTEN_PORT)
#endif /* USE_SSL */
		)
		    {
			/*
			** Configuration entry is still in use by some
			** local clients, cannot delete it--mark it so
			** that it will be deleted when the last client
			** exits...
			*/
			if (!(tmp2->status & (CONF_LISTEN_PORT|CONF_CLIENT
#ifdef USE_SSL
				|CONF_SSL_LISTEN_PORT
#endif /* USE_SSL */
			)))
			    {
				*tmp = tmp2->next; /* references from clients exist, so drop safely here */
				tmp2->next = NULL;
			    }
			else /* I/P/p lines not deleting, just mark*/
				tmp = &tmp2->next;
			tmp2->status |= CONF_ILLEGAL;
		    }
		else
		    {
			*tmp = tmp2->next;
			free_conf(tmp2);
	    	    }
	    else
		tmp = &tmp2->next;

    if ((status & REHASH_ALL) || (status & REHASH_K))
    {
    	tmp = &kconf;
	while ((tmp2 = *tmp))
	    {
		*tmp = tmp2->next;
		free_conf(tmp2);
	    }
    }
    if ((status & REHASH_ALL) || (status & REHASH_E))
    {
	tmp = &econf;
	while ((tmp2 = *tmp))
	    {
		*tmp = tmp2->next;
		free_conf(tmp2);
	    }
    }

#ifdef RUSNET_RLINES
    if ((status & REHASH_ALL) || (status & REHASH_R))
    {
	tmp = &rconf;
	while ((tmp2 = *tmp))
	    {
		*tmp = tmp2->next;
		free_conf(tmp2);
	    }
    }
#endif
    if ((status & REHASH_ALL) || (status & REHASH_Y))
	/*
	 * We don't delete the class table, rather mark all entries
	 * for deletion. The table is cleaned up by check_class. - avalon
	 */
	for (cltmp = NextClass(FirstClass()); cltmp; cltmp = NextClass(cltmp))
		MaxLinks(cltmp) = -1;

    if ((status & REHASH_ALL) || (status & REHASH_DNS)) {
	flush_cache(); /* Flush local dns cache */
	ircd_res_init(); /* Re-read resolv.conf */
    }

    if ((status & REHASH_ALL) || (status & REHASH_C))
	rusnet_free_routes();

#ifdef LOG_EVENTS
    if (status & REHASH_ALL)
	log_closeall();
#endif


#ifdef USE_SSL
    if ((status & REHASH_ALL) || (status & REHASH_SSL))
	ssl_rehash();
#endif

    /*
    ** Now let's restore configuration and cleanup garbage
    */
    if (!((status & REHASH_DNS) || (status & REHASH_MOTD)
#ifdef USE_SSL    
     || (status & REHASH_SSL)
#endif     
     ))
	(void) initconf(0, rehash_flags);

	/*
	 * Delete all I/P/p marked previously as ILLEGAL, flush not used entries.
	 * This is safe since they are referenced from other structures -skold
	 */
    if ((status & REHASH_ALL) || (status & REHASH_P))
	close_listeners();
	
    for (tmp = &conf; (tmp2 = *tmp); )
	if ((IsIllegal(tmp2)) &&
		((status & REHASH_ALL) || (tmp2->status & rehash_flags)) &&
		!tmp2->clients)
		/* Why drop from list when we don't free it?  --erra */
	    {
		*tmp = tmp2->next;
		tmp2->next = NULL;
		free_conf(tmp2);
	    }
	else
		tmp = &tmp2->next;

    if ((status & REHASH_ALL) || (status & REHASH_MOTD))
	read_motd(IRCDMOTD_PATH);

	rehashed = 1;
	return ret;
}

/*
 * openconf
 *
 * returns -1 on any error or else the fd opened from which to read the
 * configuration file from.  This may either be the file direct or one end
 * of a pipe from m4.
 */
int	openconf(void)
{
#ifdef	M4_PREPROC
	int	pi[2], i;
#else
	int ret;
#endif

#ifdef M4_PREPROC
	if (pipe(pi) == -1)
		return -1;
	switch(vfork())
	{
	case -1 :
		if (serverbooting)
		{
			fprintf(stderr,
			"Fatal Error: Unable to fork() m4 (%s)",
			strerror(errno));
		}
		return -1;
	case 0 :
		(void)close(pi[0]);
		if (pi[1] != 1)
		    {
			(void)dup2(pi[1], 1);
			(void)close(pi[1]);
		    }
		/* If the server is booting, stderr is still open and
		 * user should receive error message */
		if (!serverbooting)
		{
		    (void)dup2(1,2);
		}
		for (i = 3; i < MAXCONNECTIONS; i++)
			if (local[i])
				(void) close(i);
		/*
		 * m4 maybe anywhere, use execvp to find it.  Any error
		 * goes out with report_error.  Could be dangerous,
		 * two servers running with the same fd's >:-) -avalon
		 */
		(void)execlp("m4", "m4", IRCDM4_PATH, configfile, 0);
		if (serverbooting)
		{
			fprintf(stderr,"Fatal Error: Error executing m4 (%s)",
				strerror(errno));
		}
		report_error("Error executing m4 %s:%s", &me);
		_exit(-1);
	default :
		(void)close(pi[1]);
		return pi[0];
	}
#else
	if ((ret = open(configfile, O_RDONLY)) == -1)
	{
		if (serverbooting)
		{
			fprintf(stderr,
			"Fatal Error: Can not open configuration file %s (%s)\n",
			configfile,strerror(errno));
		}
	}
	return ret;
#endif
}

/*
** char *ipv6_convert(char *orig)
** converts the original ip address to an standard form
** returns a pointer to a string.
*/

#ifdef	INET6
char	*ipv6_convert(orig)
char	*orig;
{
	char	*s, *t, *buf = NULL;
	int	i, j;
	int	len = 1;	/* for the '\0' in case of no @ */
	struct	in6_addr addr;
	char	dummy[MYDUMMY_SIZE];

	if ((s = strchr(orig, '@')))
	    {
		*s = '\0';
		len = strlen(orig) + 2;	/* +2 for '@' and '\0' */
		buf = (char *)MyMalloc(len);
		strcpy(buf, orig);
		buf[len - 2] = '@';
		buf[len - 1] = '\0'; 
		*s = '@';
		orig = s + 1;
	    }

	if ((s = strchr(orig, '/')))
	    {
		*s = '\0';
		s++;
	    }

	i = inetpton(AF_INET6, orig, addr.s6_addr);

	t = (i > 0) ?
		inetntop(AF_INET6, addr.s6_addr, dummy, MYDUMMY_SIZE) : orig;
	
	j = len - 1;

	len += strlen(t);
	buf = (char *)MyRealloc(buf, len);
	strcpy(buf + j, t);

	if (s)
	    {
		*(s-1) = '/'; /* put the '/' back, not sure it's needed tho */ 
		j = len;
		len += strlen(s) + 1;
		buf = (char *)MyRealloc(buf, len);
		buf[j - 1] = '/';
		strcpy(buf + j, s);
	    }

	return buf;
}
#endif

/*
** initconf() 
**    Read configuration file.
**
**    returns -1, if file cannot be opened
**             0, if file opened
**
**    init_flags = read this portion of config only
*/

#define MAXCONFLINKS 150

int 	initconf(opt, init_flags)
int	opt;
int	init_flags;
{
	static	char	quotes[9][2] = {{'b', '\b'}, {'f', '\f'}, {'n', '\n'},
					{'r', '\r'}, {'t', '\t'}, {'v', '\v'},
					{'\\', '\\'}, { 0, 0}};
	Reg	char	*tmp, *s;
	int	fd, i;
	char	*tmp2 = NULL, *tmp3 = NULL, *tmp4 = NULL;
	int	ccount = 0, ncount = 0;
	aConfItem *aconf = NULL;
	char	*line;
	aConfig	*ConfigTop, *p;
	FILE	*fdn;
#ifndef USE_OLD8BIT
	conversion_t *conv;
	char	line2[4096];
	char	*rline;
	size_t	len;
  
	UseUnicode = -1;
	Force8bit = 0; /* see set_internal_encoding() */
#endif

	Debug((DEBUG_DEBUG, "initconf(): ircd.conf = %s", configfile));
	if ((fd = openconf()) == -1)
	    {
#if defined(M4_PREPROC) && !defined(USE_IAUTH)
		(void)wait(0);
#endif
		return -1;
	    }
	fdn = fdopen(fd, "r");
	if (fdn == NULL)
	{
		if (serverbooting)
		{
			fprintf(stderr,
			"Fatal Error: Can not open configuration file %s (%s)\n",
			configfile,strerror(errno));
		}
		return -1;
	}
#ifndef USE_OLD8BIT
	conv = conv_get_conversion(config_charset);
#endif
	ConfigTop = config_read(fdn, 0, new_config_file(configfile, NULL, 0));
	for(p = ConfigTop; p; p = p->next)
	{
		line = p->line;

		/*
		 * Do quoting of characters and # detection.
		 */
		for (tmp = line; *tmp; tmp++)
		    {
			if (*tmp == '\\')
			    {
				for (i = 0; quotes[i][0]; i++)
					if (quotes[i][0] == *(tmp + 1))
					    {
						*tmp = quotes[i][1];
						break;
					    }
				if (!quotes[i][0])
					*tmp = *(tmp + 1);
				if (!*(tmp+1))
					break;
				else
					for (s = tmp; (*s = *(s + 1)); s++)
						;
			    }
			else if (*tmp == '#')
			    {
				*tmp = '\0';
				break;	/* Ignore the rest of the line */
			    }
		    }
		if (!*line || line[0] == '#' || line[0] == '\n' ||
		    line[0] == ' ' || line[0] == '\t')
			continue;
		/* Could we test if it's conf line at all?	-Vesa */
		if (line[1] != IRCDCONF_DELIMITER)
		    {
                        Debug((DEBUG_ERROR, "Bad config line: %s", line));
                        continue;
                    }
		if (aconf)
			free_conf(aconf);
		aconf = make_conf();

		if (tmp2)
		    {
			MyFree(tmp2);
			tmp2 = NULL;
		    }
		tmp3 = tmp4 = NULL;
#ifndef USE_OLD8BIT
		if (conv)
		{
			rline = line2;
			len = conv_do_in(conv, line, strlen(line),
							&rline, sizeof(line2));
			rline[len] = '\0';
		}
		tmp = getfield(rline);
#else
		tmp = getfield(line);
#endif
		if (!tmp)
			continue;
		switch (*tmp)
		{
			case 'A': /* Name, e-mail address of administrator */
			case 'a': /* of this server. */
				aconf->status = CONF_ADMIN;
				break;
			case 'B': /* Name of alternate servers */
			case 'b':
				aconf->status = CONF_BOUNCE;
				break;
			case 'C': /* Server where I should try to connect */
			  	  /* in case of lp failures             */
				ccount++;
				aconf->status = CONF_CONNECT_SERVER;
				break;
			case 'c':
				ccount++;
				aconf->status = CONF_ZCONNECT_SERVER;
				break;
			case 'D': /* auto connect restrictions */
			case 'd':
				aconf->status = CONF_DENY;
				break;
			case 'E': /* Exempt from kill user line */
			case 'e':
			        aconf->status = CONF_EXEMPT;
		        	break;
			case 'F': /* virtual interface to fasten to when  */
			case 'f': /* connecting to the server mentioned */
			        aconf->status = CONF_INTERFACE;
		        	break;
			case 'H': /* Hub server line */
			case 'h':
				aconf->status = CONF_HUB;
				break;
			case 'I': /* Just plain normal irc client trying  */
			          /* to connect me */
				aconf->status = CONF_CLIENT;
				break;
			case 'i' : /* Restricted client */
				aconf->status = CONF_RCLIENT;
				break;
			case 'K': /* Kill user line on ircd.conf           */
			case 'k':
				aconf->status = CONF_KILL;
				break;
			/* Operator. Line should contain at least */
			/* password and host where connection is  */
			case 'L': /* guaranteed leaf server */
				aconf->status = CONF_LEAF;
				break;
#ifdef LOG_EVENTS
			case 'l':
				aconf->status = CONF_LOG;
				break;
#endif
			/* Me. Host field is name used for this host */
			/* and port number is the number of the port */
			case 'M':
			case 'm':
				aconf->status = CONF_ME;
				break;
			case 'N': /* Server where I should NOT try to     */
			case 'n': /* connect in case of lp failures     */
				  /* but which tries to connect ME        */
				++ncount;
				aconf->status = CONF_NOCONNECT_SERVER;
				break;
			case 'O':
				aconf->status = CONF_OPERATOR;
				break;
			/* Local Operator, (limited privs --SRB) */
			case 'o':
				aconf->status = CONF_LOCOP;
				break;
#ifdef USE_SSL
			case 'p': /* SSL listen port line */
				aconf->status = CONF_SSL_LISTEN_PORT;
				break;
#endif /* USE_SSL */
			case 'P': /* listen port line */
				aconf->status = CONF_LISTEN_PORT;
				break;
			case 'Q': /* a server that you don't want in your */
			case 'q': /* network. USE WITH CAUTION! */
				aconf->status = CONF_QUARANTINED_SERVER;
				break;
#ifdef RUSNET_RLINES
			case 'R': /* Rusnet R-lines */
				aconf->status = CONF_RUSNETRLINE;
				break;
#endif
			case 'S': /* Service. Same semantics as   */
			case 's': /* CONF_OPERATOR                */
				aconf->status = CONF_SERVICE;
				break;
			case 'V': /* Server link version requirements */
				aconf->status = CONF_VER;
				break;
			case 'Y':
			case 'y':
			        aconf->status = CONF_CLASS;
		        	break;
			default:
			Debug((DEBUG_ERROR, "Error in config file: %s", line));
			break;
		    }
		if (IsIllegal(aconf))
			continue;

		/*
		** Split this global switch.
		** Rehash is getting really busy with this code otherwise.
		** -skold
		*/
		if (init_flags && !(aconf->status & init_flags))
			continue;

#ifdef USE_SSL
		if (aconf->status & CONF_SSL_LISTEN_PORT && !sslable)
			{
			fprintf(stderr, "You have p:lines in your conf, "
				"but SSL init failed (probably you did not "
				"create SSL certificate?)\n"
				"Reverting to non-ssl ports.\n");
			aconf->status = CONF_LISTEN_PORT;
		}
#endif


		for (;;) /* Fake loop, that I can use break here --msa */
		    {
			if ((tmp = getfield(NULL)) == NULL)
				break;
#ifdef	INET6
			if (aconf->status & 
				(CONF_CONNECT_SERVER|CONF_ZCONNECT_SERVER
				|CONF_CLIENT|CONF_RCLIENT
				|CONF_TLINE|CONF_NOCONNECT_SERVER
				|CONF_OPERATOR|CONF_LOCOP|CONF_LISTEN_PORT
#ifdef USE_SSL
				|CONF_SSL_LISTEN_PORT
#endif
					|CONF_INTERFACE|CONF_SERVICE))
				aconf->host = ipv6_convert(tmp);
			else
#endif
			DupString(aconf->host, tmp);
			if ((tmp = getfield(NULL)) == NULL)
				break;
			if (aconf->status & 
				(CONF_LISTEN_PORT
#ifdef USE_SSL
						|CONF_SSL_LISTEN_PORT
#endif
									))
			{
				/* provide reasonable default for ports
					with unassigned charset  --erra */
				char *s = *tmp ? tmp : DEFAULT_CHARSET;

				DupString(aconf->passwd, s);
			}
			else
			DupString(aconf->passwd, tmp);
			if ((tmp = getfield(NULL)) == NULL)
				break;
#ifdef	INET6
			if (aconf->status & CONF_INTERFACE)
				aconf->name = ipv6_convert(tmp);
			else
#endif
			if (aconf->status & (CONF_TLINE))
			{
			    if (EmptyString(tmp))
				tmp = "*";
			}
			DupString(aconf->name, tmp);
			if ((tmp = getfield(NULL)) == NULL)
				break;
			aconf->port = 0;
			if (sscanf(tmp, "0x%x", &aconf->port) != 1 ||
			    aconf->port == 0)
				aconf->port = atoi(tmp);
			/* fool proof: guard against port numbers in N:lines */
			/* Guard them even smarter than that -cj*/
			if (aconf->status == CONF_NOCONNECT_SERVER &&
							aconf->port > 1024)
				aconf->port = 0;
			if (aconf->status == CONF_CONNECT_SERVER)
				DupString(tmp2, tmp);
			if (aconf->status == CONF_ZCONNECT_SERVER)
				DupString(tmp2, tmp);
			if ((tmp = getfield(NULL)) == NULL) {
			    if (aconf->status & (CONF_TLINE)) /* compat with tkserv */
			    {
				aconf->hold = 0;
			    }

				break;
			}
			if (aconf->status & (CONF_TLINE))
			{
			    aconf->hold = atoi(tmp);
			}
			Class(aconf) = find_class(atoi(tmp));
			/* the following are only used for Y: */
			if ((tmp3 = getfield(NULL)) == NULL)
				break;
			tmp4 = getfield(NULL);
			break;
		    }
		/*
		** Check for broken T-lines
		*/
		if ((aconf->status & (CONF_TLINE)) && EmptyString(aconf->name)) {
		    tmp = "*";
		    DupString(aconf->name, tmp);
		}

		istat.is_confmem += aconf->host ? strlen(aconf->host)+1 : 0;
		istat.is_confmem += aconf->passwd ? strlen(aconf->passwd)+1 :0;
		istat.is_confmem += aconf->name ? strlen(aconf->name)+1 : 0;

		/*
		** Bounce line fields are mandatory
		*/
#ifdef LOG_EVENTS
		if (aconf->status == CONF_LOG)
			log_open(aconf);
#endif
		/*
                ** If conf line is a class definition, create a class entry
                ** for it and make the conf_line illegal and delete it.
                */
		if (aconf->status & CONF_CLASS)
		{
			if (atoi(aconf->host) >= 0)
				add_class(atoi(aconf->host),
					  atoi(aconf->passwd),
					  atoi(aconf->name), aconf->port,
					  tmp ? atoi(tmp) : 0,
					  tmp3 ? atoi(tmp3) : 1,
					  (tmp3 && index(tmp3, '.')) ?
					  atoi(index(tmp3, '.') + 1) : 1,
 					  tmp4 ? atoi(tmp4) : 1,
					  (tmp4 && index(tmp4, '.')) ?
					  atoi(index(tmp4, '.') + 1) : 1);
			continue;
		}
		/*
		** associate each conf line with a class by using a pointer
		** to the correct class record. -avalon
		*/
		if (aconf->status & (CONF_CLIENT_MASK|CONF_LISTEN_PORT)
#ifdef USE_SSL
			|| aconf->status & (CONF_CLIENT_MASK|CONF_SSL_LISTEN_PORT)
#endif		
		)
		    {
			if (Class(aconf) == 0)
				Class(aconf) = find_class(0);
			if (MaxLinks(Class(aconf)) < 0)
				Class(aconf) = find_class(0);
		    }
		if (aconf->status & (CONF_LISTEN_PORT|CONF_CLIENT|CONF_RCLIENT
#ifdef USE_SSL
			|CONF_SSL_LISTEN_PORT
#endif		
		))
		    {
			aConfItem *bconf;

			if ((bconf = find_conf_entry(aconf, aconf->status)))
			    {
				delist_conf(bconf);
				bconf->status &= ~CONF_ILLEGAL;
				if (aconf->status == CONF_CLIENT)
				    {
					bconf->class->links -= bconf->clients;
					bconf->class = aconf->class;
					bconf->class->links += bconf->clients;
				    }
#ifndef USE_OLD8BIT
				else if (aconf->status & (CONF_LISTEN_PORT
#ifdef USE_SSL
							|CONF_SSL_LISTEN_PORT
#endif		
					) && (find_me())->port == aconf->port)
				    { /* it's port with internal encoding */
					set_internal_encoding(find_listener(aconf->port),
							      aconf);
				    }
#endif
				free_conf(aconf);
				aconf = bconf;
			    }
#ifdef USE_SSL
			else if (((aconf->status & CONF_LISTEN_PORT) &&
			    (bconf = find_conf_entry(aconf, CONF_SSL_LISTEN_PORT))) ||
			    ((aconf->status & CONF_SSL_LISTEN_PORT) &&
			    (bconf = find_conf_entry(aconf, CONF_LISTEN_PORT))))
			    {
				delist_conf(bconf);
				bconf->status &= ~CONF_ILLEGAL;
				bconf->status ^= (CONF_LISTEN_PORT | CONF_SSL_LISTEN_PORT);
				if (aconf->status == CONF_CLIENT)
				    {
					bconf->class->links -= bconf->clients;
					bconf->class = aconf->class;
					bconf->class->links += bconf->clients;
				    }
				free_conf(aconf);
				aconf = bconf;
			    }
#endif
			else if (aconf->host &&
				 (aconf->status == CONF_LISTEN_PORT
#ifdef USE_SSL
				 || aconf->status == CONF_SSL_LISTEN_PORT
#endif
				 ))
			{
				if (add_listener(aconf))
				{
					Debug((DEBUG_NOTICE, "Not adding new"
						" listener: %s %d: port"
						" opening error",
						aconf->host, aconf->port));
					aconf->status |= CONF_ILLEGAL;
				}
				else
				{
					Debug((DEBUG_NOTICE,
						"Adding new listener: %s %d %s",
						aconf->host, aconf->port,
						(aconf->status ==
							CONF_LISTEN_PORT) ?
							"Non-SSL" : "SSL"));
				}
			}
		    }
		if (aconf->status & CONF_SERVICE)
			aconf->port &= SERVICE_MASK_ALL;
		if (aconf->status & (CONF_SERVER_MASK|CONF_SERVICE))
			if (ncount > MAXCONFLINKS || ccount > MAXCONFLINKS ||
			    !aconf->host || index(aconf->host, '*') ||
			     index(aconf->host,'?') || !aconf->name)
				continue;

		if (aconf->status &
		    (CONF_SERVER_MASK|CONF_LOCOP|CONF_OPERATOR|CONF_SERVICE
		    |CONF_TLINE))
			if (!index(aconf->host, '@') && *aconf->host != '/')
			    {
				char	*newhost;
				int	len = 3;	/* *@\0 = 3 */

				len += strlen(aconf->host);
				newhost = (char *)MyMalloc(len);
				sprintf(newhost, "*@%s", aconf->host);
				MyFree(aconf->host);
				aconf->host = newhost;
				istat.is_confmem += 2;
			    }
		if (aconf->status & CONF_SERVER_MASK)
		    {
			if (BadPtr(aconf->passwd))
				continue;
			else if (!(opt & BOOT_QUICK))
				(void)lookup_confhost(aconf);
		    }
		if (aconf->status &
				(CONF_CONNECT_SERVER | CONF_ZCONNECT_SERVER))
		    {
			aconf->ping = (aCPing *)MyMalloc(sizeof(aCPing));
			bzero((char *)aconf->ping, sizeof(*aconf->ping));
			istat.is_confmem += sizeof(*aconf->ping);
			if (tmp2)
			    {
			 	char *x = index(tmp2, '.');

				aconf->ping->port = (x) ? atoi(++x) :
								aconf->port;
				MyFree(tmp2);
				tmp2 = NULL;
			    }
			else
				aconf->ping->port = aconf->port;
		    }

		/* we want autoconnect links prioritized */
		if (aconf->status & (CONF_CLIENT | CONF_RCLIENT |
				CONF_CONNECT_SERVER | CONF_ZCONNECT_SERVER |
				CONF_NOCONNECT_SERVER))
			aconf->localpref = (tmp3) ? atoi(tmp3) : 0;
		if (aconf->status & CONF_INTERFACE &&
				aconf->host && aconf->name)
		    {
			rusnet_add_route(aconf->host, aconf->name,
						(BadPtr(aconf->passwd)) ?
						"[Unnamed]" : aconf->passwd);
		    }
		    
		/*
		** Name cannot be changed after the startup.
		** (or could be allowed, but only if all links are closed
		** first).
		** Configuration info does not override the name and port
		** if previously defined. Note, that "info"-field can be
		** changed by "/rehash".
		*/
		if (aconf->status == CONF_ME)
		    {
			if (me.info != DefInfo)
				MyFree(me.info);
			/* we hope here that only 8bit chars are in description */
			me.info = MyMalloc(REALLEN+1);
			strncpyzt(me.info, aconf->name, REALLEN+1);
			if (ME[0] == '\0' && aconf->host[0])
				strncpyzt(ME, aconf->host,
					  sizeof(ME));
			if (aconf->port)
				setup_ping(aconf);
		    }
		(void)collapse(aconf->host);
		(void)collapse(aconf->name);
		Debug((DEBUG_NOTICE,
		      "Read Init: (%d) (%s) (%s) (%s) (%d) (%d)",
		      aconf->status, aconf->host, aconf->passwd,
		      aconf->name, aconf->port,
		      aconf->class ? ConfClass(aconf) : 0));

		if (aconf->status & CONF_KILL)
		    {
			aconf->next = kconf;
			kconf = aconf;
		    }
		else if (aconf->status & CONF_EXEMPT)
		    {
			aconf->next = econf;
			econf = aconf;
		    }
#ifdef RUSNET_RLINES
		else if (aconf->status & CONF_RUSNETRLINE)
		    {
			aconf->next = rconf;
			rconf = aconf;
		    }
#endif
		else
		    {
			aconf->next = conf;
			conf = aconf;
		    }
		aconf = NULL;
	    }
	if (aconf)
		free_conf(aconf);
#ifndef USE_OLD8BIT
	if (UseUnicode == -1) /* didn't set from config, so use default */
		set_internal_encoding(NULL, NULL);
	conv_free_conversion(conv);
#endif
	(void)close(fd);
#if defined(M4_PREPROC) && !defined(USE_IAUTH) && !defined(USE_ISERV)
	(void)wait(0);
#endif
	check_class();
	nextping = nextconnect = timeofday;
	return 0;
}

/*
 * lookup_confhost
 *   Do (start) DNS lookups of all hostnames in the conf line and convert
 * an IP addresses in a.b.c.d number for to IP#s.
 */
static	int	lookup_confhost(aconf)
Reg	aConfItem	*aconf;
{
	Reg	char	*s;
	Reg	struct	hostent *hp;
	Link	ln;

	if (BadPtr(aconf->host) || BadPtr(aconf->name))
		goto badlookup;
	if ((s = index(aconf->host, '@')))
		s++;
	else
		s = aconf->host;
	/*
	** Do name lookup now on hostnames given and store the
	** ip numbers in conf structure.
	*/
	if (!isalpha(*s) && !isdigit(*s))
		goto badlookup;

	/*
	** Prepare structure in case we have to wait for a
	** reply which we get later and store away.
	*/
	ln.value.aconf = aconf;
	ln.flags = ASYNC_CONF;

#ifdef INET6
	if (inetpton(AF_INET6, s, aconf->ipnum.s6_addr))
		;
#else
	if (isdigit(*s))
		aconf->ipnum.s_addr = inetaddr(s);
#endif
	else if ((hp = gethost_byname(s, &ln)))
		bcopy(hp->h_addr, (char *)&(aconf->ipnum),
			sizeof(struct IN_ADDR));
#ifdef	INET6
	else
		goto badlookup;

#else
	if (aconf->ipnum.s_addr != INADDR_NONE)
		return 0;
#endif

badlookup:
#ifdef INET6
	if (AND16(aconf->ipnum.s6_addr) == 255)
#endif
		bzero((char *)&aconf->ipnum, sizeof(struct IN_ADDR));
	Debug((DEBUG_ERROR,"Host/server name error: (%s) (%s)",
		aconf->host, aconf->name));
	return -1;
}

#ifdef RUSNET_RLINES
void do_restrict(cptr)
aClient	*cptr;
{
/* Unrestrict is nonsence. Umode -b never happens */
#ifndef NO_PREFIX
	if (IsRestricted(cptr) ||
		*cptr->user->username == '^' ||
		*cptr->user->username == '~')
	    {
		*cptr->user->username = '%';
	    }
	else if (!IsRMode(cptr))
	    {
		char buf[BUFSIZE];

		strncpyzt(buf, cptr->user->username, USERLEN+1);
		*cptr->user->username = '%';
		strncpy(&cptr->user->username[1], buf, USERLEN);
		cptr->user->username[USERLEN] = '\0';    
	    }
#endif
	SetRMode(cptr);
}
#endif

int 	check_tline_one(cptr, nick, aconf, checkhost, checkip)
aClient		*cptr;
char		*nick;
aConfItem	*aconf;
char		*checkhost;
char		*checkip;
{
	
    char	*p;

    if ((aconf->hold > 0) && (aconf->hold <= time(NULL)))
	return 1; /* Expired. We do not rehash these lists 
		    every now and then, so just skip it */

    /* host & IP matching.. */
    if (!checkip[0]) /* unresolved */
    {
	if (strchr(aconf->host, '/'))
	{
	    if (match_ipmask(aconf->host, cptr))
		return 1; /* not matched */
	}
	else          
	    if (match(aconf->host, checkhost))
		return 1; /* not matched */
    } 
    else
    { 
	if ((p = index(aconf->host, '@')) && (*(++p) == '='))
	    return 1;
	else if (*aconf->host == '=') /* numeric only */
	    return 1;
	else /* resolved */
	    if (strchr(aconf->host, '/'))
	    {
		if (match_ipmask(aconf->host, cptr))
		    return 1;
	    }
	    else
	    {
	    	if (match(aconf->host, checkip) &&
			match(aconf->host, checkhost))
		    return 1;
	    }
    }
    /* nick & port matching */
    if ((!nick || match(aconf->name, nick) == 0) &&
	    (!aconf->port || (aconf->port == cptr->acpt->port)))   
    {
	return 0; /* matched*/
    }
    return 1;
}

int	check_tlines(cptr, doall, comment, nick, iconf, aconf)
aClient		*cptr;
int		doall;
char		**comment;
char		*nick;
aConfItem	**iconf;
aConfItem	*aconf;
{

    char	userhost[USERLEN + HOSTLEN + 3];
    char	userip[sizeof(userhost)];
    char 	*host, *ip, *user;
    aConfItem 	*tmp = NULL;
    int		now = 0, check;
    static char reply[256];
    
    if (!cptr->user || !nick || !cptr->name || !iconf || !(*iconf))
	return 0;
    host = cptr->sockhost;
    user = cptr->user->username;
#ifdef INET6
    ip = (char *) inetntop(AF_INET6, (char *)&cptr->ip, mydummy,
			       MYDUMMY_SIZE);
#else
    ip = (char *) inetntoa((char *)&cptr->ip);
#endif

    if (IsRestricted(cptr) && user[0] == '+')
    {
		/*
		** since we added '+' at the begining of valid
		** ident response, remove it here for kline
		** comparison --Beeth
		*/
		user++;
    }

    if (strlen(host)  > (size_t) HOSTLEN ||
        (user ? strlen(user) : 0) > (size_t) USERLEN ||
	strlen(nick) > (size_t) UNINICKLEN)
	return (0);
    sprintf(userhost, "%s@%s", user, host);
    
#if	defined(INET6) && defined(USE_VHOST6)
    if (strstr(host, ip) == host)
#else	/* INET6 && USE_VHOST6 */
    if (!strcmp(host, ip))
#endif	/* INET6 && USE_VHOST6 */
    {
	*userip = '\0'; /* we don't have a name for the ip# */
    } else {
	sprintf(userip, "%s@%s", user, ip);
    }
    
    if ((*iconf)->status & CONF_KILL) {
    /* Check exempts first. */
	for (tmp = econf; tmp; tmp = tmp->next)
	{
	    if (!(tmp->status & CONF_EXEMPT))
		continue; /* should never happen with econf */
	    if (!tmp->host || !tmp->name)
		continue;

	    if (!check_tline_one(cptr, nick, tmp, userhost, userip))
		return 0; /* Exempt found */    
	}
    /* Now check klines */
	if (aconf) {
	    check = check_tline_one(cptr, nick, aconf, userhost, userip);
	    if (check == 0)
		tmp = aconf; /* We do not check time intervals here, only on rehash */
	    else
		tmp = NULL;
	    Debug((DEBUG_INFO, "check_tkline_one(%s!%s) returned %d",
		    aconf->name, aconf->host, check));
	}
	else {
	    for (tmp = *iconf; tmp; tmp = tmp->next)
	    {
		if (!doall && (BadPtr(tmp->passwd) || !isdigit(*tmp->passwd)))
			continue;
		if (!(tmp->status & CONF_KILL))
			continue; /* should never happen with kconf */
		if (!tmp->host || !tmp->name)
			continue;
		check = check_tline_one(cptr, nick, tmp, userhost, userip);
		if (check == 0) {
			now = 0;
			if (!BadPtr(tmp->passwd) && isdigit(*tmp->passwd) &&
			    !(now = check_time_interval(tmp->passwd, reply))) {
				continue;
			}
			if (now == ERR_YOUWILLBEBANNED) {
				tmp = NULL;
			}
			break;
		}
	    }
	}
	if (tmp && !BadPtr(tmp->passwd))
	    *comment = tmp->passwd;
	else
	    *comment = NULL;

	if (*reply)
	    sendto_one(cptr, reply, ME, now, cptr->name);
	else if (tmp)
	    sendto_one(cptr,
			":%s %d %s :You are not welcome to this server%s%s",
			ME, ERR_YOUREBANNEDCREEP, cptr->name,
			   (*comment) ? "" : ": ",
			   (*comment) ? "" : *comment);

	return (tmp ? -1 : 0);
    }
    if ((*iconf)->status & (CONF_RUSNETRLINE)) {
    /* check rlines */
	if (aconf) {
	    check = check_tline_one(cptr, nick, aconf, userhost, userip);
	    if (check == 0)
		tmp = aconf; /* We do not check time intervals here, only on rehash */
	}
	else {
	    for (tmp = *iconf; tmp; tmp = tmp->next)
	    {
		if (!doall && (BadPtr(tmp->passwd) || !isdigit(*tmp->passwd)))
			continue;
		if (!(tmp->status & (CONF_RUSNETRLINE)))
			continue;
		if (!tmp->host || !tmp->name)
			continue;

		check = check_tline_one(cptr, nick, tmp, userhost, userip);
		if (check == 0) {
			now = 0;
			if (!BadPtr(tmp->passwd) && isdigit(*tmp->passwd) &&
			    !(now = check_time_interval(tmp->passwd, reply))) {
				continue;
			}
			if (now == ERR_YOUWILLBEBANNED) {
				tmp = NULL;
			}
			break;
		}
	    }
	}
	if (tmp && !BadPtr(tmp->passwd))
		*comment = tmp->passwd;
	else
		*comment = NULL;

	if (tmp)
		sendto_one(cptr, ":%s %d %s :Your connection is restricted%s%s",
				ME, ERR_RLINED, cptr->name,
				(*comment) ? "" : ": ",
				(*comment) ? "" : *comment);

 	return (tmp ? -1 : 0);
    }

    return 0;
}


/*
 * For type stat, check if both name and host masks match.
 * Return -1 for match, 0 for no-match.
 */
int	find_two_masks(name, host, stat)
char	*name, *host;
u_int	stat;
{
	aConfItem *tmp;

	for (tmp = conf; tmp; tmp = tmp->next)
 		if ((tmp->status == stat) && tmp->host && tmp->name &&
		    (match(tmp->host, host) == 0) &&
 		    (match(tmp->name, name) == 0))
			break;
 	return (tmp ? -1 : 0);
}

/*
 * For type stat, check if name matches and any char from key matches
 * to chars in passwd field.
 * Return -1 for match, 0 for no-match.
 */
int	find_conf_flags(name, key, stat)
char		*name, *key;
unsigned int	stat;
{
	aConfItem *tmp;
	int l;

	if (index(key, '/') == NULL)
		return 0;
	l = ((char *)index(key, '/') - key) + 1;
	for (tmp = conf; tmp; tmp = tmp->next)
 		if ((tmp->status == stat) && tmp->passwd && tmp->name &&
		    (strncasecmp(key, tmp->passwd, l) == 0) &&
		    (match(tmp->name, name) == 0) &&
		    (strpbrk(key + l, tmp->passwd + l)))
			break;
 	return (tmp ? -1 : 0);
}


/*
** check against a set of time intervals
*/

static	int	check_time_interval(interval, reply)
char	*interval, *reply;
{
	struct tm *tptr;
 	char	*p;
 	int	perm_min_hours, perm_min_minutes,
 		perm_max_hours, perm_max_minutes;
 	int	now, perm_min, perm_max;

	tptr = localtime(&timeofday);
 	now = tptr->tm_hour * 60 + tptr->tm_min;

	while (interval)
	    {
		p = (char *)index(interval, ',');
		if (p)
			*p = '\0';
		if (sscanf(interval, "%2d%2d-%2d%2d",
			   &perm_min_hours, &perm_min_minutes,
			   &perm_max_hours, &perm_max_minutes) != 4)
		    {
			if (p)
				*p = ',';
			return(0);
		    }
		if (p)
			*(p++) = ',';
		perm_min = 60 * perm_min_hours + perm_min_minutes;
		perm_max = 60 * perm_max_hours + perm_max_minutes;
           	/*
           	** The following check allows intervals over midnight ...
           	*/
		if ((perm_min < perm_max)
		    ? (perm_min <= now && now <= perm_max)
		    : (perm_min <= now || now <= perm_max))
		    {
			(void)sprintf(reply,
				":%%s %%d %%s :%s %d:%02d to %d:%02d.",
				"You are not allowed to connect from",
				perm_min_hours, perm_min_minutes,
				perm_max_hours, perm_max_minutes);
			return(ERR_YOUREBANNEDCREEP);
		    }
		if ((perm_min < perm_max)
		    ? (perm_min <= now + 5 && now + 5 <= perm_max)
		    : (perm_min <= now + 5 || now + 5 <= perm_max))
		    {
			(void)sprintf(reply, ":%%s %%d %%s :%d minute%s%s",
				perm_min-now,(perm_min-now)>1?"s ":" ",
				"and you will be denied for further access");
			return(ERR_YOUWILLBEBANNED);
		    }
		interval = p;
	    }
	return(0);
}

/*
** find_bounce
**	send a bounce numeric to a client.
**	fd is optional, and only makes sense if positive and when cptr is NULL
**	fd == -1 : not fd, class is a class number.
**	fd == -2 : not fd, class isn't a class number.
*/
int	find_bounce(cptr, class, fd)
aClient *cptr;
int	class, fd;
    {
	Reg	aConfItem	*aconf;

	for (aconf = conf; aconf; aconf = aconf->next)
	    {
		if (aconf->status != CONF_BOUNCE)
			continue;

		if (fd >= 0)
		{
			/*
			** early rejection,
			** connection class and hostname are unknown
			*/
			if (*aconf->host == '\0')
			    {
				char rpl[BUFSIZE];

				sprintf(rpl, rpl_str(RPL_BOUNCE,"unknown"),
					aconf->name, (aconf->port) ?
					aconf->port :
# ifndef USE_OLD8BIT
					find_conf_listener(conv_charset(cptr->conv)));
# else
					rusnet_getclientport(fd));
# endif
				strcat(rpl, "\r\n");
#ifdef INET6
				sendto(fd, rpl, strlen(rpl), 0, 0, 0);
#else
				send(fd, rpl, strlen(rpl), 0);
#endif
				return 1;
			    }
			else
				continue;
		}

		/* fd < 0 */
		/*
		** "too many" type rejection, class is known.
		** check if B line is for a class #,
		** and if it is for a hostname.
		*/
		if (fd != -2 &&
		    !strchr(aconf->host, '.') && isdigit(*aconf->host))
		    {
			if (class != atoi(aconf->host))
				continue;
		    }
		else
			if (strchr(aconf->host, '/'))
			    {
				if (match_ipmask(aconf->host, cptr))
					continue;
			    }
			else if (match(aconf->host, cptr->sockhost))
				continue;

		if (!find_name(aconf->name, NULL))	/* disregard bounce */
			return 0;	/* (server not in network)  --erra */

		sendto_one(cptr, rpl_str(RPL_BOUNCE, cptr->name), aconf->name,
				(!aconf->port) ?
# ifndef USE_OLD8BIT
				find_conf_listener (conv_charset(cptr->conv)) :
# else
				rusnet_getclientport(cptr->acpt->fd) :
# endif
				aconf->port);
		return 1;
	    }
	return -1;
    }

/*
** find_denied
**	for a given server name, make sure no D line matches any of the
**	servers currently present on the net.
*/
aConfItem * find_denied(char *name, int class)
{
    aConfItem	*aconf;

    for (aconf = conf; aconf; aconf = aconf->next)
	{
	    if (aconf->status != CONF_DENY)
		    continue;
	    if (!aconf->name)
		    continue;
	    if (match(aconf->name, name) && aconf->port != class)
		    continue;
	    if (isdigit(*aconf->passwd))
		{
		    aConfItem	*aconf2;
		    int		ck = atoi(aconf->passwd);

		    for (aconf2 = conf; aconf2; aconf2 = aconf2->next)
			{
			    if (aconf2->status != CONF_NOCONNECT_SERVER)
				    continue;
			    if (!aconf2->class || ConfClass(aconf2) != ck)
				    continue;
			    if (find_client(aconf2->host, NULL))
				    return aconf2;
			}
		}
	    if (aconf->host)
		{
		    aServer	*asptr;

		    for (asptr = svrtop; asptr; asptr = asptr->nexts)
			    if (aconf->host &&
				!match(aconf->host, asptr->bcptr->name))
				    return aconf;
		}
	}
    return NULL;
}
