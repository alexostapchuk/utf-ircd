/*
 *   IRC - Internet Relay Chat, ircd/s_serv.c (formerly ircd/s_msg.c)
 *   Copyright (C) 1990 Jarkko Oikarinen and
 *                      University of Oulu, Computing Center
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
#define S_SERV_C
#include "s_externs.h"
#undef S_SERV_C

#ifndef USE_OLD8BIT
static	char	buf[MB_LEN_MAX*BUFSIZE];
#else
static	char	buf[BUFSIZE];
#endif

static	int	check_link(aClient *);
static  void    report_listeners(aClient *, char *);

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
** m_version
**	parv[0] = sender prefix
**	parv[1] = remote server
*/
int	m_version(cptr, sptr, parc, parv)
aClient *sptr, *cptr;
int	parc;
char	*parv[];
{
#ifdef RUSNET_RLINES
	if (MyClient(sptr) && IsRMode(sptr)) {
		sendto_one(sptr, err_str(ERR_RESTRICTED, parv[0]));
		return 5;
	}
#endif
	if (hunt_server(cptr,sptr,":%s VERSION :%s",1,parc,parv) == HUNTED_ISME)
		sendto_one(sptr, rpl_str(RPL_VERSION, parv[0]),
			   version, debugmode, ME, serveropts);
	return 2;
}

/*
** m_squit
**	parv[0] = sender prefix
**	parv[1] = server name
**	parv[2] = comment
*/
int	m_squit(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
    {
	Reg	aConfItem *aconf;
	char	*server;
	Reg	aClient	*acptr;
	char	*comment = (parc > 2 && parv[2]) ? parv[2] : cptr->name;

	if (parc > 1)
	    {
		server = parv[1];
		/*
		** To accomodate host masking, a squit for a masked server
		** name is expanded if the incoming mask is the same as
		** the server name for that link to the name of link.
		*/
		while ((*server == '*') && IsServer(cptr))
		    {
			aconf = cptr->serv->nline;
			if (!aconf)
				break;
			if (!strcasecmp(server,
				   my_name_for_link(ME, aconf->port)))
				server = cptr->name;
			break; /* WARNING is normal here */
		    }
		/*
		** The following allows wild cards in SQUIT. Only usefull
		** when the command is issued by an oper.
		*/
		for (acptr = client; (acptr = next_client(acptr, server));
		     acptr = acptr->next)
			if (IsServer(acptr) || IsMe(acptr))
				break;
		if (acptr && IsMe(acptr))
		    {
			acptr = cptr;
			server = cptr->sockhost;
		    }
	    }
	else
	    {
		/*
		** This is actually protocol error. But, well, closing
		** the link is very proper answer to that...
		*/
		server = cptr->name;
		acptr = cptr;
	    }

	/*
	** SQUIT semantics is tricky, be careful...
	**
	** The old (irc2.2PL1 and earlier) code just cleans away the
	** server client from the links (because it is never true
	** "cptr == acptr".
	**
	** This logic here works the same way until "SQUIT host" hits
	** the server having the target "host" as local link. Then it
	** will do a real cleanup spewing SQUIT's and QUIT's to all
	** directions, also to the link from which the orinal SQUIT
	** came, generating one unnecessary "SQUIT host" back to that
	** link.
	**
	** One may think that this could be implemented like
	** "hunt_server" (e.g. just pass on "SQUIT" without doing
	** nothing until the server having the link as local is
	** reached). Unfortunately this wouldn't work in the real life,
	** because either target may be unreachable or may not comply
	** with the request. In either case it would leave target in
	** links--no command to clear it away. So, it's better just
	** clean out while going forward, just to be sure.
	**
	** ...of course, even better cleanout would be to QUIT/SQUIT
	** dependant users/servers already on the way out, but
	** currently there is not enough information about remote
	** clients to do this...   --msa
	*/
	if (!acptr)
	    {
		sendto_one(sptr, err_str(ERR_NOSUCHSERVER, parv[0]), server);
		return 1;
	    }
	if (MyConnect(sptr) && !MyConnect(acptr) && parc < 3)
	    {
                sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS,parv[0]), "SQUIT");
		return 0;
            }
	if (IsLocOp(sptr) && !MyConnect(acptr))
	    {
		sendto_one(sptr, err_str(ERR_NOPRIVILEGES, parv[0]));
		return 1;
	    }
	if ((cptr != acptr->from)  && IsServer(cptr) &&
		cptr->serv->nline->localpref) {
#ifdef WALLOPS_TO_CHANNEL
		sendto_serv_butone(NULL, ":%s WALLOPS : %s from %s issued remote SQUIT %s :%s",
				ME, sptr->name, sptr->from->name, acptr->name, comment);

		sendto_flag(SCH_WALLOPS, "%s: %s from %s issued remote SQUIT %s :%s",
			    ME, sptr->name, sptr->from->name, acptr->name, comment);
#else
		sendto_ops_butone(NULL, &me,
			":%s WALLOPS :%s from %s issued remote SQUIT %s :%s",
			ME, sptr->name, sptr->from->name, acptr->name, comment);
#endif
		/*
		** At this point damage is already done to sptr->from.
		** We have no other options but to squit cptr.
		*/
		return exit_client(cptr, cptr, &me, "Unauthorized remote squit from test server");
	}

	if (!MyConnect(acptr) && (cptr != acptr->from))
	    {
		/* propagate SQUIT to upstream */
		sendto_one(acptr->from, ":%s SQUIT %s :%s", parv[0],
			   acptr->name, comment);
		sendto_flag(SCH_SERVER,
			    "Forwarding SQUIT %s from %s (%s)",
			    acptr->name, parv[0], comment);
		sendto_flag(SCH_DEBUG,
			    "Forwarding SQUIT %s to %s from %s (%s)",
			    acptr->name, acptr->from->name, 
			    parv[0], comment);
		return 1;
	    }
	/*
	**  Notify all opers, if my local link is remotely squitted
	*/
	if (MyConnect(acptr) && !IsAnOper(cptr))
	    {
#ifdef WALLOPS_TO_CHANNEL
		sendto_serv_butone(NULL,
			":%s WALLOPS :Received SQUIT %s from %s (%s)",
			ME, server, parv[0], comment);
		sendto_flag(SCH_WALLOPS, "SQUIT From %s : %s (%s)",
		       parv[0], server, comment);
#else
		sendto_ops_butone(NULL, &me,
			":%s WALLOPS :Received SQUIT %s from %s (%s)",
			ME, server, parv[0], comment);
		sendto_flag(SCH_DEBUG,"SQUIT From %s : %s (%s)",
		       parv[0], server, comment);
#endif
	    }
	if (MyConnect(acptr))
	    {
		int timeconnected = timeofday - acptr->firsttime;
		sendto_flag(SCH_NOTICE, 
			    "Closing link to %s (%d, %2d:%02d:%02d)",
			    get_client_name(acptr, FALSE),
			    timeconnected / 86400,
			    (timeconnected % 86400) / 3600,
			    (timeconnected % 3600)/60, 
			    timeconnected % 60);
	    }
	sendto_flag(SCH_SERVER, "Received SQUIT %s from %s (%s)",
		    acptr->name, parv[0], comment);

	/*
	 * The following code seems pretty useless, but we'll keep it
	 * for a while just in case if something happens  --erra
	 */
#if 0
	if (MyConnect(acptr) && 
	    IsServer(cptr) && (cptr->serv->version & SV_UNKNOWN) == 0)
	    {
		sendto_one(cptr, ":%s SQUIT %s :%s", ME, acptr->name, comment);
		sendto_flag(SCH_DEBUG, "Issuing additionnal SQUIT %s for %s",
			    acptr->name, acptr->from->name);
	    }
#endif
	return exit_client(cptr, acptr, sptr, comment);
    }

/*
** check_version
**      The PASS command delivers additional information about incoming
**	connection. The data is temporarily stored to info/name/username
**	in m_pass() and processed here before the fields are natively used.
** Return: < 1: exit/error, > 0: no error
*/
int	check_version(cptr)
aClient	*cptr;
{
	char *id, *misc = NULL, *link = NULL;

	Debug((DEBUG_INFO,"check_version: %s", cptr->info));

	if (cptr->info == DefInfo)
	    {
		cptr->hopcount = SV_UNKNOWN;
		return 1; /* no version checked (e.g. older than RusNet 1.4) */
	    }
	if ((id = index(cptr->info, ' ')))
	    {
		*id++ = '\0';
		if ((link = index(id, ' ')))
			*link++ = '\0';
		if ((misc = index(id, '|')))
			*misc++ = '\0';
		else
		    {
			misc = id;
			id = "";
		    }
	    }
	else
		id = "";

	if (!strncmp(cptr->info, "021", 3) && cptr->info[6] >= '1')
	{
		cptr->hopcount = SV_RUSNET1;

		if (cptr->info[6] >= '2')	/* RusNet 2.0 and later */
			cptr->hopcount = SV_RUSNET2;
#ifndef USE_OLD8BIT
		conv_free_conversion(cptr->conv); /* was added by add_connection() */
		/* check if we have to use unicode now:
		   if we started connection then FLAGS_UNICODE may be set
		   if we accepted connection then we could agree to it */
		if (((cptr->flags & FLAGS_UNICODE) || UseUnicode) &&
		    link && strchr(link, 'U'))
		{
			cptr->flags |= FLAGS_UNICODE;
			cptr->conv = conv_get_conversion(CHARSET_UNICODE);
		}
		else
			cptr->conv = conv_get_conversion(CHARSET_8BIT);
#endif
	}
	else
		cptr->hopcount = SV_UNKNOWN; /* uhuh */

	/* Check version number/mask from conf */
	sprintf(buf, "%s/%s", id, cptr->info);
	if (find_two_masks(cptr->name, buf, CONF_VER))
	    {
		sendto_flag(SCH_ERROR, "Bad version %s %s from %s", id,
			    cptr->info, get_client_name(cptr, TRUE));
		return exit_client(cptr, cptr, &me, "Bad version");
	    }

	if (misc)
	    {
		sprintf(buf, "%s/%s", id, misc);
		/* Check version flags from conf */
		if (find_conf_flags(cptr->name, buf, CONF_VER))
		    {
			sendto_flag(SCH_ERROR, "Bad flags %s (%s) from %s",
				    misc, id, get_client_name(cptr, TRUE));
			return exit_client(cptr, cptr, &me, "Bad flags");
		    }
	    }

	/* right now, I can't code anything good for this */
	/* Stop whining, and do it! ;) */
	if (link && strchr(link, 'Z'))	/* Compression requested */
                cptr->flags |= FLAGS_ZIPRQ;
	/*
	 * If server was started with -p strict, be careful about the
	 * other server mode. Isn't it just an old piece of crap? --erra
	 */
#if 0
	if (link && strncmp(cptr->info, "020", 3) &&
	    (bootopt & BOOT_STRICTPROT) && !strchr(link, 'P'))
		return exit_client(cptr, cptr, &me, "Unsafe mode");
#endif

	return 2;
}

/*
** m_server
**	parv[0] = sender prefix
**	parv[1] = servername
**	parv[2] = serverinfo/hopcount
**	parv[3] = token/serverinfo (2.9)
**      parv[4] = serverinfo
*/
int	m_server(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	Reg	char	*ch;
	Reg	int	i;
	char	info[HOSTLEN+REALLEN+4], *inpath, *host, *stok;
	aClient *acptr, *bcptr;
	aConfItem *aconf;
	int	hop = 0, token = 0;

	if (sptr->user) /* in case NICK hasn't been received yet */
            {
                sendto_one(sptr, err_str(ERR_ALREADYREGISTRED, parv[0]));
                return 1;
            }
	/* note: we hope here that server description has only 8bit chars */
	info[0] = info[sizeof(info)-1] = '\0';  /* strncpy() doesn't guarantee NULL */
	inpath = get_client_name(cptr, FALSE);
	if (parc < 2 || *parv[1] == '\0')
	    {
			sendto_one(cptr,"ERROR :No servername");
			return 1;
	    }
	host = parv[1];
	if (parc > 3 && (hop = atoi(parv[2])))
	    {
		if (parc > 4 && (token = atoi(parv[3])))
			(void)strncpy(info, parv[4], sizeof(info)-1);
		else
			(void)strncpy(info, parv[3], sizeof(info)-1);
	    }
	else if (parc > 2)
	    {
		(void)strncpy(info, parv[2], sizeof(info)-1);
		i = strlen(info);
		if (parc > 3 && ((unsigned)(i + 2) < sizeof(info)-1))
		    {
			info[i] = ' ';
                	(void)strncpy(info + i + 1, parv[3], sizeof(info) - i - 2);
		    }
	    }
	/*
	** Check for "FRENCH " infection ;-) (actually this should
	** be replaced with routine to check the hostname syntax in
	** general). [ This check is still needed, even after the parse
	** is fixed, because someone can send "SERVER :foo bar " ].
	** Also, changed to check other "difficult" characters, now
	** that parse lets all through... --msa
	*/
	if (strlen(host) > (size_t) HOSTLEN)
		host[HOSTLEN] = '\0';
	for (ch = host; *ch; ch++)
		if (*ch <= ' ' || *ch > '~')
			break;
	if (*ch || !index(host, '.'))
	    {
		sendto_one(sptr,"ERROR :Bogus server name (%s)", host);
		sendto_flag(SCH_ERROR, "Bogus server name (%s) from %s", host,
			    get_client_name(cptr, TRUE));
		return 2;
	    }
	
	/* *WHEN* can it be that "cptr != sptr" ????? --msa */
	/* When SERVER command (like now) has prefix. -avalon */

	if (IsRegistered(cptr) && ((acptr = find_name(host, NULL))
				   || (acptr = find_mask(host, NULL))))
	    {
		/*
		** This link is trying feed me a server that I already have
		** access through another path -- multiple paths not accepted
		** currently, kill this link immeatedly!!
		**
		** Rather than KILL the link which introduced it, KILL the
		** youngest of the two links. -avalon
		*/
		bcptr = (cptr->firsttime > acptr->from->firsttime) ? cptr :
			acptr->from;
		sendto_one(bcptr, "ERROR :Server %s already exists", host);
		/* in both cases the bcptr (the youngest is killed) */
		if (bcptr == cptr)
		    {
			sendto_flag(SCH_ERROR,
			    "Link %s cancelled, server %s already exists",
				    get_client_name(bcptr, TRUE), host);
			return exit_client(bcptr, bcptr, &me, "Server Exists");
		    }
		else
		    {
			/*
			** in this case, we are not dropping the link from
			** which we got the SERVER message.  Thus we canNOT
			** `return' yet! -krys
			*/
			strcpy(buf, get_client_name(bcptr, TRUE));
			sendto_flag(SCH_ERROR,
			    "Link %s cancelled, server %s reintroduced by %s",
				    buf, host, get_client_name(cptr, TRUE));
			(void) exit_client(bcptr, bcptr, &me, "Server Exists");
		    }
	    }
	if ((acptr = find_person(host, NULL)) && (acptr != cptr))
	    {
		/*
		** Server trying to use the same name as a person. Would
		** cause a fair bit of confusion. Enough to make it hellish
		** for a while and servers to send stuff to the wrong place.
		*/
		sendto_one(cptr,"ERROR :Nickname %s already exists!", host);
		sendto_flag(SCH_ERROR,
			    "Link %s cancelled: Server/nick collision on %s",
			    inpath, host);
		sendto_serv_butone(NULL, /* all servers */
				   ":%s KILL %s :%s (%s <- %s)",
				   ME, acptr->name, ME,
				   acptr->from->name, host);
		acptr->flags |= FLAGS_KILLED;
		(void)exit_client(NULL, acptr, &me, "Nick collision");
		return exit_client(cptr, cptr, &me, "Nick as Server");
	    }

	if (IsServer(cptr))
	    {
		/* A server can only be introduced by another server. */
		if (!IsServer(sptr))
		    {
			sendto_flag(SCH_LOCAL,
			    "Squitting %s brought by %s (introduced by %s)",
				    host, get_client_name(cptr, FALSE),
				    sptr->name);
			sendto_one(cptr,
				   ":%s SQUIT %s :(Introduced by %s from %s)",
				   me.name, host, sptr->name,
				   get_client_name(cptr, FALSE));
	  		return 1;
		    }
		/*
		** Server is informing about a new server behind
		** this link. Create REMOTE server structure,
		** add it to list and propagate word to my other
		** server links...
		*/
		if (parc == 1 || info[0] == '\0')
		    {
	  		sendto_one(cptr,
				   "ERROR :No server info specified for %s",
				   host);
			sendto_flag(SCH_ERROR, "No server info for %s from %s",
				    host, get_client_name(cptr, TRUE));
	  		return 1;
		    }

		/*
		** See if the newly found server is behind a guaranteed
		** leaf (L-line). If so, close the link.
		*/
		if ((aconf = find_conf_host(cptr->confs, host, CONF_LEAF)) &&
		    (!aconf->port || (hop > aconf->port)))
		    {
	      		sendto_flag(SCH_ERROR,
				    "Leaf-only link %s->%s - Closing",
				    get_client_name(cptr, TRUE),
				    aconf->host ? aconf->host : "*");
	      		sendto_one(cptr, "ERROR :Leaf-only link, sorry.");
      			return exit_client(cptr, cptr, &me, "Leaf Only");
		    }
		/*
		**
		*/
		if (!(aconf = find_conf_host(cptr->confs, host, CONF_HUB)) ||
		    (aconf->port && (hop > aconf->port)) )
		    {
			sendto_flag(SCH_ERROR,
				    "Non-Hub link %s introduced %s(%s).",
				    get_client_name(cptr, TRUE), host,
				   aconf ? (aconf->host ? aconf->host : "*") :
				   "!");
			return exit_client(cptr, cptr, &me,
					   "Too many servers");
		    }
		/*
		** See if the newly found server has a Q line for it in
		** our conf. If it does, lose the link that brought it
		** into our network. Format:
		**
		** Q:<unused>:<reason>:<servername>
		**
		** Example:  Q:*:for the hell of it:eris.Berkeley.EDU
		*/
		if ((aconf = find_conf_name(host, CONF_QUARANTINED_SERVER)))
		    {

#ifdef WALLOPS_TO_CHANNEL
			sendto_serv_butone(NULL,
				":%s WALLOPS * :%s brought in %s, %s %s",
				ME, get_client_name(cptr, TRUE),
				host, "closing link because",
				BadPtr(aconf->passwd) ? "reason unspecified" :
				aconf->passwd);
			sendto_flag(SCH_WALLOPS, "%s brought in %s, %s %s",
				get_client_name(cptr, TRUE), host,
				"closing link:", BadPtr(aconf->passwd) ?
				"reason unspecified" : aconf->passwd);
#else
			sendto_ops_butone(NULL, &me,
				":%s WALLOPS * :%s brought in %s, %s %s",
				ME, get_client_name(cptr, TRUE),
				host, "closing link because",
				BadPtr(aconf->passwd) ? "reason unspecified" :
				aconf->passwd);
#endif

			sendto_one(cptr,
				   "ERROR :%s is not welcome: %s. %s",
				   host, BadPtr(aconf->passwd) ?
				   "reason unspecified" : aconf->passwd,
				   "Go away and get a life");

			return exit_client(cptr, cptr, &me, "Q-Lined Server");
		    }

		acptr = make_client(cptr);
		(void)make_server(acptr);
		acptr->hopcount = hop;
		strncpyzt(acptr->name, host, sizeof(acptr->name));
		acptr->serv->crc = gen_crc(host);

		if (acptr->info != DefInfo)
			MyFree(acptr->info);
		acptr->info = mystrdup(info);
		acptr->serv->up = sptr;
		acptr->serv->stok = token;
		acptr->serv->snum = find_server_num(acptr->name);
		SetServer(acptr);
		istat.is_serv++;
#ifdef EXTRA_STATISTICS
		/*
		**  Keep track of the highest number of connected
		**  servers (hostmasks) to this network.
		**/
		if (istat.is_serv > istat.is_m_serv)
			istat.is_m_serv = istat.is_serv;
#endif
		add_client_to_list(acptr);
		(void)add_to_client_hash_table(acptr->name, acptr);
		(void)add_to_server_hash_table(acptr->serv, cptr);
		/*
		** Old sendto_serv_but_one() call removed because we now
		** need to send different names to different servers
		** (domain name matching)
		*/
		for (i = fdas.highest; i >= 0; i--)
		    {
			if (!(bcptr = local[fdas.fd[i]]) || !IsServer(bcptr) ||
			    bcptr == cptr || IsMe(bcptr))
				continue;
			if (!(aconf = bcptr->serv->nline))
			    {
				sendto_flag(SCH_NOTICE,
					    "Lost N-line for %s on %s:Closing",
					    get_client_name(cptr, TRUE), host);
				return exit_client(cptr, cptr, &me,
						   "Lost N line");
			    }
			if (match(my_name_for_link(ME, aconf->port),
				    acptr->name) == 0)
				continue;
			stok = acptr->serv->tok;
			sendto_one(bcptr, ":%s SERVER %s %d %s :%s", parv[0],
				   acptr->name, hop+1, stok, acptr->info);
		    }
#ifdef	USE_SERVICES
		check_services_butone(SERVICE_WANT_SERVER, acptr->name, acptr,
				      ":%s SERVER %s %d %s :%s", parv[0],
				      acptr->name, hop+1, acptr->serv->tok,
				      acptr->info);
#endif
		sendto_flag(SCH_SERVER, "Received SERVER %s from %s (%d %s)",
			    acptr->name, parv[0], hop+1, acptr->info);
		return 0;
	    }

	if ((!IsUnknown(cptr) && !IsHandshake(cptr)) ||
	    (cptr->flags & FLAGS_UNKCMD))
		return 1;
	/*
	** A local link that is still in undefined state wants
	** to be a SERVER. Check if this is allowed and change
	** status accordingly...
	*/
	strncpyzt(cptr->name, host, sizeof(cptr->name));
	/* cptr->name has to exist before check_version(), and cptr->info
	 * may not be filled before check_version(). */
	if ((hop = check_version(cptr)) < 1)
		return hop;	/* from exit_client() */
	if (cptr->info != DefInfo)
		MyFree(cptr->info);
	cptr->info = mystrdup(info[0] ? info : ME);

	switch (check_server_init(cptr))
	{
	case 0 :
		return m_server_estab(cptr);
	case 1 :
		sendto_flag(SCH_NOTICE, "Checking access for %s",
			    get_client_name(cptr,TRUE));
		return 1;
	default :
		ircstp->is_ref++;
		sendto_flag(SCH_NOTICE, "Unauthorized server from %s.",
			    get_client_host(cptr));
		return exit_client(cptr, cptr, &me, "No C/N conf lines");
	}
}

int	m_server_estab(cptr)
Reg	aClient	*cptr;
{
	Reg	aClient	*acptr;
	Reg	aConfItem	*aconf, *bconf;
	char	mlname[HOSTLEN+1], *inpath, *host, *s, *encr, *stok;
	int	split, i;

	host = cptr->name;
	inpath = get_client_name(cptr,TRUE); /* "refresh" inpath with host */
	split = strcasecmp(cptr->name, cptr->sockhost);

	if (!(aconf = find_conf(cptr->confs, host, CONF_NOCONNECT_SERVER)))
	    {
		ircstp->is_ref++;
		sendto_one(cptr,
			   "ERROR :Access denied. No N line for server %s",
			   inpath);
		sendto_flag(SCH_ERROR,
			    "Access denied. No N line for server %s", inpath);
		return exit_client(cptr, cptr, &me, "No N line for server");
	    }
	if (!(bconf = find_conf(cptr->confs, host, CONF_CONNECT_SERVER|
				CONF_ZCONNECT_SERVER)))
	    {
		ircstp->is_ref++;
		sendto_one(cptr, "ERROR :Only N (no C) field for server %s",
			   inpath);
		sendto_flag(SCH_ERROR,
			    "Only N (no C) field for server %s",inpath);
		return exit_client(cptr, cptr, &me, "No C line for server");
	    }

	if (cptr->hopcount == SV_UNKNOWN) /* lame test, should be == 0 */
	    {
		sendto_one(cptr, "ERROR :Server version is unknown.");
		sendto_flag(SCH_ERROR, "Weird version for %s", inpath);
		return exit_client(cptr, cptr, &me, "Weird version");
	    }

#ifdef CRYPT_LINK_PASSWORD
	/* pass whole aconf->passwd as salt, let crypt() deal with it */

	if (*cptr->passwd)
	    {
		extern  char *crypt();

		encr = crypt(cptr->passwd, aconf->passwd);
		if (encr == NULL)
		    {
			ircstp->is_ref++;
			sendto_one(cptr, "ERROR :No Access (crypt failed) %s",
			  	inpath);
			sendto_flag(SCH_ERROR,
			    	"Access denied (crypt failed) %s", inpath);
			return exit_client(cptr, cptr, &me, "Bad Password");
		    }
	    }
	else
		encr = "";
#else
	encr = cptr->passwd;
#endif  /* CRYPT_LINK_PASSWORD */
	if (*aconf->passwd && !StrEq(aconf->passwd, encr))
	    {
		ircstp->is_ref++;
		sendto_one(cptr, "ERROR :No Access (passwd mismatch) %s",
			   inpath);
		sendto_flag(SCH_ERROR,
			    "Access denied (passwd mismatch) %s", inpath);
		return exit_client(cptr, cptr, &me, "Bad Password");
	    }
	bzero(cptr->passwd, sizeof(cptr->passwd));

#ifndef	HUB
	for (i = 0; i <= highest_fd; i++)
		if (local[i] && IsServer(local[i]))
		    {
			ircstp->is_ref++;
			sendto_flag(SCH_ERROR, "I'm a leaf, cannot link %s",
				    get_client_name(cptr, TRUE));
			return exit_client(cptr, cptr, &me, "I'm a leaf");
		    }
#endif
	(void) strcpy(mlname, my_name_for_link(ME, aconf->port));
	if (IsUnknown(cptr))
	    {
		if (bconf->passwd[0])
			sendto_one(cptr, "PASS %s %s IRC|%s %s"
#ifdef	ZIP_LINKS
			"%s"
#endif
#ifndef USE_OLD8BIT
			"%s"
#endif
			, bconf->passwd, pass_version, serveropts,
#ifdef	ZIP_LINKS
			   (bconf->status == CONF_ZCONNECT_SERVER) ? "Z" : "",
#endif
				   (bootopt & BOOT_STRICTPROT) ? "P" : ""
#ifndef USE_OLD8BIT
				   , (cptr->flags & FLAGS_UNICODE) ? "U" : ""
#endif
				   );
		/*
		** Pass my info to the new server
		*/
		sendto_one(cptr, "SERVER %s 1 :%s", mlname, me.info);

		/*
		** If we get a connection which has been authorized to be
		** an already existing connection, remove the already
		** existing connection if it has a sendq else remove the
		** new and duplicate server. -avalon
		** Remove existing link only if it has been linked for longer
		** and has sendq higher than a threshold. -Vesa
		*/
		if ((acptr = find_name(host, NULL))
		    || (acptr = find_mask(host, NULL)))
		    {
			if (MyConnect(acptr) &&
			    DBufLength(&acptr->sendQ) > CHREPLLEN &&
			    timeofday - acptr->firsttime > TIMESEC)
				(void) exit_client(acptr, acptr, &me,
						   "New Server");
			else
				return exit_client(cptr, cptr, &me,
						   "Server Exists");
		    }
	    }
	else
	    {
		s = (char *)index(aconf->host, '@');
		*s = '\0'; /* should never be NULL */
		Debug((DEBUG_INFO, "Check Usernames [%s]vs[%s]",
			aconf->host, cptr->username));
		if (match(aconf->host, cptr->username))
		    {
			*s = '@';
			ircstp->is_ref++;
			sendto_flag(SCH_ERROR,
				    "Username mismatch [%s]v[%s] : %s",
				    aconf->host, cptr->username,
				    get_client_name(cptr, TRUE));
			sendto_one(cptr, "ERROR :No Username Match");
			return exit_client(cptr, cptr, &me, "Bad User");
		    }
		*s = '@';
	    }

#ifdef	ZIP_LINKS
	if ((cptr->flags & FLAGS_ZIPRQ) &&
	    (bconf->status == CONF_ZCONNECT_SERVER))	    
	    {
		if (zip_init(cptr) == -1)
		    {
			zip_free(cptr);
			sendto_flag(SCH_ERROR,
			    "Unable to setup compressed link for %s",
				    get_client_name(cptr, TRUE));
			return exit_client(cptr, cptr, &me,
					   "zip_init() failed");
		    }
		cptr->flags |= FLAGS_ZIP|FLAGS_ZIPSTART;
	    }
#endif

	det_confs_butmask(cptr, CONF_LEAF|CONF_HUB|CONF_NOCONNECT_SERVER);
	/*
	** *WARNING*
	** 	In the following code in place of plain server's
	**	name we send what is returned by get_client_name
	**	which may add the "sockhost" after the name. It's
	**	*very* *important* that there is a SPACE between
	**	the name and sockhost (if present). The receiving
	**	server will start the information field from this
	**	first blank and thus puts the sockhost into info.
	**	...a bit tricky, but you have been warned, besides
	**	code is more neat this way...  --msa
	*/
	SetServer(cptr);
	istat.is_unknown--;
	istat.is_serv++;
	istat.is_myserv++;
#ifdef EXTRA_STATISTICS
	/*
	**  Keep track of the highest number of connected
	**  servers to this network as well as the servers
	**  connected to me
	**/
	if (istat.is_serv > istat.is_m_serv)
		istat.is_m_serv = istat.is_serv;
	if (istat.is_myserv > istat.is_m_myserv)
		istat.is_m_myserv = istat.is_myserv;
#endif
	nextping = timeofday;
	sendto_flag(SCH_NOTICE, "Link with %s established. (%X%s)", inpath,
		    cptr->hopcount, (cptr->flags & FLAGS_ZIP) ? "z" : "");
	(void)add_to_client_hash_table(cptr->name, cptr);
	/* doesnt duplicate cptr->serv if allocted this struct already */
	(void)make_server(cptr);
	cptr->serv->up = &me;
	cptr->serv->nline = aconf;
	cptr->serv->version = cptr->hopcount;   /* temporary location */
	cptr->hopcount = 1;			/* local server connection */
	cptr->serv->snum = find_server_num(cptr->name);
	cptr->serv->stok = 1;
	cptr->serv->crc = gen_crc(cptr->name);
	cptr->flags |= FLAGS_CBURST;
	(void) add_to_server_hash_table(cptr->serv, cptr);
	Debug((DEBUG_NOTICE, "Server link established with %s V%X %d",
		cptr->name, cptr->serv->version, cptr->serv->stok));
	add_fd(cptr->fd, &fdas);
	/*
	** Remove all NDELAY locks depends on this server. -kmale
	*/
	release_locks_byserver(cptr->name,(long)(DELAYCHASETIMELIMIT));
#ifdef	USE_SERVICES
	check_services_butone(SERVICE_WANT_SERVER, cptr->name, cptr,
			      ":%s SERVER %s %d %s :%s", ME, cptr->name,
			      cptr->hopcount+1, cptr->serv->tok, cptr->info);
#endif
	sendto_flag(SCH_SERVER, "Sending SERVER %s (%d %s)", cptr->name,
		    1, cptr->info);

	/*
	** Old sendto_serv_but_one() call removed because we now
	** need to send different names to different servers
	** (domain name matching) Send new server to other servers.
	*/
	for (i = fdas.highest; i >= 0; i--)
	    {
		if (!(acptr = local[fdas.fd[i]]) || !IsServer(acptr) ||
		    acptr == cptr || IsMe(acptr))		    
			continue;
		if ((aconf = acptr->serv->nline) &&
		    !match(my_name_for_link(ME, aconf->port), cptr->name))
			continue;
		stok = cptr->serv->tok;
		if (split)
			sendto_one(acptr,":%s SERVER %s 2 %s :[%s] %s",
				   ME, cptr->name, stok,
				   cptr->sockhost, cptr->info);
		else
			sendto_one(acptr,":%s SERVER %s 2 %s :%s",
				   ME, cptr->name, stok, cptr->info);
	    }
	/*
	** Pass on my client information to the new server
	**
	** First, pass only servers (idea is that if the link gets
	** cancelled beacause the server was already there,
	** there are no NICK's to be cancelled...). Of course,
	** if cancellation occurs, all this info is sent anyway,
	** and I guess the link dies when a read is attempted...? --msa
	** 
	** Note: Link cancellation to occur at this point means
	** that at least two servers from my fragment are building
	** up connection this other fragment at the same time, it's
	** a race condition, not the normal way of operation...
	**
	** ALSO NOTE: using the get_client_name for server names--
	**	see previous *WARNING*!!! (Also, original inpath
	**	is destroyed...)
	*/
	aconf = cptr->serv->nline;
	for (acptr = &me; acptr; acptr = acptr->prev)
	    {
		/* acptr->from == acptr for acptr == cptr */
		if ((acptr->from == cptr) || !IsServer(acptr))
			continue;
		if (*mlname == '*' && match(mlname, acptr->name) == 0)
			continue;
		stok = acptr->serv->tok;

		/*
		 * The following piece of code was marked as ifndef RUSNET_IRCD
		 * While it is completely safe to remove it, it would be nice
		 * to check it for something potentially useful first  --erra
		 */
#if 0
		split = (MyConnect(acptr) &&
			 strcasecmp(acptr->name, acptr->sockhost));
		if (split)
			sendto_one(cptr, ":%s SERVER %s %d %s :[%s] %s",
				   acptr->serv->up->name,
				   acptr->name, acptr->hopcount+1, stok,
				   acptr->sockhost, acptr->info);
		else
#endif
			sendto_one(cptr, ":%s SERVER %s %d %s :%s",
				   acptr->serv->up->name, acptr->name,
				   acptr->hopcount+1, stok, acptr->info);
	    }

	for (acptr = &me; acptr; acptr = acptr->prev)
	    {
		/* acptr->from == acptr for acptr == cptr */
		if (acptr->from == cptr)
			continue;
		if (IsPerson(acptr))
		    {
			/*
			** IsPerson(x) is true only when IsClient(x) is true.
			** These are only true when *BOTH* NICK and USER have
			** been received. -avalon
			*/
			if (*mlname == '*' &&
			    match(mlname, acptr->user->server) == 0)
				stok = me.serv->tok;
			else
				stok = acptr->user->servp->tok;
			send_umode(NULL, acptr, 0, SEND_UMODES, buf);
			sendto_one(cptr,"NICK %s %d %s %s %s %s :%s",
				   acptr->name, acptr->hopcount + 1,
				   acptr->user->username, acptr->sockhost,
				   stok, (*buf) ? buf : "+", acptr->info);
		    }
		else if (IsService(acptr) &&
			 match(acptr->service->dist, cptr->name) == 0)
		    {
			if (*mlname == '*' &&
			    match(mlname, acptr->service->server) == 0)
				stok = me.serv->tok;
			else
				stok = acptr->service->servp->tok;
			sendto_one(cptr, "SERVICE %s %s %s %d %d :%s",
				   acptr->name, stok, acptr->service->dist,
				   acptr->service->type, acptr->hopcount + 1,
				   acptr->info);
		    }
		/* the previous if does NOT catch all services.. ! */
	    }

	flush_connections(cptr->fd);

	/*
	** Last, pass all channels modes
	** only sending modes for LIVE channels.
	*/
	    {
		Reg	aChannel *chptr;
		for (chptr = channel; chptr; chptr = chptr->nextch)
			if (chptr->users)
			    {
				send_channel_members(cptr, chptr);
				send_channel_modes(cptr, chptr);
				send_channel_topic(cptr, chptr);
			    }
	    }

	cptr->flags &= ~FLAGS_CBURST;
#ifdef	ZIP_LINKS
 	/*
 	** some stats about the connect burst,
 	** they are slightly incorrect because of cptr->zip->outbuf.
 	*/
 	if ((cptr->flags & FLAGS_ZIP) && cptr->zip->out->total_in)
	  sendto_flag(SCH_NOTICE,
		      "Connect burst to %s: %lu, compressed: %lu (%3.1f%%)",
		      get_client_name(cptr, TRUE),
		      cptr->zip->out->total_in,cptr->zip->out->total_out,
 	    (float) 100*cptr->zip->out->total_out/cptr->zip->out->total_in);
#endif
	return 0;
}


/*
** m_info
**	parv[0] = sender prefix
**	parv[1] = servername
*/
int	m_info(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	char **text = infotext;

#ifdef RUSNET_RLINES
	if (MyClient(sptr) && IsRMode(sptr)) {
		sendto_one(sptr, err_str(ERR_RESTRICTED, parv[0]));
		return 10;
	}
#endif
	if (IsServer(cptr) && check_link(cptr))
	    {
		sendto_one(sptr, rpl_str(RPL_TRYAGAIN, parv[0]),
			   "INFO");
		return 5;
	    }
	if (hunt_server(cptr,sptr,":%s INFO :%s",1,parc,parv) == HUNTED_ISME)
	    {
		while (*text)
			sendto_one(sptr, rpl_str(RPL_INFO, parv[0]), *text++);

		sendto_one(sptr, rpl_str(RPL_INFO, parv[0]), "");
		sendto_one(sptr,
			   ":%s %d %s :Birth Date: %s, compile # %s",
			   ME, RPL_INFO, parv[0], creation, generation);
		sendto_one(sptr, ":%s %d %s :On-line since %s",
			   ME, RPL_INFO, parv[0],
			   myctime(me.firsttime));
		sendto_one(sptr, rpl_str(RPL_ENDOFINFO, parv[0]));
		return 5;
	    }
	else
		return 10;
}

/*
** m_links
**	parv[0] = sender prefix
**	parv[1] = servername mask
** or
**	parv[0] = sender prefix
**	parv[1] = server to query 
**      parv[2] = servername mask
*/
int	m_links(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	Reg	aServer	*asptr;
	char	*mask;
	aClient	*acptr;

#ifdef RUSNET_RLINES
	if (MyClient(sptr) && IsRMode(sptr)) {
		sendto_one(sptr, err_str(ERR_RESTRICTED, parv[0]));
		return 10;
	}
#endif

	if (parc > 2)
	    {
		if (IsServer(cptr) && check_link(cptr) && !IsOper(sptr))
		    {
			sendto_one(sptr, rpl_str(RPL_TRYAGAIN, parv[0]),
				   "LINKS");
			return 5;
		    }
		if (hunt_server(cptr, sptr, ":%s LINKS %s :%s", 1, parc, parv)
				!= HUNTED_ISME)
			return 5;
		mask = parv[2];
	    }
	else
		mask = parc < 2 ? NULL : parv[1];

	for (asptr = svrtop, (void)collapse(mask); asptr; asptr = asptr->nexts) 
	    {
		acptr = asptr->bcptr;
		if (!BadPtr(mask) && match(mask, acptr->name))
			continue;
		sendto_one(sptr, rpl_str(RPL_LINKS, parv[0]),
			   acptr->name, acptr->serv->up->name,
			   acptr->hopcount, (acptr->info[0] ? acptr->info :
			   "(Unknown Location)"));
	    }

	sendto_one(sptr, rpl_str(RPL_ENDOFLINKS, parv[0]),
		   BadPtr(mask) ? "*" : mask);
	return 2;
}

/*
** m_summon should be redefined to ":prefix SUMMON host user" so
** that "hunt_server"-function could be used for this too!!! --msa
** As of 2.7.1e, this was the case. -avalon
**
**	parv[0] = sender prefix
**	parv[1] = user
**	parv[2] = server
**	parv[3] = channel (optional)
*/
int	m_summon(cptr, sptr, parc, parv)
aClient *sptr, *cptr;
int	parc;
char	*parv[];
{
	char	*host, *user, *chname;
#ifdef	ENABLE_SUMMON
	char	hostbuf[17], namebuf[10], linebuf[10];
#  ifdef LEAST_IDLE
	char	linetmp[10], ttyname[15]; /* Ack */
	struct	stat stb;
	time_t	ltime = (time_t)0;
#  endif
	int	fd, flag = 0;
#endif

	if (parc < 2 || *parv[1] == '\0')
	    {
		sendto_one(sptr, err_str(ERR_NORECIPIENT, parv[0]), "SUMMON");
		return 1;
	    }
	user = parv[1];
	host = (parc < 3 || BadPtr(parv[2])) ? ME : parv[2];
	chname = (parc > 3) ? parv[3] : "*";
	/*
	** Summoning someone on remote server, find out which link to
	** use and pass the message there...
	*/
	parv[1] = user;
	parv[2] = host;
	parv[3] = chname;
	parv[4] = NULL;
	if (hunt_server(cptr, sptr, ":%s SUMMON %s %s %s", 2, parc, parv) ==
	    HUNTED_ISME)
	    {
#ifdef ENABLE_SUMMON
		if ((fd = utmp_open()) == -1)
		    {
			sendto_one(sptr, err_str(ERR_FILEERROR, parv[0]),
				   "open", UTMP);
			return 1;
		    }
#  ifndef LEAST_IDLE
		while ((flag = utmp_read(fd, namebuf, linebuf, hostbuf,
					 sizeof(hostbuf))) == 0) 
			if (StrEq(namebuf,user))
				break;
#  else
		/* use least-idle tty, not the first
		 * one we find in utmp. 10/9/90 Spike@world.std.com
		 * (loosely based on Jim Frost jimf@saber.com code)
		 */
		
		while ((flag = utmp_read(fd, namebuf, linetmp, hostbuf,
					 sizeof(hostbuf))) == 0)
		    {
			if (StrEq(namebuf,user))
			    {
				sprintf(ttyname,"/dev/%s",linetmp);
				if (stat(ttyname,&stb) == -1)
				    {
					sendto_one(sptr,
						   err_str(ERR_FILEERROR,
						   sptr->name),
						   "stat", ttyname);
					return 1;
				    }
				if (!ltime)
				    {
					ltime= stb.st_mtime;
					(void)strcpy(linebuf,linetmp);
				    }
				else if (stb.st_mtime > ltime) /* less idle */
				    {
					ltime= stb.st_mtime;
					(void)strcpy(linebuf,linetmp);
				    }
			    }
		    }
#  endif
		(void)utmp_close(fd);
#  ifdef LEAST_IDLE
		if (ltime == 0)
#  else
		if (flag == -1)
#  endif
			sendto_one(sptr, err_str(ERR_NOLOGIN, parv[0]), user);
		else
			summon(sptr, user, linebuf, chname);
#else
		sendto_one(sptr, err_str(ERR_SUMMONDISABLED, parv[0]));
#endif /* ENABLE_SUMMON */
	    }
	else
		return 3;
	return 2;
}


/*
** m_stats
**	parv[0] = sender prefix
**	parv[1] = statistics selector (defaults to Message frequency)
**	parv[2] = server name (current server defaulted, if omitted)
**
**	Currently supported are:
**		M = Message frequency (the old stat behaviour)
**		L = Local Link statistics
**		C = Report C and N configuration lines
*/
/*
** m_stats/stats_conf
**    Report N/C-configuration lines from this server. This could
**    report other configuration lines too, but converting the
**    status back to "char" is a bit akward--not worth the code
**    it needs...
**
**    Note:   The info is reported in the order the server uses
**	      it--not reversed as in ircd.conf!
*/

#ifdef RUSNET_RLINES
#define	REP_ARRAY_SIZE	19
#else
#define	REP_ARRAY_SIZE	18
#endif

static u_int report_array[REP_ARRAY_SIZE][3] = {
		{ CONF_ZCONNECT_SERVER,	  RPL_STATSCLINE, 'c'},
		{ CONF_CONNECT_SERVER,	  RPL_STATSCLINE, 'C'},
		{ CONF_NOCONNECT_SERVER,  RPL_STATSNLINE, 'N'},
		{ CONF_CLIENT,		  RPL_STATSILINE, 'I'},
		{ CONF_RCLIENT,		  RPL_STATSILINE, 'i'},
		{ CONF_KILL,		  RPL_STATSKLINE, 'K'},
		{ CONF_QUARANTINED_SERVER,RPL_STATSQLINE, 'Q'},
		{ CONF_LOG,		  RPL_STATSLLINE, 'L'},
		{ CONF_OPERATOR,	  RPL_STATSOLINE, 'O'},
		{ CONF_HUB,		  RPL_STATSHLINE, 'H'},
		{ CONF_LOCOP,		  RPL_STATSOLINE, 'o'},
		{ CONF_SERVICE,		  RPL_STATSSLINE, 'S'},
		{ CONF_VER,		  RPL_STATSVLINE, 'V'},
		{ CONF_BOUNCE,		  RPL_STATSBLINE, 'B'},
		{ CONF_DENY,		  RPL_STATSDLINE, 'D'},
		{ CONF_EXEMPT,		  RPL_STATSKLINE, 'E'},
		{ CONF_INTERFACE,	  RPL_STATSFLINE, 'F'},
# ifdef RUSNET_RLINES
		{ CONF_RUSNETRLINE,	  RPL_STATSRLINE, 'R'},
# endif
		{ 0, 0, 0}
	};

static	void	report_configured_links(sptr, to, mask)
aClient *sptr;
char	*to;
int	mask;
{
	static	char	null[] = "<NULL>";
	unsigned int	port;
	aConfItem *tmp;
	u_int	*p;
	char	c, *host, *pass, *name;
	time_t	hold;
	aClient *acptr = find_client(to, NULL);

	for (tmp = (mask & CONF_KILL) ? kconf : (
#ifdef RUSNET_RLINES
	    (mask & (CONF_RUSNETRLINE)) ? rconf :
#endif	
	 (mask & (CONF_EXEMPT)) ? econf : conf);
	     tmp; tmp = tmp->next)
		if (tmp->status & mask)
		    {
			for (p = &report_array[0][0]; *p; p += 3)
				if (*p == tmp->status)
					break;
			if (!*p)
				continue;
			c = (char)*(p + 2);
			host = BadPtr(tmp->host) ? null : tmp->host;
			pass = BadPtr(tmp->passwd) ? NULL : tmp->passwd;
			name = BadPtr(tmp->name) ? null : tmp->name;
			port = (int)tmp->port;
			hold = (time_t)tmp->hold;
			/*
			 * On K/V/B/F lines the passwd contents can be
			 * displayed on STATS reply. 	-Vesa
			 */
			if (tmp->status & CONF_TLINE)
				sendto_one(sptr, rpl_str(p[1], to), c, 
					    (name) ? name : "*",
					    (host) ? host : "*",
					    (pass) ? pass : "-",
					    port, (hold > 0) ? myctime(hold) : "permanent");
			    
			else if ( tmp->status == CONF_VER
					    || tmp->status == CONF_INTERFACE
					    || tmp->status == CONF_BOUNCE)
				sendto_one(sptr, rpl_str(p[1], to), c, host,
					   (pass) ? pass : null,
					   name, port, get_conf_class(tmp));

			else if (tmp->status == CONF_CONNECT_SERVER ||
				tmp->status == CONF_ZCONNECT_SERVER ||
				tmp->status == CONF_NOCONNECT_SERVER ||
				tmp->status == CONF_CLIENT ||
				tmp->status == CONF_RCLIENT)
				sendto_one(sptr, rpl_str(p[1], to), c, host,
					   (pass || !(IsOper(sptr) ||
					    IsServer(sptr))) ? "x" : null,
					   name, port, get_conf_class(tmp),
							tmp->localpref);

			else if (tmp->status == CONF_OPERATOR ||
				tmp->status == CONF_LOCOP)
				sendto_one(sptr, rpl_str(p[1], to), c, 
					   (acptr && IsAnOper(acptr)) ?
					   host : "-", (pass) ? "x" : null,
					   name, port, get_conf_class(tmp));

			else if (tmp->status == CONF_LOG)
				sendto_one(sptr, rpl_str(p[1], to), c,
								host, pass);

			else
				sendto_one(sptr, rpl_str(p[1], to), c, host,
					    (pass) ? "x" : null,
					   name, port, get_conf_class(tmp));
		    }
	return;
}

static	void	report_ping(sptr, to)
aClient	*sptr;
char	*to;
{
	aConfItem *tmp;
	aCPing	*cp;

	for (tmp = conf; tmp; tmp = tmp->next)
		if ((cp = tmp->ping) && cp->lseq)
		    {
			if (strcasecmp(tmp->name, tmp->host))
				sprintf(buf,"%s[%s]",tmp->name, tmp->host);
			else
				(void)strcpy(buf, tmp->name);
			sendto_one(sptr, rpl_str(RPL_STATSPING, to),
				   buf, cp->lseq, cp->lrecvd,
				   cp->ping / (cp->recvd ? cp->recvd : 1),
				   tmp->pref);
			sendto_flag(SCH_DEBUG, "%s: %d", buf, cp->seq);
		    }
	return;
}

int	m_stats(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
    {
	static	char	Lformat[]  = ":%s %d %s %s %u %u %u %u %u :%u";
	struct	Message	*mptr;
	aClient	*acptr;
	char	stat = parc > 1 ? parv[1][0] : '\0';
	Reg	int	i;
	int	wilds, doall;
	char	*name, *cm;

#ifdef RUSNET_RLINES
	if (MyClient(sptr) && IsRMode(sptr)) {
		sendto_one(sptr, err_str(ERR_RESTRICTED, parv[0]));
		return 5;
	}
#endif

#ifdef STATS_RESTRICTED
	if (!IsAnOper(sptr) && !(stat == 'c' || stat == 'C' ||
				stat == 'H' || stat == 'h' ||
				stat == 'o' || stat == 'O' ||
				stat == 'P' || stat == 'U' || stat == 'u'))
	{
		sendto_one(sptr, err_str(ERR_NOPRIVILEGES, parv[0]));
		return 2;
	}
#endif
	if (IsServer(cptr) &&
	    (stat != 'd' && stat != 'p' && stat != 'q' && stat != 's' &&
	     stat != 'u' && stat != 'v') &&
	    !((stat == 'o' || stat == 'c') && IsOper(sptr)))
	    {
		if (check_link(cptr))
		    {
			sendto_one(sptr, rpl_str(RPL_TRYAGAIN, parv[0]),
				   "STATS");
			return 5;
		    }
	    }
	if (parc == 3)
	    {
		if (hunt_server(cptr, sptr, ":%s STATS %s %s",
				2, parc, parv) != HUNTED_ISME)
			return 5;
	    }
	else if (parc >= 3)
	    {
		if (hunt_server(cptr, sptr, ":%s STATS %s %s %s",
				2, parc, parv) != HUNTED_ISME)
			return 5;
	    }

	name = (parc > 2) ? parv[2] : ME;
	cm = (parc > 3) ? parv[3]: name;
	doall = !match(name, ME) && !match(cm, ME);
	wilds = index(cm, '*') || index(cm, '?');

	switch (stat)
	{
	case 'N' : case 'n' :
		/*
		 * send info about connections which match, or all if the
		 * mask matches ME.  Only restrictions are on those who
		 * are invisible not being visible to 'foreigners' who use
		 * a wild card based search to list it.
		 */
		if (doall || wilds)
		    {
			for (i = 0; i <= highest_fd; i++)
			    {
				if (!(acptr = local[i]))
					continue;
				if (IsPerson(acptr) && !(MyConnect(sptr) 
				    && IsAnOper(sptr)) && acptr != sptr)
					continue;
				if (wilds && match(cm, acptr->name))
					continue;
				sendto_one(cptr, Lformat, ME,
					RPL_STATSLINKINFO, parv[0],
					get_client_name(acptr, isupper(stat)),
					(int)DBufLength(&acptr->sendQ),
					(int)acptr->sendM, (int)acptr->sendK,
					(int)acptr->receiveM, 
					(int)acptr->receiveK,
					timeofday - acptr->firsttime);
			    }
		    }
		else
		    {
			if ((acptr = find_client(cm, NULL)))
				sendto_one(cptr, Lformat, ME,
					RPL_STATSLINKINFO, parv[0],
					get_client_name(acptr, isupper(stat)),
					(int)DBufLength(&acptr->sendQ),
					(int)acptr->sendM, (int)acptr->sendK,
					(int)acptr->receiveM,
					(int)acptr->receiveK,
					timeofday - acptr->firsttime);
			
		    }
		break;
#if defined(USE_IAUTH)
	case 'a' : case 'A' : /* iauth configuration */
		report_iauth_conf(sptr, parv[0]);
		break;
#endif
	case 'B' : case 'b' : /* B conf lines */
		report_configured_links(cptr, parv[0], CONF_BOUNCE);
		break;
	case 'c' : case 'C' : /* C and N conf lines */
		report_configured_links(cptr, parv[0], CONF_CONNECT_SERVER|
					CONF_ZCONNECT_SERVER|
					CONF_NOCONNECT_SERVER);
		break;
	case 'd' : case 'D' : /* defines */
		send_defines(cptr, parv[0]);
		break;
#ifndef USE_OLD8BIT
	case 'e' : /* charsets conversion - no conf lines */
		conv_report(sptr, parv[0]);
		break;
#else
	case 'e' :
#endif
	case 'E' : /* E lines */
		report_configured_links(cptr, parv[0],
					(CONF_EXEMPT));
		break;

	case 'F' : case 'f' : /* F conf lines */
		report_configured_links(cptr, parv[0], CONF_INTERFACE);
		break;

	case 'H' : case 'h' : /* H, L and D conf lines */
		report_configured_links(cptr, parv[0],
					CONF_HUB|CONF_LEAF|CONF_DENY);
		break;
	case 'I' : case 'i' : /* I (and i) conf lines */
		report_configured_links(cptr, parv[0],
					CONF_CLIENT|CONF_RCLIENT);
		break;
	case 'K' : case 'k' : /* K lines */
		report_configured_links(cptr, parv[0], CONF_KILL);
		break;
	case 'L' : case 'l' : /* L lines */
		report_configured_links(cptr, parv[0], CONF_LOG);
		break;
	case 'M' : case 'm' : /* commands use/stats */
		for (mptr = msgtab; mptr->cmd; mptr++)
			if (mptr->count)
				sendto_one(cptr, rpl_str(RPL_STATSCOMMANDS,
					   parv[0]), mptr->cmd,
					   mptr->count, mptr->bytes,
					   mptr->rcount);
		break;
	case 'o' : case 'O' : /* O (and o) lines */
		report_configured_links(cptr, parv[0], CONF_OPS);
		break;
        case 'P' : /* ports listening */
	        report_listeners(sptr, parv[0]);
		break;			
	case 'p' : /* ircd ping stats */
		report_ping(sptr, parv[0]);
		break;
	case 'Q' : case 'q' : /* Q lines */
		report_configured_links(cptr,parv[0],CONF_QUARANTINED_SERVER);
		break;
#ifdef RUSNET_RLINES
	case 'R' : case 'r' : /* Rusnet R lines */
		report_configured_links(cptr, parv[0],
					CONF_RUSNETRLINE);
		break;
#endif
	case 'S' : case 's' : /* S lines */
		report_configured_links(cptr, parv[0], CONF_SERVICE);
		break;
	case 'T' : case 't' : /* various statistics */
		send_usage(cptr, parv[0]);
		tstats(cptr, parv[0]);
		break;
	case 'U' : case 'u' : /* uptime */
	    {
		register time_t now;

		now = timeofday - me.since;
		sendto_one(sptr, rpl_str(RPL_STATSUPTIME, parv[0]),
			   now/86400, (now/3600)%24, (now/60)%60, now%60);
		break;
	    }
	case 'V' : case 'v' : /* V conf lines */
		report_configured_links(cptr, parv[0], CONF_VER);
		break;
	case 'X' : case 'x' : /* lists */
#ifdef	DEBUGMODE
		send_listinfo(cptr, parv[0]);
#endif
		break;
	case 'Y' : case 'y' : /* Y lines */
		report_classes(cptr, parv[0]);
		break;
	case 'Z' : 	      /* memory use (OPER only) */
		if (MyOper(sptr))
			count_memory(cptr, parv[0], 1);
		else
			sendto_one(sptr, err_str(ERR_NOPRIVILEGES, parv[0]));
		break;
	case 'z' :	      /* memory use */
		count_memory(cptr, parv[0], 0);
		break;
	default :
		stat = '*';
		break;
	}
	sendto_one(cptr, rpl_str(RPL_ENDOFSTATS, parv[0]), stat);
	return 2;
    }

/*
** m_users
**	parv[0] = sender prefix
**	parv[1] = servername
*/
int	m_users(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
#ifdef ENABLE_USERS
	char	namebuf[10],linebuf[10],hostbuf[17];
	int	fd, flag = 0;
#endif

	if (hunt_server(cptr,sptr,":%s USERS :%s",1,parc,parv) == HUNTED_ISME)
	    {
#ifdef ENABLE_USERS
		if ((fd = utmp_open()) == -1)
		    {
			sendto_one(sptr, err_str(ERR_FILEERROR, parv[0]),
				   "open", UTMP);
			return 1;
		    }

		sendto_one(sptr, rpl_str(RPL_USERSSTART, parv[0]));
		while (utmp_read(fd, namebuf, linebuf,
				 hostbuf, sizeof(hostbuf)) == 0)
		    {
			flag = 1;
			sendto_one(sptr, rpl_str(RPL_USERS, parv[0]),
				   namebuf, linebuf, hostbuf);
		    }
		if (flag == 0) 
			sendto_one(sptr, rpl_str(RPL_NOUSERS, parv[0]));

		sendto_one(sptr, rpl_str(RPL_ENDOFUSERS, parv[0]));
		(void)utmp_close(fd);
#else
		sendto_one(sptr, err_str(ERR_USERSDISABLED, parv[0]));
#endif
	    }
	else
		return 3;
	return 2;
}

/*
** Note: At least at protocol level ERROR has only one parameter,
** although this is called internally from other functions
** --msa
**
**	parv[0] = sender prefix
**	parv[*] = parameters
*/
int	m_error(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
    {
	Reg	char	*para;

	para = (parc > 1 && *parv[1] != '\0') ? parv[1] : "<>";

	Debug((DEBUG_ERROR,"Received ERROR message from %s: %s",
	      sptr->name, para));
	/*
	** Ignore error messages generated by normal user clients
	** (because ill-behaving user clients would flood opers
	** screen otherwise). Pass ERROR's from other sources to
	** the local operator...
	*/
	if (IsPerson(cptr) || IsUnknown(cptr) || IsService(cptr))
		return 2;
	if (cptr == sptr)
		sendto_flag(SCH_ERROR, "from %s -- %s",
			   get_client_name(cptr, FALSE), para);
	else
		sendto_flag(SCH_ERROR, "from %s via %s -- %s",
			   sptr->name, get_client_name(cptr,FALSE), para);
	return 2;
    }

/*
** m_help
**	parv[0] = sender prefix
*/
int	m_help(aClient *cptr _UNUSED_, aClient *sptr,
			int parc _UNUSED_, char *parv[])
    {
	int i;

#ifdef RUSNET_RLINES
	if (MyClient(sptr) && IsRMode(sptr)) {
		sendto_one(sptr, err_str(ERR_RESTRICTED, parv[0]));
		return 5;
	}
#endif
	for (i = 0; msgtab[i].cmd; i++)
	  sendto_one(sptr,":%s NOTICE %s :%s",
		     ME, parv[0], msgtab[i].cmd);
	return 2;
    }

/*
 * parv[0] = sender
 * parv[1] = host/server mask.
 * parv[2] = server to query
 */
int	 m_lusers(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
    {
	int	s_count = 0,	/* server */
		c_count = 0,	/* client (visible) */
		u_count = 0,	/* unknown */
		i_count = 0,	/* invisible client */
		o_count = 0,	/* oparator */
		v_count = 0;	/* service */
	int	m_client = 0,	/* my clients */
		m_server = 0,	/* my server links */
		m_service = 0;	/* my services */
	aClient *acptr;

#ifdef RUSNET_RLINES
	if (MyClient(sptr) && IsRMode(sptr)) {
		sendto_one(sptr, err_str(ERR_RESTRICTED, parv[0]));
		return 5;
	}
#endif

	if (parc > 2)
		if (hunt_server(cptr, sptr, ":%s LUSERS %s :%s", 2, parc, parv)
		    != HUNTED_ISME)
			return 3;

	if (parc == 1 || !MyConnect(sptr))
	    {
		sendto_one(sptr, rpl_str(RPL_LUSERCLIENT, parv[0]),
			   istat.is_user[0] + istat.is_user[1],
			   istat.is_service, istat.is_serv);
		if (istat.is_oper)
			sendto_one(sptr, rpl_str(RPL_LUSEROP, parv[0]),
				   istat.is_oper);
		if (istat.is_unknown > 0)
			sendto_one(sptr, rpl_str(RPL_LUSERUNKNOWN, parv[0]),
				   istat.is_unknown);
		if (istat.is_chan)
			sendto_one(sptr, rpl_str(RPL_LUSERCHANNELS, parv[0]),
				   istat.is_chan);
		sendto_one(sptr, rpl_str(RPL_LUSERME, parv[0]),
			   istat.is_myclnt, istat.is_myservice,
			   istat.is_myserv);
#ifdef EXTRA_STATISTICS
		sendto_one(sptr, rpl_str(RPL_LOCALUSERS, parv[0]),
				istat.is_myclnt, istat.is_m_myclnt);
		sendto_one(sptr, rpl_str(RPL_GLOBALUSERS, parv[0]),
				istat.is_user[1] + istat.is_user[0],
							istat.is_m_users);
#endif
		return 2;
	    }
	(void)collapse(parv[1]);
	for (acptr = client; acptr; acptr = acptr->next)
	    {
		if (!IsServer(acptr) && acptr->user)
		    {
			if (match(parv[1], acptr->user->server))
				continue;
		    }
		else
      			if (match(parv[1], acptr->name))
				continue;

		switch (acptr->status)
		{
		case STAT_SERVER:
			if (MyConnect(acptr))
				m_server++;
			/* flow thru */
		case STAT_ME:
			s_count++;
			break;
		case STAT_SERVICE:
			if (MyConnect(acptr))
				m_service++;
			v_count++;
			break;
		case STAT_CLIENT:
			if (IsOper(acptr))
				o_count++;
#ifdef	SHOW_INVISIBLE_LUSERS
			if (MyConnect(acptr))
		  		m_client++;
			if (!IsInvisible(acptr))
				c_count++;
			else
				i_count++;
#else
			if (MyConnect(acptr))
			    {
				if (IsInvisible(acptr))
				    {
					if (IsAnOper(sptr))
						m_client++;
				    }
				else
					m_client++;
			    }
	 		if (!IsInvisible(acptr))
				c_count++;
			else
				i_count++;
#endif
			break;
		default:
			u_count++;
			break;
	 	}
	     }
#ifndef	SHOW_INVISIBLE_LUSERS
	if (IsAnOper(sptr) && i_count)
#endif
	sendto_one(sptr, rpl_str(RPL_LUSERCLIENT, parv[0]),
		   c_count + i_count, v_count, s_count);
#ifndef	SHOW_INVISIBLE_LUSERS
	else
		sendto_one(sptr, rpl_str(RPL_LUSERCLIENT, parv[0]),
			   c_count, v_count, s_count);
#endif
	if (o_count)
		sendto_one(sptr, rpl_str(RPL_LUSEROP, parv[0]), o_count);
	if (u_count > 0)
		sendto_one(sptr, rpl_str(RPL_LUSERUNKNOWN, parv[0]), u_count);
	if ((c_count = count_channels(sptr))>0)
		sendto_one(sptr, rpl_str(RPL_LUSERCHANNELS, parv[0]),
			   count_channels(sptr));
	sendto_one(sptr, rpl_str(RPL_LUSERME, parv[0]), m_client, m_service,
		   m_server);
#if 0	/* it sends local values which is probably not that you wanted  -erra */
/* #ifdef EXTRA_STATISTICS */
	sendto_one(sptr, rpl_str(RPL_LOCALUSERS, parv[0]),
			istat.is_myclnt, istat.is_m_myclnt);
	sendto_one(sptr, rpl_str(RPL_GLOBALUSERS, parv[0]),
                         c_count + i_count, istat.is_m_users);
#endif
	return 2;
    }

  
/***********************************************************************
 * m_connect() - Added by Jto 11 Feb 1989
 ***********************************************************************/

/*
** m_connect
**	parv[0] = sender prefix
**	parv[1] = servername
**	parv[2] = port number
**	parv[3] = remote server
*/
int	m_connect(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
    {
	int	port, tmpport, retval;
	aConfItem *aconf;
	aClient *acptr;

	if (parc > 3 && IsLocOp(sptr))
	    {
		sendto_one(sptr, err_str(ERR_NOPRIVILEGES, parv[0]));
		return 1;
	    }

	if (hunt_server(cptr,sptr,":%s CONNECT %s %s :%s",
		       3,parc,parv) != HUNTED_ISME)
		return 1;

	if (parc < 3 || *parv[1] == '\0')
	    {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]),
			   "CONNECT");
		return 0;
	    }

	if ((acptr = find_name(parv[1], NULL))
	    || (acptr = find_mask(parv[1], NULL)))
	    {
		sendto_one(sptr, ":%s NOTICE %s :Connect: Server %s %s %s.",
			   ME, parv[0], parv[1], "already exists from",
			   acptr->from->name);
		return 0;
	    }

	for (aconf = conf; aconf; aconf = aconf->next)
		if ((aconf->status == CONF_CONNECT_SERVER ||
		     aconf->status == CONF_ZCONNECT_SERVER) &&
		    match(parv[1], aconf->name) == 0)
		  break;
	/* Checked first servernames, then try hostnames. */
	if (!aconf)
		for (aconf = conf; aconf; aconf = aconf->next)
			if ((aconf->status == CONF_CONNECT_SERVER ||
			     aconf->status == CONF_ZCONNECT_SERVER) &&
			    (match(parv[1], aconf->host) == 0 ||
			     match(parv[1], index(aconf->host, '@')+1) == 0))
		  		break;

	if (!aconf)
	    {
	      sendto_one(sptr,
			 "NOTICE %s :Connect: Host %s not listed in ircd.conf",
			 parv[0], parv[1]);
	      return 0;
	    }
	/*
	** Get port number from user, if given. If not specified,
	** use the default from configuration structure. If missing
	** from there, then use the precompiled default.
	*/
	tmpport = port = aconf->port;
	if ((port = atoi(parv[2])) <= 0)
	    {
		sendto_one(sptr, "NOTICE %s :Connect: Illegal port number",
			   parv[0]);
		return 0;
	    }
	else if (port <= 0)
	    {
		sendto_one(sptr, ":%s NOTICE %s :Connect: missing port number",
			   ME, parv[0]);
		return 0;
	    }
	/*
	** Notify all operators about remote connect requests
	*/
	if (!IsAnOper(cptr))
	    {
#ifdef WALLOPS_TO_CHANNEL
		sendto_serv_butone(NULL,
				  ":%s WALLOPS :Remote CONNECT %s %s from %s",
				   ME, parv[1], parv[2] ? parv[2] : "",
				   get_client_name(sptr,FALSE));
		sendto_flag(SCH_WALLOPS, "CONNECT From %s : %s %d", parv[0],
		       parv[1], parv[2] ? parv[2] : "");
#else
		sendto_ops_butone(NULL, &me,
				  ":%s WALLOPS :Remote CONNECT %s %s from %s",
				   ME, parv[1], parv[2] ? parv[2] : "",
				   get_client_name(sptr,FALSE));
		sendto_flag(SCH_DEBUG, "CONNECT From %s : %s %d", parv[0],
		       parv[1], parv[2] ? parv[2] : "");
#endif
	    }
	aconf->port = port;
	switch (retval = connect_server(aconf, sptr, NULL))
	{
	case 0:
		sendto_one(sptr, ":%s NOTICE %s :*** Connecting to %s[%s]:%d.",
			   ME, parv[0], aconf->host, aconf->name, aconf->port);
		sendto_flag(SCH_NOTICE, "Connecting to %s[%s]:%d by %s",
			    aconf->host, aconf->name, aconf->port,
			    get_client_name(sptr, FALSE));
		break;
	case -1:
		sendto_one(sptr, ":%s NOTICE %s :*** Couldn't connect to %s:%d",
			   ME, parv[0], aconf->host, aconf->port);
		sendto_flag(SCH_NOTICE, "Couldn't connect to %s:%d by %s",
			    aconf->host, aconf->port,
			    get_client_name(sptr, FALSE));
		break;
	case -2:
		sendto_one(sptr, ":%s NOTICE %s :*** Host %s is unknown.",
			   ME, parv[0], aconf->host);
		sendto_flag(SCH_NOTICE, "Connect by %s to unknown host %s",
			    get_client_name(sptr, FALSE), aconf->host);
		break;
	default:
		sendto_one(sptr,
			   ":%s NOTICE %s :*** Connection to %s failed: %s",
			   ME, parv[0], aconf->host, strerror(retval));
		sendto_flag(SCH_NOTICE, "Connection to %s by %s failed: %s",
			    aconf->host, get_client_name(sptr, FALSE),
			    strerror(retval));
	}
	aconf->port = tmpport;
	return 0;
    }

/*
** m_wallops (write to *all* opers currently online)
**	parv[0] = sender prefix
**	parv[1] = message text
*/
int	m_wallops(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
    {
	char	*message = parc > 1 ? parv[1] : NULL;

	if (BadPtr(message))
	    {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]),
			   "WALLOPS");
		return 1;
	    }
#ifdef WALLOPS_TO_CHANNEL
	sendto_serv_butone(IsServer(cptr) ? cptr : sptr,
			":%s WALLOPS :%s", parv[0], message);
	sendto_flag(SCH_WALLOPS, "[%s] %s", parv[0], message);
#else
	sendto_ops_butone(IsServer(cptr) ? cptr : NULL, sptr,
			":%s WALLOPS :%s", parv[0], message);
#endif
#ifdef	USE_SERVICES
	check_services_butone(SERVICE_WANT_WALLOP, NULL, sptr,
			      ":%s WALLOP :%s", parv[0], message);
#endif
	return 2;
    }

/*
** m_time
**	parv[0] = sender prefix
**	parv[1] = servername
*/
int	m_time(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	if (hunt_server(cptr,sptr,":%s TIME :%s",1,parc,parv) == HUNTED_ISME)
		sendto_one(sptr, rpl_str(RPL_TIME, parv[0]), ME, date((long)0));
#ifdef RUSNET_RLINES
	if (MyClient(sptr) && IsRMode(sptr)) {
		return 10;
	}
#endif
	return 2;
}


/*
** m_admin
**	parv[0] = sender prefix
**	parv[1] = servername
*/
int	m_admin(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
    {
	aConfItem *aconf;

	if (IsRegistered(cptr) &&	/* only local query for unregistered */
	    hunt_server(cptr,sptr,":%s ADMIN :%s",1,parc,parv) != HUNTED_ISME)
		return 3;
	if ((aconf = find_admin()) && aconf->host && aconf->passwd
	    && aconf->name)
	    {
		sendto_one(sptr, rpl_str(RPL_ADMINME, parv[0]), ME);
		sendto_one(sptr, rpl_str(RPL_ADMINLOC1, parv[0]), aconf->host);
		sendto_one(sptr, rpl_str(RPL_ADMINLOC2, parv[0]),
			   aconf->passwd);
		sendto_one(sptr, rpl_str(RPL_ADMINEMAIL, parv[0]),
			   aconf->name);
	    }
	else
		sendto_one(sptr, err_str(ERR_NOADMININFO, parv[0]), ME);
#ifdef RUSNET_RLINES
	if (MyClient(sptr) && IsRMode(sptr)) {
		return 10;
	}
#endif
	return 2;
    }

#if defined(OPER_REHASH) || defined(LOCOP_REHASH)
/*
** m_rehash
**	parv[0] = sender prefix
** 	parv[1] - subsystem to rehash, empty to rehash all.
*/
int	m_rehash(aClient *cptr _UNUSED_, aClient *sptr, int parc, char *parv[])
{
	int status = 0;
	
	if (parc == 1)
		status |= REHASH_ALL;
	else 
		switch (parv[1][0])
		{
			case 'B':
			case 'b':
				status |= REHASH_B;
				break;
			case 'C':
			case 'c':
				status |= REHASH_C | REHASH_Y;
				break;
			case 'D':
			case 'd':
				status |= REHASH_DNS;
				break;
			case 'E':
			case 'e':
				status |= REHASH_E;
				break;
			case 'H':
			case 'h':
				status |= REHASH_C | REHASH_Y;
				break;
			case 'I':
			case 'i':
				status |= REHASH_I | REHASH_Y;
				break;
			case 'K':
			case 'k':
				status |= REHASH_K;
				break;
			case 'L':
			case 'l':
				status |= REHASH_C | REHASH_Y;
				break;
			case 'M':
			case 'm':
				status |= REHASH_MOTD;
				break;
			case 'O':
			case 'o':
				status |= REHASH_O | REHASH_Y;
				break;
			case 'P':
			case 'p':
				status |= REHASH_P;
				break;
			case 'R':
			case 'r':
				status |= REHASH_R;
				break;
#ifdef USE_SSL
			case 'S':
			case 's':
				status |= REHASH_SSL;
				break;
#endif /* USE_SSL */
			case 'Y':
			case 'y':
				status |= REHASH_Y;
				break;
			default:
				sendto_one(sptr, err_str(ERR_NOREHASHPARAM,
							parv[0]), parv[1]);
				return 1;
		}

	sendto_one(sptr, rpl_str(RPL_REHASHING, parv[0]),
			mybasename(configfile), (parv[1]) ? parv[1] : "ALL");

	sendto_flag(SCH_NOTICE, "%s is rehashing Server config file [%s]",
					parv[0], (parv[1]) ? parv[1] : "ALL");
	
	return rehash(0, status);
}
#endif

#if defined(OPER_RESTART) || defined(LOCOP_RESTART)
/*
** m_restart
**
*/
int	m_restart(aClient *cptr _UNUSED_, aClient *sptr,
			int parc _UNUSED_, char *parv[] _UNUSED_)
{
	Reg	aClient	*acptr;
	Reg	int	i;
	char	killer[HOSTLEN * 2 + USERLEN + 5];

	strcpy(killer, get_client_xname(sptr, FALSE));
	sprintf(buf, "RESTART by %s", get_client_xname(sptr, FALSE));

	for (i = 0; i <= highest_fd; i++)
	    {
		if (!(acptr = local[i]))
			continue;
		if (IsClient(acptr) || IsService(acptr))
		    {
			sendto_one(acptr,
				   ":%s NOTICE %s :Server Restarting. %s",
				   ME, acptr->name, killer);
			acptr->exitc = EXITC_DIE;
			if (IsClient(acptr))
				exit_client(acptr, acptr, &me,
					    "Server Restarting");
			/* services are kept for logging purposes */
		    }
		else if (IsServer(acptr))
			sendto_one(acptr, ":%s ERROR :Restarted by %s",
				   ME, killer);
	    }
	flush_connections(me.fd);

	server_reboot(buf);
	/*NOT REACHED*/
	return 0;
}
#endif

/*
** m_trace
**	parv[0] = sender prefix
**	parv[1] = servername
*/
int	m_trace(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	Reg	int	i;
	Reg	aClient	*acptr;
	aClass	*cltmp;
	char	*tname;
	int	doall, link_s[MAXCONNECTIONS], link_u[MAXCONNECTIONS];
	int	wilds, dow;

#ifdef RUSNET_RLINES
	if (MyClient(sptr) && IsRMode(sptr)) {
		sendto_one(sptr, err_str(ERR_RESTRICTED, parv[0]));
		return 10;
	}
#endif
	if (parc > 1)
		tname = parv[1];
	else
		tname = ME;

	switch (hunt_server(cptr, sptr, ":%s TRACE :%s", 1, parc, parv))
	{
	case HUNTED_PASS: /* note: gets here only if parv[1] exists */
	    {
		aClient	*ac2ptr;

		ac2ptr = next_client(client, parv[1]);
		sendto_one(sptr, rpl_str(RPL_TRACELINK, parv[0]),
			   version, debugmode, tname, ac2ptr->from->name,
			   ac2ptr->from->serv->version,
			   (ac2ptr->from->flags & FLAGS_ZIP) ? "z" : "",
			   timeofday - ac2ptr->from->firsttime,
			   (int)DBufLength(&ac2ptr->from->sendQ),
			   (int)DBufLength(&sptr->from->sendQ));
		return 5;
	    }
	case HUNTED_ISME:
		break;
	default:
		return 1;
	}

	doall = (parv[1] && (parc > 1)) ? !match(tname, ME): TRUE;
	wilds = !parv[1] || index(tname, '*') || index(tname, '?');
	dow = wilds || doall;

	if (doall) {
		for (i = 0; i < MAXCONNECTIONS; i++)
			link_s[i] = 0, link_u[i] = 0;
		for (acptr = client; acptr; acptr = acptr->next)
#ifdef	SHOW_INVISIBLE_LUSERS
			if (IsPerson(acptr))
				link_u[acptr->from->fd]++;
#else
			if (IsPerson(acptr) &&
			    (!IsInvisible(acptr) || IsOper(sptr)))
				link_u[acptr->from->fd]++;
#endif
			else if (IsServer(acptr))
				link_s[acptr->from->fd]++;
	}

	/* report all direct connections */
	
	for (i = 0; i <= highest_fd; i++)
	    {
		char	*name;
		int	class;

		if (!(acptr = local[i])) /* Local Connection? */
			continue;
		if (IsPerson(acptr) && IsInvisible(acptr) && dow &&
		    !(MyConnect(sptr) && IsAnOper(sptr)) &&
		    !IsAnOper(acptr) && (acptr != sptr))
			continue;
		if (!doall && wilds && match(tname, acptr->name))
			continue;
		if (!dow && strcasecmp(tname, acptr->name))
			continue;

		name = get_client_xname(acptr, IsAnOper(sptr));
		class = get_client_class(acptr);

		switch(acptr->status)
		{
		case STAT_CONNECTING:
			sendto_one(sptr, rpl_str(RPL_TRACECONNECTING,
				   parv[0]), class, name);
			break;
		case STAT_HANDSHAKE:
			sendto_one(sptr, rpl_str(RPL_TRACEHANDSHAKE, parv[0]),
				   class, name);
			break;
		case STAT_ME:
			break;
		case STAT_UNKNOWN:
			if (IsAnOper(sptr))
				sendto_one(sptr,
					   rpl_str(RPL_TRACEUNKNOWN, parv[0]),
					   class, name);
			break;
		case STAT_CLIENT:
			/* Only opers see users if there is a wildcard
			 * but anyone can see all the opers.
			 */

			if (IsAnOper(acptr))
				sendto_one(sptr,
					   rpl_str(RPL_TRACEOPERATOR, parv[0]),
					   class, name);
			else if (!dow || (MyConnect(sptr) && IsAnOper(sptr)))
				sendto_one(sptr,
					   rpl_str(RPL_TRACEUSER, parv[0]),
					   class, name);
			break;
		case STAT_SERVER:
			if (acptr->serv->user)
				sendto_one(sptr, rpl_str(RPL_TRACESERVER,
					   parv[0]), class, link_s[i],
					   link_u[i], name, acptr->serv->by,
					   acptr->serv->user->username,
					   acptr->serv->user->host,
					   acptr->serv->version,
					   (acptr->flags & FLAGS_ZIP) ?"z":"");
			else
				sendto_one(sptr, rpl_str(RPL_TRACESERVER,
					   parv[0]), class, link_s[i],
					   link_u[i], name,
					   *(acptr->serv->by) ?
					   acptr->serv->by : "*", "*", ME,
					   acptr->serv->version,
					   (acptr->flags & FLAGS_ZIP) ?"z":"");
			break;
		case STAT_RECONNECT:
			sendto_one(sptr, rpl_str(RPL_TRACERECONNECT, parv[0]),
				   class, name);
			break;
		case STAT_SERVICE:
			sendto_one(sptr, rpl_str(RPL_TRACESERVICE, parv[0]),
				   class, name, acptr->service->type,
				   acptr->service->wants);
			break;
		default: /* ...we actually shouldn't come here... --msa */
			sendto_one(sptr, rpl_str(RPL_TRACENEWTYPE, parv[0]),
				   name);
			break;
		}
	    }

	/*
	 * Add these lines to summarize the above which can get rather long
	 * and messy when done remotely - Avalon
	 */
	if (IsOper(sptr))
	    for (cltmp = FirstClass(); doall && cltmp; cltmp = NextClass(cltmp))
		if (Links(cltmp) > 0)
			sendto_one(sptr, rpl_str(RPL_TRACECLASS, parv[0]),
				   Class(cltmp), Links(cltmp));
	sendto_one(sptr, rpl_str(RPL_TRACEEND, parv[0]), tname, version,
		   debugmode);
	return 2;
    }

/*
** m_motd
**	parv[0] = sender prefix
**	parv[1] = servername
*/
int	m_motd(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	register aMotd *temp;
	struct tm *tm;

#ifdef RUSNET_RLINES
	if (MyClient(sptr) && IsRMode(sptr)) {
		if (rmotd == NULL)
		{
			sendto_one(sptr, err_str(ERR_NOMOTD, parv[0]));
			return 5;
		}
		sendto_one(sptr, rpl_str(RPL_MOTDSTART, parv[0]), ME);
		temp = rmotd;
		for(temp=rmotd;temp != NULL;temp = temp->next)
			sendto_one(sptr, rpl_str(RPL_MOTD, parv[0]), temp->line);
		sendto_one(sptr, rpl_str(RPL_ENDOFMOTD, parv[0]));		
		return 10;
	}
#endif
	if (!IsUnknown(sptr))
	{
	if (check_link(cptr))
	    {
		sendto_one(sptr, rpl_str(RPL_TRYAGAIN, parv[0]), "MOTD");
		return 5;
	    }
	if (hunt_server(cptr, sptr, ":%s MOTD :%s", 1,parc,parv)!=HUNTED_ISME)
		return 5;
	}
	tm = localtime(&motd_mtime);
	if (motd == NULL)
	    {
		sendto_one(sptr, err_str(ERR_NOMOTD, parv[0]));
		return 1;
	    }
	sendto_one(sptr, rpl_str(RPL_MOTDSTART, parv[0]), ME);
	sendto_one(sptr, ":%s %d %s :- %d/%d/%d %d:%02d", ME, RPL_MOTD,
		   parv[0], tm->tm_mday, tm->tm_mon + 1, 1900 + tm->tm_year,
		   tm->tm_hour, tm->tm_min);
	temp = motd;
	for(temp=motd;temp != NULL;temp = temp->next) {
		sendto_one(sptr, rpl_str(RPL_MOTD, parv[0]), temp->line);
	}
	sendto_one(sptr, rpl_str(RPL_ENDOFMOTD, parv[0]));
	return IsUnknown(sptr) ? 5 : 2;
}

/*
** m_close - added by Darren Reed Jul 13 1992.
*/
int	m_close(aClient *cptr _UNUSED_, aClient *sptr,
			int parc _UNUSED_, char *parv[])
{
	Reg	aClient	*acptr;
	Reg	int	i;
	int	closed = 0;

	for (i = highest_fd; i; i--)
	    {
		if (!(acptr = local[i]))
			continue;
		if (!IsUnknown(acptr) && !IsConnecting(acptr) &&
		    !IsHandshake(acptr))
			continue;
		sendto_one(sptr, rpl_str(RPL_CLOSING, parv[0]),
			   get_client_name(acptr, TRUE), acptr->status);
		(void)exit_client(acptr, acptr, &me, "Oper Closing");
		closed++;
	    }
	sendto_one(sptr, rpl_str(RPL_CLOSEEND, parv[0]), closed);
	sendto_flag(SCH_NOTICE, "%s closed %d unknown connections", sptr->name,
		    closed);
#ifdef DELAY_CLOSE
	closed = istat.is_delayclosewait;
	delay_close(-2);
	sendto_flag(SCH_NOTICE, "%s closed %d delayed connections", sptr->name,
		    closed);
#endif
	return 1;
}

#ifdef HAVE_EOB
/* End Of Burst command
** parv[0] - server sending the EOB
** parv[1] - optional comma separated list of servers for which this EOB
**           is also valid.
*/
int	m_eob(aClient *cptr, aClient *sptr, int parc, char *parv[])
{
	return 0;
}

int	m_eoback(aClient *cptr, aClient *sptr, int parc, char *parv[])
{
	return 0;
}
#endif

#if defined(OPER_DIE) || defined(LOCOP_DIE)
int	m_die(aClient *cptr _UNUSED_, aClient *sptr,
		int parc _UNUSED_, char *parv[] _UNUSED_)
{
	Reg	aClient	*acptr;
	Reg	int	i;
	char	killer[HOSTLEN * 2 + USERLEN + 5];

	strcpy(killer, get_client_name(sptr, TRUE));
	for (i = 0; i <= highest_fd; i++)
	    {
		if (!(acptr = local[i]))
			continue;
		if (IsClient(acptr) || IsService(acptr))
		    {
			sendto_one(acptr,
				   ":%s NOTICE %s :Server Terminating. %s",
				   ME, acptr->name, killer);
			acptr->exitc = EXITC_DIE;
			if (IsClient(acptr))
				exit_client(acptr, acptr, &me, "Server died");
			/* services are kept for logging purposes */
		    }
		else if (IsServer(acptr))
			sendto_one(acptr, ":%s ERROR :Terminated by %s",
				   ME, killer);
	    }
	flush_connections(me.fd);
	(void)s_die(0);
	return 0;
}
#endif

/*
** storing server names in User structures is a real waste,
** the following functions change it to only store a pointer.
** A better way might be to store in Server structure and use servp. -krys
*/

static char	**server_name = NULL;
static int	server_max = 0, server_num = 0;

/*
** find_server_string
**
** Given an index, this will return a pointer to the corresponding
** (already allocated) string
*/
char *
find_server_string(snum)
int snum;
{
	if (snum < server_num && snum >= 0)
		return server_name[snum];
	/* request for a bogus snum value, something is wrong */
	sendto_flag(SCH_ERROR, "invalid index for server_name[] : %d (%d,%d)",
		    snum, server_num, server_max);
	return NULL;
}

/*
** find_server_num
**
** Given a server name, this will return the index of the corresponding
** string. This index can be used with find_server_name_from_num().
** If the string doesn't exist already, it will be allocated.
*/
int
find_server_num(sname)
char *sname;
{
	Reg int i = 0;

	while (i < server_num)
	    {
		if (!strcasecmp(server_name[i], sname))
			break;
		i++;
	    }
	if (i < server_num)
		return i;
	if (i == server_max)
	  {
	    /* server_name[] array is full, let's make it bigger! */
	    if (server_name)
		    server_name = (char **) MyRealloc((char *)server_name,
					      sizeof(char *)*(server_max+=50));
	    else
		    server_name = (char **) MyMalloc(sizeof(char *)*(server_max=50));
	  }
	server_name[server_num] = mystrdup(sname);
	return server_num++;
}

/*
** check_link (added 97/12 to prevent abuse)
**	routine which tries to find out how healthy a link is.
**	useful to know if more strain may be imposed on the link or not.
**
**	returns 0 if link load is light, -1 otherwise.
*/
static int
check_link(cptr)
aClient	*cptr;
{
    if (!IsServer(cptr))
	    return 0;
    if (!(bootopt & BOOT_PROT))
	    return 0;

    ircstp->is_ckl++;
    if ((int)DBufLength(&cptr->sendQ) > 65536) /* SendQ is already (too) high*/
	{
	    cptr->serv->lastload = timeofday;
	    ircstp->is_cklQ++;
	    return -1;
	}
    if (timeofday - cptr->firsttime < 60) /* link is too young */
	{
	    ircstp->is_ckly++;
	    return -1;
	}
    if (timeofday - cptr->serv->lastload > 30)
	    /* last request more than 30 seconds ago => OK */
	{
	    cptr->serv->lastload = timeofday;
	    ircstp->is_cklok++;
	    return 0;
	}
    if (timeofday - cptr->serv->lastload > 15
	&& (int)DBufLength(&cptr->sendQ) < CHREPLLEN)
	    /* last request between 15 and 30 seconds ago, but little SendQ */
	{
	    cptr->serv->lastload = timeofday;
	    ircstp->is_cklq++;
	    return 0;
	}
    ircstp->is_cklno++;
    return -1;
}

static void report_listeners(aClient *sptr, char *to)
{
	aConfItem *tmp;
	aClient	*acptr;
	char *what;

	if (IsAnOper(sptr))
		sendto_one(sptr, ":%s %d %s Port Host DBuf sendM sendB "
				"rcvM rcvB Uptime State"
#ifndef USE_OLD8BIT
				" Charset"
#endif
				, ME, RPL_STATSLINKINFO, to);

	for (acptr = ListenerLL; acptr; acptr = acptr->next)
	{
		tmp = acptr->confs->value.aconf;
		if (IsIllegal(tmp)) {
#ifdef USE_SSL
		    if (IsSSLListener(acptr))
			    what = "ssl dead";
			else
#endif
			    what = "nonssl dead";
#ifdef USE_SSL
		} else if (IsSSLListener(acptr)) {
			what = "ssl active";
#endif
		} else {
			what = "nonssl active";
		}

		if (IsAnOper(sptr))
		    sendto_one(sptr, ":%s %d %s %d %s %d %d %u %d %u %u %d %s"
#ifndef USE_OLD8BIT
				" %s"
#endif
			, ME, RPL_STATSLINKINFO, to,
			tmp->port, (*tmp->host == '\0') ? tmp->host : "-",
			(uint)DBufLength(&acptr->sendQ),
			acptr->sendM, acptr->sendB,
			acptr->receiveM, acptr->receiveB,
			timeofday - acptr->firsttime,
			tmp->clients, what
#ifndef USE_OLD8BIT
				, acptr->conv ? conv_charset(acptr->conv) : ""
#endif
		    );
		else
		    sendto_one(sptr, ":%s %d %s %d %s %s"
#ifndef USE_OLD8BIT
							" %s"
#endif
			, ME, RPL_STATSLINKINFO, to,
			tmp->port, (*tmp->host == '\0') ? tmp->host : "-",
			what
#ifndef USE_OLD8BIT
				, acptr->conv ? conv_charset(acptr->conv) : ""
#endif
		    );
	}
}


/* Dynamic lines handling code */


#define NONWILDCHARS 4

int sendto_slaves(cptr, parv, command)
aClient	*cptr;
char	*parv[];
char	*command;
{
	aClient	*acptr = NULL;
/* sendto_slaves */
	if (parv[1][0] == '*') { /* masks need to be broadcasted */
	    sendto_serv_butone(cptr, ":%s %s %s %s %s %s %s :%s",
					parv[0],
					command,
					parv[1],
					parv[2],
					parv[3],
					parv[4],
					parv[5] ? parv[5] : "",
					parv[6] ? parv[6] : "No reason");
	    if (parv[1][1]) /* mask is not just '*' but server mask */
		if (match(parv[1], ME))
		    return -1; /* Mask doesn't match me */		
	} else {
	    /* Check that target server exists and is not sending us this */
	    if ((acptr = find_server(parv[1], NULL)) == NULL)
	       return -1;		/* No such server known */
	    if (!IsMe(acptr))
	    {
		if (acptr->from != cptr) /* Send it through */
		    sendto_one(acptr, ":%s %s %s %s %s %s %s :%s",
					parv[0],
					command,
					parv[1],
					parv[2],
					parv[3],
					parv[4],
					parv[5] ? parv[5] : "",
					parv[6] ? parv[6] : "No reason");
		return -1;
	    }
	}
	return 0;
/* end of sendto_slaves */
}

int nonwilds(char *name, char *user, char *host)
/* Originally from hybrid ircd */
{
    register int  nonwild = 0;
    register char tmpch;
    register int  wild = 0 ;
    char	  *p;

    p = name;
    while ((tmpch = *p++))
    {
	if (!IsLWildChar(tmpch))
	{
	/*
	 * If we find enough non-wild characters, we can
	 * break - no point in searching further.
	 */
	    if (++nonwild >= NONWILDCHARS)
		break;
	} else {
	    wild++;
	}
    }
    
    Debug((DEBUG_INFO, "%d nonwild chars found in name %s",
		    nonwild, name));
    if (!wild)
	return NONWILDCHARS;
    wild = 0;
    if (nonwild < NONWILDCHARS)
    {
    /*
     * The nammae portion did not contain enough non-wild
     * characters, try the user.
     */
	p = user;
	while ((tmpch = *p++))
	{
	    if (!IsLWildChar(tmpch)) {
    		if (++nonwild >= NONWILDCHARS)
    		    break;
	    } else {
		wild++;
	    }
	}
    }
    Debug((DEBUG_INFO, "%d non wild chars found in user %s",
		    nonwild, user));
    if (!wild)
	return NONWILDCHARS;
    wild = 0 ;
    if (nonwild < NONWILDCHARS)
    {
    /*
     * The user portion did not contain enough non-wild
     * characters, try the host.
     */
	p = host;
	while ((tmpch = *p++))
	{
	    if (!IsLWildChar(tmpch)) {
    		if (++nonwild >= NONWILDCHARS)
    		    break;
	    } else {
		wild++;
	    }
	}
    }
    Debug((DEBUG_INFO, "%d non wild chars found in host %s",
		    nonwild, host));
    if (!wild)
	return NONWILDCHARS;
    else
	return nonwild;
}

 /*
 ** line1 is less than line2 if usermask of line1 is mached by usermask of line2
 ** and expire time of line1 is less than expire time of line2.
 ** They are equal if masks are equal.
 ** Returns -1 if line1 < line2,
 ** 0 if line1 == line2,
 ** 1 if line1 > line2
 ** -2 if line1 is incomparable with line2
 */

 /* line1 stands for new line, line2 is for existing one */
int match_tline(aConfItem *line1, aConfItem *line2)
{
    int line12_uh;
    int line21_uh;
    char *p;
/* TODO: port matching here and everywhere in this code (now ignoring) -skold */
    if (line1->status != line2->status)
	return -2;
    if (((p = index(line1->host, '@')) && (*(++p) == '=')) ||
	((p = index(line2->host, '@')) && (*(++p) == '='))) 
    {
	int line12_h = strcmp(line1->host, line2->host);
	line12_uh = match(line1->name, line2->name);
	line21_uh = match(line2->name, line1->name);
	if (line12_h)
	    return -2;
	else if (!line12_uh && !line21_uh)
	    return 0;
	else if (!line12_uh)
	    return 1;
	else if (!line21_uh)
	    return -1;
	else
	    return -2;
    }
    line12_uh = match(line1->host, line2->host) + 
	    match(line1->name, line2->name);
    line21_uh = match(line2->host, line1->host) + 
	    match(line2->name, line1->name);

    Debug((DEBUG_INFO, "Lines match: %s!%s => %s!%s: %d/%d",
	    line1->name, line1->host,
	    line2->name, line2->host, line12_uh, line21_uh));

    if ((line12_uh + line21_uh) == 0) /* lines are equal */
	return 0;
    if ((line12_uh >= 1) && (line21_uh >= 1)) /* incomp. */
	return -2;
    if (line12_uh == 0 && (line1->hold <= 0 ||
	    (line1->hold >= line2->hold))) /* line1 >= line2 */
    {
	return 1;
    }
    if (line21_uh == 0 && (line2->hold == 0 ||
	    (line2->hold >= line1->hold))) /* line1 <= line2 */
    {
	return -1;
    }
    return -2; /* reached only if line matched but expiration is not */
}

int send_tline_one(aConfItem *aconf) {
    char type;
    switch (aconf->status) {
	case CONF_KILL:
	    type = 'K';
	    break;
	case CONF_EXEMPT:
	    type = 'E';
	    break;
	case CONF_RUSNETRLINE:
	    type = 'R';
	    break;
	default:
	    return 0;
    }
	sendto_iserv("%c%c%s%c%s%c%s%c%s%c%d",
	    type, IRCDCONF_DELIMITER,
	    aconf->host, IRCDCONF_DELIMITER,
	    aconf->passwd, IRCDCONF_DELIMITER,
	    aconf->name,  IRCDCONF_DELIMITER,
	    "", IRCDCONF_DELIMITER,
		    aconf->hold);
	sendto_flag(SCH_ISERV, "%c%c%s%c%s%c%s%c%s%c%s",
	    type, IRCDCONF_DELIMITER,
	    aconf->host, IRCDCONF_DELIMITER,
	    aconf->passwd, IRCDCONF_DELIMITER,
	    aconf->name,  IRCDCONF_DELIMITER,
	    "", IRCDCONF_DELIMITER,
				(aconf->hold > 0) ? myctime(aconf->hold) : 
				    ((aconf->hold == 0) ? "permanent" : "remove"));
	return 0;    
}

void rehash_tline(aConfItem **confstart, aConfItem *aconf)
{
    int		i = 0;
    Reg	aClient	*cptr;
    char	*reason;
    int klined = 0;
#ifdef RUSNET_RLINES
    int rlined = 0;
#endif
    
    for (i = highest_fd; i >= 0; i--)
    {
	if (!(cptr = local[i]) || IsListener(cptr))
	    continue;
	if (IsPerson(cptr))
	{
	    if (check_tlines(cptr, 1, &reason, cptr->name, confstart, aconf)) {
		if (aconf->status == CONF_KILL) {
		    char buf[300];
		    Debug((DEBUG_DEBUG, "Found K-line for user %s!%s@%s", cptr->name,
			                        cptr->user->username, cptr->sockhost));
		    sendto_flag(SCH_NOTICE, "Kill line active for %s",
				get_client_name(cptr, FALSE));
		    klined ++;
		    cptr->exitc = EXITC_KLINE;
		    if (!BadPtr(reason))
			sprintf(buf, "Kill line active: %.256s", reason);
		    (void)exit_client(cptr, cptr, &me, (reason) ?
				buf : "Kill line active");
		}
#ifdef RUSNET_RLINES
		if (aconf->status == CONF_RUSNETRLINE) {
		    if (!IsRMode(cptr))
		    {
			int old = (cptr->user->flags & ALL_UMODES);
			do_restrict(cptr);
			send_umode_out(cptr, cptr, old);			
			Debug((DEBUG_DEBUG, "Active R-Line for user %s!%s@%s", cptr->name,
			                cptr->user->username, cptr->sockhost));
			sendto_flag(SCH_NOTICE,"R line active for %s",
					get_client_name(cptr, FALSE));
			rlined++;
		    }
		}
#endif
	    }
	}
    }

    if (klined)
#ifdef WALLOPS_TO_CHANNEL
    {
	sendto_serv_butone(NULL, ":%s WALLOPS : %d user%s klined",
				ME, klined, (klined > 1) ? "s" : "");
	sendto_flag(SCH_WALLOPS, "%s: %d user%s klined",
				ME, klined, (klined > 1) ? "s" : "");
    }
#else
	sendto_ops_butone(NULL, &me, "%s: %d user%s klined",
				ME, klined, (klined > 1) ? "s" : "");
#endif
    if (rlined)
#ifdef WALLOPS_TO_CHANNEL
    {
	sendto_serv_butone(NULL, ":%s WALLOPS : %d user%s restricted",
				ME, rlined, (rlined > 1) ? "s" : "");
	sendto_flag(SCH_WALLOPS, "%s: %d user%s restricted",
				ME, rlined, (rlined > 1) ? "s" : "");
    }
#else
	sendto_ops_butone(NULL, &me, "%s: %d user%s restricted",
				ME, rlined, (rlined > 1) ? "s" : "");
#endif
}


/* Server syntax:
     * LINE server nickmask usermask hostmask [hours|E] [:reason]
     * Wildcards in masks are allowed. No hours means forever
     * "E" instead of hours means E-line (Exempt)
*/
/* Oper syntax:
     * LINE nick!user@host [hours] [reason]
     * For exempt mind ELINE command.
     * To remove Xline use UXKLINE.
*/ 
 /*
 ** If we add a line we should first check if we already have exact or matched (narrower) ones
 ** with timestamps less than the one we want to add.
 ** If we do, delete all lines that matching with the one we want to add and add this one instead.
 ** If the new line is matched by the one already added, do nothing.
 ** If we want to remove line, find all lines matched (more precise) by this one and remove all
 ** the lines with timestamps less than the one we want to remove.
 ** This is reasonable but a bit dangerous, so watch your fingers. -skold
 ** Since we are searching through all the lines anyway, we remove expired at the same time.
 */

int handle_tline(cptr, sptr, parc, parv, command)
aClient	*cptr, *sptr;
int	parc;
char	*parv[];
char	*command;
{
    char *user, *host, *name, *reason = NULL;
    time_t  tline_time = 0, now = time(NULL);
    char *str;
    
    if (IsServer(sptr)) {

	if (parc < 6)
	    return -1;		/* not enough parameters */    
	/* Max line size is 2 +  64 +  32 + 6 +   11 +   256  
	**                 type: host: nick:port: expire:reason
	** You are allowed to supply the reason of any length, 
	** but do we really need this fun? Let's truncate it 
	** downto 255 chars, that's enough to express yourself -skold
	*/
	if (parc < 7 || !parv[6] || !*parv[6])
	    reason = "No reason";
	else {
	    if (strlen(parv[6]) > 255)
		parv[6][255] = '\0';
	    reason = parv[6];
	}
	name = parv[2];
	user = parv[3];
	host = parv[4];
	if (sendto_slaves(cptr, parv, command) < 0)
		return 0;
	if (parv[5]) {
	    if (*parv[5] == 'E')
		command = "ELINE";
	    tline_time = (tline_time = atol(parv[5])) ? tline_time : 0;
	}

    } else { /* Client command */

	char	*p;
	int	nonwild = 0;

	if (!MyClient(sptr)) {
	    sendto_one(sptr, err_str(ERR_NOPRIVILEGES, parv[0]));
	    return 0;
	}

	if (*command == 'U')
	    tline_time = -1;
	else if (parv[2])
    	    tline_time = (tline_time = atol(parv[2])) ? tline_time : 0;
	else
	    tline_time = 0;

	if ((tline_time < 0) && (*command != 'U')) {
	    sendto_one(sptr, ":%s NOTICE %s :Use UN%s <hostmask> command instead", 
		    me.name, parv[0], command);
	    return 0;
	}
	if ((parc < 2) && (*command == 'U')) {
	    sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]), command);
	    return 0;
	} else {
	    if ((parc < 3) && (*command != 'U')) {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]), command);
		return 0;
	    }
	}
	name = parv[1];
	if ((p = index(parv[1], '!'))) {
	    *p = '\0';
	    user = ++p;
	} else {
	    name = NULL;
	    user = parv[1];
	}
	if ((p = index(user, '@'))) {
	    *p = '\0';
	    host = ++p;
	} else {
	    host = user;
	    user = NULL;
	}
	if (EmptyString(name))
	    name = "*";
	else
	    collapse(name);
	if (EmptyString(user))
	    user = "*";
	else
	    collapse(user);
	collapse(host);
	nonwild = nonwilds(name, user, host);
	if (*command != 'U' && (nonwild < NONWILDCHARS)) {
	    sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]), command);
	    return 0;	
	}	    
	Debug((DEBUG_INFO, "%d non wild chars found in mask %s@%s", 
			nonwild, user, host));
	if (*command == 'U')
	    reason = "";
	else
	    if (parc < 4 || !parv[3] || !*parv[3])
		reason = "No reason";
	    else {
		if (strlen(parv[3]) > 255) {
		    parv[3][255] = '\0';
		}
		reason = parv[3];
	    }
    }
    if (!host || !user || !name)
	return -1;
    if (strlen(host) > (size_t) HOSTLEN ||
	strlen(user) > (size_t) USERLEN ||
	strlen(name) > (size_t) UNINICKLEN)
	 return -1;

    for (str = reason; *str; str++)
	    if (*str == IRCDCONF_DELIMITER)
		*str = ',';
  
#ifdef USE_ISERV
    /*
    * iserv command:
    * <type>:<user@hostmask|user@ipmask>:<reason>:<nick>:<port>:<expiration_timestamp>
    */
    {
	aConfItem *aconf = NULL, *tmp, **confstart, **prev;
	char	type;
	char	buf[USERLEN + HOSTLEN + 2];
	time_t	expire;
	int	conf_delete = 0;

	*buf = '\0';
	type = (*command == 'U') ? command[2] : command[0];
	if (tline_time > now) /* Means we got it in timestamp fmt */
	    expire = tline_time;
	else
	    expire = (tline_time > 0) ? (tline_time * 3600 + time(NULL)) : tline_time;

	if ((expire > 0) && (expire <= now))
	    return -1;
	aconf = make_conf();
	
	switch (type) {
	    case 'K':
		    aconf->status = CONF_KILL;
		    confstart = &kconf;
		    break;
	    case 'E':
		    aconf->status = CONF_EXEMPT;
		    confstart = &econf;
		    break;
	    case 'R':
		    aconf->status = CONF_RUSNETRLINE;
		    confstart = &rconf;
		    break;
	    default:
		    free_conf(aconf);
		    return 0; /* At the moment I decided to return here since sending the line 
				having no idea about what it is for is not smart -skold*/
#if 0
		    goto single_line;
#endif
	}

	sprintf(buf, "%s@%s", user, host);
	DupString(aconf->host, buf);
	DupString(aconf->passwd, reason);
	DupString(aconf->name, name);
	aconf->hold = expire;
	aconf->clients = (tline_time < 0) ? -1 : 1;
	istat.is_confmem += aconf->host ? strlen(aconf->host) + 1 : 0;
	istat.is_confmem += aconf->passwd ? strlen(aconf->passwd) + 1 :0;
	istat.is_confmem += aconf->name ? strlen(aconf->name) + 1 : 0;
	prev = confstart;
	
	while ((tmp = *prev))
	{
	    int match_res = match_tline(aconf, tmp);
	    conf_delete = 0;
	    
	    if (((tmp->hold > 0) && (tmp->hold <= now)) || (tmp->status & CONF_ILLEGAL))
	    {
		*prev = tmp->next; 
		free_conf(tmp);
		continue;
	    }
	    if (match_res >= 0) {
		conf_delete = 1;
		goto next_conf;
	    }
	    if (match_res == -1) {
		if (tline_time >= 0) {
		    aconf->clients = -1;
		}
		conf_delete = 0;
		goto next_conf;
	    }
	    if (match_res == -2) {
		conf_delete = 0;
		goto next_conf;
	    }
	    
	    next_conf:
		if (conf_delete) {
		    tmp->hold = -1;
		    send_tline_one(tmp);
		    *prev = tmp->next;
		    free_conf(tmp);

		}
		else {
		    prev = &(tmp->next);
		}
	}
	if (aconf->clients > 0) {
	    send_tline_one(aconf);
	    aconf->next = *confstart;
	    *confstart = aconf;

#ifdef TLINES_SCAN_INSTANT
	    rehash_tline(confstart, aconf); /* Temporary-lines list is often bigger 
					       than local users list in small nets and 
					       i consider it reasonable to define, */
#else					    /* though the opposite in active networks is */
	    rehashed = 1;		    /* also almost instant. -skold */
#endif
	}
	else {
	    free_conf(aconf);
	}
	return 0;
#if 0
single_line: /* never reached in this code -skold */
	sendto_iserv("%c%c%s@%s%c%s%c%s%c%s%c%d",
		    type, IRCDCONF_DELIMITER,
		    user,
		    host, IRCDCONF_DELIMITER,
		    reason, IRCDCONF_DELIMITER,
		    name,  IRCDCONF_DELIMITER,
		    "", IRCDCONF_DELIMITER,
		    expire);
	sendto_flag(SCH_ISERV, "%c%c%s@%s%c%s%c%s%c%s%c%s",
		type, IRCDCONF_DELIMITER,
				user,
		host, IRCDCONF_DELIMITER,
		reason, IRCDCONF_DELIMITER,
		name,  IRCDCONF_DELIMITER,
		"", IRCDCONF_DELIMITER,
				(expire > 0) ? myctime(expire) : 
				    ((expire == 0) ? "permanent" : "remove"));

#endif
    }
#endif
}

#ifdef DEBUGMODE
/*
 * Actually this one is never called, so it can be removed safely
 * if nobody finds it useful  --erra
 *
 */
int check_tconfs() {
    aConfItem **confstart;
    aConfItem *tmp;
    confstart = &kconf;
    while (tmp = *confstart) {
	Debug((DEBUG_INFO, "Kconf: %s %s %s", tmp->host, tmp->name, tmp->passwd));
	confstart = &(tmp->next);
    }
    confstart = &econf;
    while (tmp = *confstart) {
	Debug((DEBUG_INFO, "Econf: %s %s %s", tmp->host, tmp->name, tmp->passwd));
	confstart = &(tmp->next);
    }
    confstart = &rconf;
    while (tmp = *confstart) {
	Debug((DEBUG_INFO, "Rconf: %s %s %s", tmp->host, tmp->name, tmp->passwd));
	confstart = &(tmp->next);
    }
}
#endif

/* Below are the command wrappers, codeless */
int m_kline(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int     parc;
char    *parv[];
{

    handle_tline(cptr, sptr, parc, parv, "KLINE");
    return 0;
}

int m_rline(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int     parc;
char    *parv[];
{
    handle_tline(cptr, sptr, parc, parv, "RLINE");
    return 0;
}

int m_eline(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int     parc;
char    *parv[];
{
    handle_tline(cptr, sptr, parc, parv, "ELINE");
    return 0;
}

int m_unkline(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int     parc;
char    *parv[];
{
    handle_tline(cptr, sptr, parc, parv, "UNKLINE");
    return 0;
}

int m_unrline(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int     parc;
char    *parv[];
{
    handle_tline(cptr, sptr, parc, parv, "UNRLINE");
    return 0;
}

int m_uneline(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int     parc;
char    *parv[];
{
    handle_tline(cptr, sptr, parc, parv, "UNELINE");
    return 0;
}
