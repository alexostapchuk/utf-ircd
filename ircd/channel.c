/************************************************************************
 *   IRC - Internet Relay Chat, ircd/channel.c
 *   Copyright (C) 1990 Jarkko Oikarinen and
 *                      University of Oulu, Co Center
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
#define CHANNEL_C
#include "s_externs.h"
#undef CHANNEL_C

aChannel *channel = NullChn;

static	void	add_invite(aClient *, aChannel *);
static	u_int	can_join(aClient *, aChannel *, char *);
void	channel_modes(aClient *, char *, char *, aChannel *);
static	int	check_channelmask(aClient *, aClient *, char *);
static	aChannel *get_channel(aClient *, char *, int);
static	int	set_mode(aClient *, aClient *, aChannel *, int *, int,\
				char **, char *,char *);
static	void	free_channel(aChannel *);

static	int	add_modeid(u_int, aClient *, aChannel *, char *);
static	int	del_modeid(u_int, aChannel *, char *);
static	Link	*match_modeid(u_int, aClient *, aChannel *);

static	char	*PartFmt = ":%s PART %s :%s";

/*
 * some buffers for rebuilding channel/nick lists with ,'s
 */
static	char	nickbuf[BUFSIZE];
static	char	modebuf[MODEBUFLEN], parabuf[MODEBUFLEN];
#ifndef USE_OLD8BIT
static	char	buf[MB_LEN_MAX*BUFSIZE];
#else
static	char	buf[BUFSIZE];
#endif

/*
 * return the length (>=0) of a chain of links.
 */
static	int	list_length(lp)
Reg	Link	*lp;
{
	Reg	int	count = 0;

	for (; lp; lp = lp->next)
		count++;
	return count;
}

/*
** find_chasing
**	Find the client structure for a nick name (user) using history
**	mechanism if necessary. If the client is not found, an error
**	message (NO SUCH NICK) is generated. If the client was found
**	through the history, chasing will be 1 and otherwise 0.
*/
static	aClient *find_chasing(sptr, user, chasing)
aClient *sptr;
char	*user;
Reg	int	*chasing;
{
	Reg	aClient *who = find_client(user, (aClient *)NULL);

	if (chasing)
		*chasing = 0;
	if (who)
		return who;
	if (!(who = get_history(user, (long)KILLCHASETIMELIMIT)))
	    {
		sendto_one(sptr, err_str(ERR_NOSUCHNICK, sptr->name), user);
		return NULL;
	    }
	if (chasing)
		*chasing = 1;
	return who;
}

/*
 *  Fixes a string so that the first white space found becomes an end of
 * string marker (`\-`).  returns the 'fixed' string or "*" if the string
 * was NULL length or a NULL pointer.
 */
static	char	*check_string(s)
Reg	char *s;
{
	static	char	star[2] = "*";
	char	*str = s;

	if (BadPtr(s))
		return star;

	for ( ;*s; s++)
		if (isspace(*s))
		    {
			*s = '\0';
			break;
		    }

	return (BadPtr(str)) ? star : str;
}

/*
 * create a string of form "foo!bar@fubar" given foo, bar and fubar
 * as the parameters.  If NULL, they become "*".
 */

static  char *make_nick_user_host(nick, name, host)
Reg     char    *nick, *name, *host;
{
        static  char    namebuf[UNINICKLEN+USERLEN+HOSTLEN+6];
        Reg     char    *s = namebuf;

        bzero(namebuf, sizeof(namebuf));
        nick = check_string(nick);
        strncpyzt(namebuf, nick, UNINICKLEN + 1);
        s += strlen(s);
        *s++ = '!';
        name = check_string(name);
        strncpyzt(s, name, USERLEN + 1);
        s += strlen(s);
        *s++ = '@';
        host = check_string(host);
        strncpyzt(s, host, HOSTLEN + 1);
        s += strlen(s);
        *s = '\0';
        return (namebuf);
}



/*
 * Ban functions to work with mode +b/+e/+I
 */
/* add_modeid - add an id to the list of modes "type" for chptr
 *  (belongs to cptr)
 */

static	int	add_modeid(type, cptr, chptr, modeid)
u_int type;
aClient	*cptr;
aChannel *chptr;
char	*modeid;
{
	Reg	Link	*mode;
	Reg	int	cnt = 0, len = 0;
#ifndef USE_OLD8BIT
	char	ucp[MODEBUFLEN+1];
#endif

	if (MyClient(cptr))
		(void) collapse(modeid);
#ifndef USE_OLD8BIT
	rfcstrtoupper(ucp, modeid, sizeof(ucp));
#endif
	for (mode = chptr->mlist; mode; mode = mode->next)
	    {
		len += strlen(mode->value.cp);
		if (MyClient(cptr))
		    {
			if ((len > MAXBANLENGTH) || (++cnt >= MAXBANS))
			    {
				sendto_one(cptr, err_str(ERR_BANLISTFULL,
							 cptr->name),
					   chptr->chname, modeid);
				return -1;
			    }
			if (type == mode->flags &&
#ifndef USE_OLD8BIT
			    (!match(mode->ucp, ucp) || !match(ucp, mode->ucp))
#else
			    (!match(mode->value.cp, modeid) ||
			    !match(modeid, mode->value.cp))
#endif
			)
			    {
				int rpl;

				if (type == CHFL_BAN)
					rpl = RPL_BANLIST;
				else if (type == CHFL_EXCEPTION)
					rpl = RPL_EXCEPTLIST;
				else
					rpl = RPL_INVITELIST;

				sendto_one(cptr, rpl_str(rpl, cptr->name),
					   chptr->chname, mode->value.cp);
				return -1;
			    }
		    }
#ifndef USE_OLD8BIT
		else if (type == mode->flags && !strcmp(mode->ucp, ucp))
#else
		else if (type == mode->flags && !strcasecmp(mode->value.cp, modeid))
#endif
			return -1;
		
	    }
	mode = make_link();
	istat.is_bans++;
	bzero((char *)mode, sizeof(Link));
	mode->flags = type;
	mode->next = chptr->mlist;
	mode->value.cp = (char *)MyMalloc(len = strlen(modeid)+1);
	istat.is_banmem += len;
	(void)strcpy(mode->value.cp, modeid);
#ifndef USE_OLD8BIT
	mode->ucp = (char *)MyMalloc(len = strlen(ucp)+1);
	(void)strcpy(mode->ucp, ucp);
#endif
	chptr->mlist = mode;
	return 0;
}

/*
 * del_modeid - delete an id belonging to chptr
 * if modeid is null, delete all ids belonging to chptr.
 */
static	int	del_modeid(type, chptr, modeid)
u_int type;
aChannel *chptr;
char	*modeid;
{
	Reg	Link	**mode;
	Reg	Link	*tmp;
#ifndef USE_OLD8BIT
	char	ucp[MODEBUFLEN+1];
#endif

#ifndef USE_OLD8BIT
	rfcstrtoupper(ucp, modeid, sizeof(ucp));
#endif
	if (modeid == NULL)
	    {
	        for (mode = &(chptr->mlist); *mode; mode = &((*mode)->next))
		    if (type == (*mode)->flags)
		        {
			    tmp = *mode;
			    *mode = tmp->next;
			    istat.is_banmem -= (strlen(tmp->value.cp) + 1);
			    istat.is_bans--;
			    MyFree(tmp->value.cp);
#ifndef USE_OLD8BIT
			    MyFree(tmp->ucp);
#endif
			    free_link(tmp);
			    break;
			}
	    }
	else for (mode = &(chptr->mlist); *mode; mode = &((*mode)->next))
		if (type == (*mode)->flags &&
#ifndef USE_OLD8BIT
		    strcmp(ucp, (*mode)->ucp)==0)
#else
		    strcasecmp(modeid, (*mode)->value.cp)==0)
#endif
		    {
			tmp = *mode;
			*mode = tmp->next;
			istat.is_banmem -= (strlen(modeid) + 1);
			istat.is_bans--;
			MyFree(tmp->value.cp);
#ifndef USE_OLD8BIT
			MyFree(tmp->ucp);
#endif
			free_link(tmp);
			break;
		    }
	return 0;
}

/*
 * match_modeid - returns a pointer to the mode structure if matching else NULL
 */
static	Link	*match_modeid(type, cptr, chptr)
u_int type;
aClient *cptr;
aChannel *chptr;
{
	Reg	Link	*tmp;
	char	*s, *t;
#ifndef USE_OLD8BIT
	char	*name = cptr->ucname;
#else
	char	*name = cptr->name;
#endif

	if (!IsPerson(cptr))
		return NULL;

	s = make_nick_user_host(name, cptr->user->username, cptr->user->host);
	t = mystrdup(make_nick_user_host(name, cptr->user->username,
							cptr->sockhost));

	for (tmp = chptr->mlist; tmp; tmp = tmp->next)
		if (tmp->flags == type && (
#ifndef USE_OLD8BIT
			match(tmp->ucp, s) == 0 || match(tmp->ucp, t) == 0
#else
			match(tmp->value.cp, s) == 0 ||
			match(tmp->value.cp, t) == 0
#endif
							))
			break;

	if (!tmp && MyConnect(cptr))
	    {
		char *ip = NULL;

#ifdef 	INET6
		ip = (char *) inetntop(AF_INET6, (char *)&cptr->ip,
				       mydummy, MYDUMMY_SIZE);
#else
		ip = (char *) inetntoa((char *)&cptr->ip);
#endif

		if (strcmp(ip, cptr->sockhost))
		    {
			s = make_nick_user_host(name, cptr->user->username, ip);
	    
			for (tmp = chptr->mlist; tmp; tmp = tmp->next)
				if (tmp->flags == type &&
#ifndef USE_OLD8BIT
				    match(tmp->ucp, s) == 0
#else
				    match(tmp->value.cp, s) == 0
#endif
				)
					break;
		    }
	  }

	MyFree(t);
	return (tmp);
}

/*
 * adds a user to a channel by adding another link to the channels member
 * chain.
 */
static	void	add_user_to_channel(chptr, who, flags)
aChannel *chptr;
aClient *who;
int	flags;
{
	Reg	Link *ptr;
	Reg	int sz = sizeof(aChannel) + strlen(chptr->chname) + 1;

	if (who->user)
	    {
		ptr = make_link();
		ptr->flags = flags;
		ptr->value.cptr = who;
		ptr->next = chptr->members;
		chptr->members = ptr;
		istat.is_chanusers++;
		if (chptr->users++ == 0)
		    {
			istat.is_chan++;
			istat.is_chanmem += sz;
		    }
		if (chptr->users == 1 && chptr->history)
		    {
			/* Locked channel */
			istat.is_hchan--;
			istat.is_hchanmem -= sz;
			/*
			** The modes had been kept, but now someone is joining,
			** they should be reset to avoid desynchs
			** (you wouldn't want to join a +i channel, either)
			**
			** This can be wrong in some cases such as a netjoin
			** which will not complete, or on a mixed net (with
			** servers that don't do channel delay) - kalt
			*/
			if (*chptr->chname != '!')
				bzero((char *)&chptr->mode, sizeof(Mode));
		    }

#ifdef USE_SERVICES
		if (chptr->users == 1)
			check_services_butone(SERVICE_WANT_CHANNEL|
					      SERVICE_WANT_VCHANNEL,
					      NULL, &me, "CHANNEL %s %d",
					      chptr->chname, chptr->users);
		else
			check_services_butone(SERVICE_WANT_VCHANNEL,
					      NULL, &me, "CHANNEL %s %d",
					      chptr->chname, chptr->users);
#endif
		ptr = make_link();
		ptr->flags = flags;
		ptr->value.chptr = chptr;
		ptr->next = who->user->channel;
		who->user->channel = ptr;
		if (!IsQuiet(chptr))
		    {
			who->user->joined++;
			istat.is_userc++;
		    }

		if (!(ptr = find_user_link(chptr->clist, who->from)))
		    {
			ptr = make_link();
			ptr->value.cptr = who->from;
			ptr->next = chptr->clist;
			chptr->clist = ptr;
		    }
		ptr->flags++;
	    }
}

void	remove_user_from_channel(sptr, chptr)
aClient *sptr;
aChannel *chptr;
{
	Reg	Link	**curr;
	Reg	Link	*tmp, *tmp2;

	for (curr = &chptr->members; (tmp = *curr); curr = &tmp->next)
		if (tmp->value.cptr == sptr)
		    {
			/*
			 * if a chanop leaves (no matter how), record
			 * the time to be able to later massreop if
			 * necessary.
			 */
			if (*chptr->chname == '!' &&
			    (tmp->flags & CHFL_CHANOP))
				chptr->reop = timeofday + LDELAYCHASETIMELIMIT;

			*curr = tmp->next;
			free_link(tmp);
			break;
		    }
	for (curr = &sptr->user->channel; (tmp = *curr); curr = &tmp->next)
		if (tmp->value.chptr == chptr)
		    {
			*curr = tmp->next;
			free_link(tmp);
			break;
		    }

	tmp2 = find_user_link(chptr->clist, (sptr->from) ? sptr->from : sptr);

	if (tmp2 && !--tmp2->flags)
		for (curr = &chptr->clist; (tmp = *curr); curr = &tmp->next)
			if (tmp2 == tmp)
			    {
				*curr = tmp->next;
				free_link(tmp);
				break;
			    }
	if (!IsQuiet(chptr))
	    {
		sptr->user->joined--;
		istat.is_userc--;
	    }
#ifdef USE_SERVICES
	if (chptr->users == 1)
		check_services_butone(SERVICE_WANT_CHANNEL|
				      SERVICE_WANT_VCHANNEL, NULL, &me,
				      "CHANNEL %s %d", chptr->chname,
				      chptr->users-1);
	else
		check_services_butone(SERVICE_WANT_VCHANNEL, NULL, &me,
				      "CHANNEL %s %d", chptr->chname,
				      chptr->users-1);
#endif
	if (--chptr->users <= 0)
	    {
		u_int sz = sizeof(aChannel) + strlen(chptr->chname) + 1;

		istat.is_chan--;
		istat.is_chanmem -= sz;
		istat.is_hchan++;
		istat.is_hchanmem += sz;
		free_channel(chptr);
	    }
	istat.is_chanusers--;
}

static	void	change_chan_flag(lp, chptr)
Link	*lp;
aChannel *chptr;
{
	Reg	Link *tmp;
	aClient	*cptr = lp->value.cptr;

	/*
	 * Set the channel members flags...
	 */
	tmp = find_user_link(chptr->members, cptr);
	if (lp->flags & MODE_ADD)
		tmp->flags |= lp->flags & MODE_FLAGS;
	else
	    {
		tmp->flags &= ~lp->flags & MODE_FLAGS;
		if (lp->flags & CHFL_CHANOP)
			tmp->flags &= ~CHFL_UNIQOP;
	    }
	/*
	 * and make sure client membership mirrors channel
	 */
	tmp = find_channel_link(cptr->user->channel, chptr);
	if (lp->flags & MODE_ADD)
		tmp->flags |= lp->flags & MODE_FLAGS;
	else
	    {
		tmp->flags &= ~lp->flags & MODE_FLAGS;
		if (lp->flags & CHFL_CHANOP)
			tmp->flags &= ~CHFL_UNIQOP;
	    }
}

int	is_chan_op(cptr, chptr)
aClient *cptr;
aChannel *chptr;
{
	Reg	Link	*lp;
	int	chanop = 0;

	if (MyConnect(cptr) && IsPerson(cptr) && IsRestricted(cptr) &&
	    *chptr->chname != '&')
		return 0;

	if (chptr && (lp = find_user_link(chptr->members, cptr)))
		chanop = (lp->flags & (CHFL_CHANOP|CHFL_UNIQOP));

	if (chanop)
		chptr->reop = 0;

	return chanop;
}

int	is_chan_anyop(cptr, chptr)
aClient *cptr;
aChannel *chptr;
{
	Reg	Link	*lp;
	int	op = 0;

	if (MyConnect(cptr) && IsPerson(cptr) && IsRestricted(cptr) &&
	    *chptr->chname != '&')
		return 0;

	if (chptr && (lp = find_user_link(chptr->members, cptr)))
		op = (lp->flags & (CHFL_HALFOP|CHFL_CHANOP|CHFL_UNIQOP));

	return op;
}

int	has_voice(cptr, chptr)
aClient *cptr;
aChannel *chptr;
{
	Reg	Link	*lp;

	if (chptr)
		if ((lp = find_user_link(chptr->members, cptr)))
			return (lp->flags & CHFL_VOICE);

	return 0;
}

int	can_send(cptr, chptr)
aClient *cptr;
aChannel *chptr;
{
	Reg	Link	*lp;
	Reg	int	member;

	member = IsMember(cptr, chptr);
	lp = find_user_link(chptr->members, cptr);

	if ((!lp || !(lp->flags & (CHFL_CHANOP | CHFL_HALFOP | CHFL_VOICE))) &&
	    !match_modeid(CHFL_EXCEPTION, cptr, chptr) &&
	    match_modeid(CHFL_BAN, cptr, chptr))
		return (MODE_BAN);

	if (chptr->mode.mode & MODE_MODERATED &&
	    (!lp || !(lp->flags & (CHFL_CHANOP| CHFL_HALFOP | CHFL_VOICE))))
			return (MODE_MODERATED);

	if (chptr->mode.mode & MODE_NOPRIVMSGS && !member)
		return (MODE_NOPRIVMSGS);

	if (chptr->mode.mode & MODE_RECOGNIZED && !Identified(cptr))
		return (MODE_RECOGNIZED);

	if (chptr->mode.mode & MODE_NOCOLOR)
			return (MODE_NOCOLOR);

	return 0;
}

aChannel *find_channel(chname, chptr)
Reg	char	*chname;
Reg	aChannel *chptr;
{
	aChannel *achptr = chptr;

	if (chname && *chname)
		achptr = hash_find_channel((*chname == '@') ?	/* opnotices */
						chname + 1 : chname, chptr);
	return achptr;
}

void	setup_server_channels(mp)
aClient	*mp;
{
	aChannel	*chptr;
	int	smode;

	smode = MODE_MODERATED|MODE_TOPICLIMIT|MODE_NOPRIVMSGS|MODE_ANONYMOUS|
		MODE_QUIET;

	chptr = get_channel(mp, "&ERRORS", CREATE);
	strcpy(chptr->topic, "SERVER MESSAGES: server errors");
	add_user_to_channel(chptr, mp, CHFL_CHANOP);
	chptr->mode.mode = smode;
	chptr = get_channel(mp, "&NOTICES", CREATE);
	strcpy(chptr->topic, "SERVER MESSAGES: warnings and notices");
	add_user_to_channel(chptr, mp, CHFL_CHANOP);
	chptr->mode.mode = smode;
	chptr = get_channel(mp, "&KILLS", CREATE);
	strcpy(chptr->topic, "SERVER MESSAGES: operator and server kills");
	add_user_to_channel(chptr, mp, CHFL_CHANOP);
	chptr->mode.mode = smode;
	chptr = get_channel(mp, "&CHANNEL", CREATE);
	strcpy(chptr->topic, "SERVER MESSAGES: fake modes");
	add_user_to_channel(chptr, mp, CHFL_CHANOP);
	chptr->mode.mode = smode;
	chptr = get_channel(mp, "&NUMERICS", CREATE);
	strcpy(chptr->topic, "SERVER MESSAGES: numerics received");
	add_user_to_channel(chptr, mp, CHFL_CHANOP);
	chptr->mode.mode = smode;
	chptr = get_channel(mp, "&SERVERS", CREATE);
	strcpy(chptr->topic, "SERVER MESSAGES: servers joining and leaving");
	add_user_to_channel(chptr, mp, CHFL_CHANOP);
	chptr->mode.mode = smode;
	chptr = get_channel(mp, "&HASH", CREATE);
	strcpy(chptr->topic, "SERVER MESSAGES: hash tables growth");
	add_user_to_channel(chptr, mp, CHFL_CHANOP);
	chptr->mode.mode = smode;
	chptr = get_channel(mp, "&LOCAL", CREATE);
	strcpy(chptr->topic, "SERVER MESSAGES: notices about local connections");
	add_user_to_channel(chptr, mp, CHFL_CHANOP);
	chptr->mode.mode = smode;
	chptr = get_channel(mp, "&SERVICES", CREATE);
	strcpy(chptr->topic, "SERVER MESSAGES: services joining and leaving");
	add_user_to_channel(chptr, mp, CHFL_CHANOP);
	chptr->mode.mode = smode;
#if defined(USE_IAUTH)
	chptr = get_channel(mp, "&AUTH", CREATE);
	strcpy(chptr->topic,
	       "SERVER MESSAGES: messages from the authentication slave");
	add_user_to_channel(chptr, mp, CHFL_CHANOP);
	chptr->mode.mode = smode;
#endif
#if defined(USE_ISERV)
	chptr = get_channel(mp, "&ISERV", CREATE);
	strcpy(chptr->topic,
	       "SERVER MESSAGES: messages from the configuration slave");
	add_user_to_channel(chptr, mp, CHFL_CHANOP);
	chptr->mode.mode = smode|MODE_SECRET;
#endif
	chptr = get_channel(mp, "&DEBUG", CREATE);
	strcpy(chptr->topic, "SERVER MESSAGES: debug messages [you shouldn't be here! ;)]");
	add_user_to_channel(chptr, mp, CHFL_CHANOP);
	chptr->mode.mode = smode|MODE_SECRET;
#ifdef EXTRA_NOTICES
        chptr = get_channel(mp, "&OPER", CREATE);
        strcpy(chptr->topic, "SERVER MESSAGES: opers-only notices");
        add_user_to_channel(chptr, mp, CHFL_CHANOP);
        chptr->mode.mode = smode|MODE_SECRET;
#endif
#ifdef WALLOPS_TO_CHANNEL
        chptr = get_channel(mp, "&WALLOPS", CREATE);
        strcpy(chptr->topic, "WALLOPS MESSAGES: supermouse-only");
        add_user_to_channel(chptr, mp, CHFL_CHANOP);
        chptr->mode.mode = smode|MODE_SECRET;
#endif

	setup_svchans();
}

/*
 * write the "simple" list of channel modes for channel chptr onto buffer mbuf
 * with the parameters in pbuf.
 */
void	channel_modes(cptr, mbuf, pbuf, chptr)
aClient	*cptr;
Reg	char	*mbuf, *pbuf;
aChannel *chptr;
{
	*mbuf++ = '+';

	if (chptr->mode.mode & MODE_7BIT)
		*mbuf++ = 'z';
	if (chptr->mode.mode & MODE_NOCOLOR)
		*mbuf++ = 'c';		
	if (chptr->mode.mode & MODE_SECRET)
		*mbuf++ = 's';
	else if (chptr->mode.mode & MODE_PRIVATE)
		*mbuf++ = 'p';
	if (chptr->mode.mode & MODE_MODERATED)
		*mbuf++ = 'm';
	if (chptr->mode.mode & MODE_TOPICLIMIT)
		*mbuf++ = 't';
	if (chptr->mode.mode & MODE_INVITEONLY)
		*mbuf++ = 'i';
	if (chptr->mode.mode & MODE_NOPRIVMSGS)
		*mbuf++ = 'n';
	if (chptr->mode.mode & MODE_ANONYMOUS)
		*mbuf++ = 'a';
	if (chptr->mode.mode & MODE_QUIET)
		*mbuf++ = 'q';
	if (chptr->mode.mode & MODE_REOP)
		*mbuf++ = 'r';
	if (chptr->mode.mode & MODE_RECOGNIZED &&
			(!IsServer(cptr) || cptr->serv->version & SV_RUSNET2))
		*mbuf++ = 'R';
	if (chptr->mode.limit)
	    {
		*mbuf++ = 'l';
		if (IsMember(cptr, chptr) || IsServer(cptr))
			sprintf(pbuf, "%d ", chptr->mode.limit);
	    }
	if (*chptr->mode.key)
	    {
		*mbuf++ = 'k';
		if (IsMember(cptr, chptr) || IsServer(cptr))
			(void)strcat(pbuf, chptr->mode.key);
	    }
	*mbuf++ = '\0';
	return;
}

static	void	send_mode_list(cptr, chname, top, mask, flag)
aClient	*cptr;
Link	*top;
int	mask;
char	flag, *chname;
{
	Reg	Link	*lp;
	Reg	char	*cp, *name;
	int	count = 0, send = 0;

	cp = modebuf + strlen(modebuf);
	if (*parabuf)
	{
		/*
		** we have some modes in parabuf,
		** so check how many of them.
		** however, don't count initial '+'
		*/
		count = strlen(modebuf) - 1;
	}
	for (lp = top; lp; lp = lp->next)
	{
		if (!(lp->flags & mask))
			continue;
		if (mask == CHFL_BAN || mask == CHFL_EXCEPTION ||
		    mask == CHFL_INVITE)
			name = lp->value.cp;
		else
			name = lp->value.cptr->name;
		if (strlen(parabuf) + strlen(name) + 10 < (size_t) MODEBUFLEN)
		{
			if (*parabuf)
			{
				(void)strcat(parabuf, " ");
			}
			(void)strcat(parabuf, name);
			count++;
			*cp++ = flag;
			*cp = '\0';
		}
		else
		{
			if (*parabuf)
			{
				send = 1;
			}
		}
		if (count == MAXMODEPARAMS)
		{
			send = 1;
		}
		if (send)
		{
			/*
			** send out MODEs, it's either MAXMODEPARAMS of them
			** or long enough that they filled up parabuf
			*/
			sendto_one(cptr, ":%s MODE %s %s %s",
				   ME, chname, modebuf, parabuf);
			send = 0;
			*parabuf = '\0';
			cp = modebuf;
			*cp++ = '+';
			if (count != MAXMODEPARAMS)
			{
				/*
				** we weren't able to fit another 'name'
				** into parabuf, so we have to send it
				** in another turn, appending it now to
				** empty parabuf and setting count to 1
				*/
				(void)strcpy(parabuf, name);
				*cp++ = flag;
				count = 1;
			}
			else
			{
				count = 0;
			}
			*cp = '\0';
		}
	}
}

/*
 * send "cptr" a full list of the modes for channel chptr.
 */
void	send_channel_modes(cptr, chptr)
aClient *cptr;
aChannel *chptr;
{
	if (check_channelmask(&me, cptr, chptr->chname))
		return;

	*modebuf = *parabuf = '\0';
	channel_modes(cptr, modebuf, parabuf, chptr);

	if (modebuf[1] || *parabuf)
		sendto_one(cptr, ":%s MODE %s %s %s",
			   ME, chptr->chname, modebuf, parabuf);

	*parabuf = '\0';
	*modebuf = '+';
	modebuf[1] = '\0';
	send_mode_list(cptr, chptr->chname, chptr->mlist, CHFL_BAN, 'b');
	send_mode_list(cptr, chptr->chname, chptr->mlist,
		       CHFL_EXCEPTION, 'e');
	send_mode_list(cptr, chptr->chname, chptr->mlist,
		       CHFL_INVITE, 'I');

	/* send out what left in buffer */
	if (modebuf[1] || *parabuf)
		sendto_one(cptr, ":%s MODE %s %s %s",
			   ME, chptr->chname, modebuf, parabuf);
}

/*
 * send "cptr" a full list of the channel "chptr" members and their
 * +ohv status, using NJOIN
 */
void	send_channel_members(cptr, chptr)
aClient *cptr;
aChannel *chptr;
{
	Reg	Link	*lp;
	Reg	aClient *c2ptr;
	Reg	size_t	cnt = 0, len = 0, nlen;

	if (check_channelmask(&me, cptr, chptr->chname) == -1)
		return;

	sprintf(buf, ":%s NJOIN %s :", ME, chptr->chname);
	len = strlen(buf);

	for (lp = chptr->members; lp; lp = lp->next)
	    {
		c2ptr = lp->value.cptr;
		nlen = strlen(c2ptr->name);
		if ((len + nlen) > (size_t) (BUFSIZE - 9)) /* ,@+ \r\n\0 */
		    {
			sendto_one(cptr, "%s", buf);
			sprintf(buf, ":%s NJOIN %s :", ME, chptr->chname);
			len = strlen(buf);
			cnt = 0;
		    }
		if (cnt)
		    {
			buf[len++] = ',';
			buf[len] = '\0';
		    }
		if (lp->flags & (CHFL_UNIQOP|CHFL_CHANOP
				|CHFL_HALFOP|CHFL_VOICE))
		    {
			if (lp->flags & CHFL_UNIQOP)
			    {
				buf[len++] = '@';
				buf[len++] = '@';
			    }
			else
			    {
				if (lp->flags & CHFL_CHANOP)
					buf[len++] = '@';
			    }
			if (lp->flags & CHFL_HALFOP)
				buf[len++] = '%';
			if (lp->flags & CHFL_VOICE)
				buf[len++] = '+';
			buf[len] = '\0';
		    }
		(void)strcpy(buf + len, c2ptr->name);
		len += nlen;
		cnt++;
	    }
	if (*buf && cnt)
		sendto_one(cptr, "%s", buf);

	return;
}

#ifdef TOPICWHOTIME
/*
 * send "cptr" a topic for channel chptr.
 */
void	send_channel_topic(cptr, chptr)
aClient *cptr;
aChannel *chptr;
{
	if (check_channelmask(&me, cptr, chptr->chname) == -1)
		return;

	/* send to capable servers only if topic was set */
	if (chptr->topic_time && cptr->serv->version & SV_RUSNET2
					&& !IsRusnetServices(cptr))
		sendto_one(cptr, ":%s NTOPIC %s %s %ld :%s", ME, chptr->chname,
			   chptr->topic_nick, chptr->topic_time, chptr->topic);
}
#endif

/*
 * m_mode
 * parv[0] - sender
 * parv[1] - target; channels and/or user
 * parv[2] - optional modes
 * parv[n] - optional parameters
 */
int	m_mode(cptr, sptr, parc, parv)
aClient *cptr;
aClient *sptr;
int	parc;
char	*parv[];
{
	int	mcount = 0;
	int	penalty = 0;
	aChannel *chptr;
	char	*name, *p = NULL;

	if (parc < 1)
	    {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]), "MODE");
 	 	return 1;
	    }

	if (MyClient(sptr) && IsRMode(sptr))
		return 5;

	parv[1] = canonize(parv[1]);

	for (name = strtoken(&p, parv[1], ","); name;
	     name = strtoken(&p, NULL, ","))
	    {
		clean_channelname(name);
		chptr = find_channel(name, NullChn);
		if (chptr == NullChn)
		    {
			parv[1] = name;
			penalty += m_umode(cptr, sptr, parc, parv);
			continue;
		    }
		if (check_channelmask(sptr, cptr, name))
		    {
			penalty += 1;
			continue;
		    }
		if (!UseModes(name))
		    {
			sendto_one(sptr, err_str(ERR_NOCHANMODES, parv[0]),
				   name);
			penalty += 1;
			continue;
		    }
		if (parc < 3)	/* Only a query */
		    {
			*modebuf = *parabuf = '\0';
			modebuf[1] = '\0';
			channel_modes(sptr, modebuf, parabuf, chptr);
			sendto_one(sptr, rpl_str(RPL_CHANNELMODEIS, parv[0]),
				   name, modebuf, parabuf);
			penalty += 1;
		    }
		else	/* Check parameters for the channel */
		    {
			/* We must find it now because it may be dropped.
			   Taking IsServer(sptr) here saves resources for
			   server passed mode changes  --erra */
			int chanop = IsServer(sptr) ||
					is_chan_anyop(sptr, chptr);

			if(!(mcount = set_mode(cptr, sptr, chptr, &penalty,
					       parc - 2, parv + 2,
					       modebuf, parabuf)))
				continue;	/* no valid mode change */
			if ((mcount < 0) && MyConnect(sptr) && !IsServer(sptr)
						    && !IsRusnetServices(sptr))
			    {	/* rejected mode change */
				int num = ERR_CHANOPRIVSNEEDED;

				if (IsClient(sptr) && IsRestricted(sptr))
					num = ERR_RESTRICTED;
				sendto_one(sptr, err_str(num, parv[0]), name);
				continue;
			    }
			if (strlen(modebuf) > (size_t)1)
			    {	/* got new mode to pass on */
				if (modebuf[1] == 'R')
					/* RusNet 1.x compatibility */
					sendto_match_servs_v(chptr, cptr,
							     SV_RUSNET2,
							   ":%s MODE %s %s %s",
							     parv[0], name,
							     modebuf, parabuf);
				else
					sendto_match_servs(chptr, cptr,
							   ":%s MODE %s %s %s",
							   parv[0], name,
							   modebuf, parabuf);
				if ((IsServer(cptr) && !chanop &&
					!IsRusnetServices(sptr)) || mcount < 0)
				    {
					sendto_flag(SCH_CHAN,
						    "Fake: %s MODE %s %s %s",
						    parv[0], name, modebuf,
						    parabuf);
					ircstp->is_fake++;
				    }
				else
				    {
					sendto_channel_butserv(chptr, sptr,
						        ":%s MODE %s %s %s",
							parv[0], name,
							modebuf, parabuf);
#ifdef USE_SERVICES
					*modebuf = *parabuf = '\0';
					modebuf[1] = '\0';
					channel_modes(&me, modebuf, parabuf,
						      chptr);
					check_services_butone(SERVICE_WANT_MODE,
						      NULL, sptr,
						      "MODE %s %s",
						      name, modebuf);
#endif
				    }
			   } /* if(modebuf) */
		    } /* else(parc>2) */
	    } /* for (parv1) */
	return penalty;
}

/*
 * Check and try to apply the channel modes passed in the parv array for
 * the client cptr to channel chptr.  The resultant changes are printed
 * into mbuf and pbuf (if any) and applied to the channel.
 */
static	int	set_mode(cptr, sptr, chptr, penalty, parc, parv, mbuf, pbuf)
Reg	aClient *cptr, *sptr;
aChannel *chptr;
int	parc, *penalty;
char	*parv[], *mbuf, *pbuf;
{
	static	Link	chops[MAXMODEPARAMS+3];
	static	u_int32_t flags[] = {
				MODE_PRIVATE,    'p', MODE_SECRET,     's',
				MODE_MODERATED,  'm', MODE_NOPRIVMSGS, 'n',
				MODE_TOPICLIMIT, 't', MODE_INVITEONLY, 'i',
				MODE_ANONYMOUS,  'a', MODE_REOP,       'r',
				MODE_7BIT,	 'z', MODE_NOCOLOR,    'c',
				MODE_RECOGNIZED, 'R',
				0x0, 0x0 };

	Reg	Link	*lp = NULL;
	Reg	char	*curr = parv[0], *cp = NULL;
	Reg u_int32_t	*ip;
	u_int32_t whatt = MODE_ADD;
	int	limitset = 0, count = 0, chasing = 0;
	int	nusers = 0, is_op, ischop, new, len, keychange = 0, opcnt = 0;
	aClient *who;
	Mode	*mode, oldm;
	Link	*plp = NULL;
	int	compat = -1; /* to prevent mixing old/new modes */

	*mbuf = *pbuf = '\0';
	if (parc < 1)
		return 0;

	mode = &(chptr->mode);
	bcopy((char *)mode, (char *)&oldm, sizeof(Mode));
	is_op = IsServer(sptr) || IsRusnetServices(sptr) ||
					is_chan_op(sptr, chptr);
	ischop = is_op || is_chan_anyop(sptr, chptr);
	new = mode->mode;

	while (curr && *curr && count >= 0)
	    {
		if (compat == -1 && *curr != '-' && *curr != '+')
			compat = (*curr == 'R') ? 1 : 0;

		switch (*curr)
		{
		case '+':
			whatt = MODE_ADD;
			break;
		case '-':
			whatt = MODE_DEL;
			break;
		case 'O':
			if (parc > 0)
			    {
			if (*chptr->chname == '!')
			    {
			    if (IsMember(sptr, chptr))
			        {
					*penalty += 1;
					parc--;
					/* Feature: no other modes after this query */
     	                           *(curr+1) = '\0';
					for (lp = chptr->members; lp; lp = lp->next)
						if (lp->flags & CHFL_UNIQOP)
						    {
							sendto_one(sptr,
							   rpl_str(RPL_UNIQOPIS,
								   sptr->name),
								   chptr->chname,
							   lp->value.cptr->name);
							break;
						    }
					if (!lp)
						sendto_one(sptr,
							   err_str(ERR_NOSUCHNICK,
								   sptr->name),
							   chptr->chname);
					break;
				    }
				else /* not IsMember() */
				    {
					if (!IsServer(sptr) &&
							!IsRusnetServices(sptr))
					    {
						sendto_one(sptr, err_str(ERR_NOTONCHANNEL, sptr->name),
							    chptr->chname);
						*(curr+1) = '\0';
						break;
					    }
				    }
			    }
			else /* *chptr->chname != '!' */
				sendto_one(cptr, err_str(ERR_UNKNOWNMODE,
					sptr->name), *curr, chptr->chname);
					*(curr+1) = '\0';
					break;
			    }
			/*
			 * is this really ever used ?
			 * or do ^G & NJOIN do the trick?
			 */
			if (*chptr->chname != '!' || whatt == MODE_DEL ||
			    !IsServer(sptr))
			    {
				*penalty += 1;
				--parc;
				parv++;
				break;
			    }
		case 'o' :
		case 'h' :
		case 'v' :
			*penalty += 1;
			if (--parc <= 0)
				break;
			parv++;
			*parv = check_string(*parv);
			if (opcnt >= MAXMODEPARAMS && (MyClient(sptr)
						|| opcnt >= MAXMODEPARAMS + 1))
				break;
			if (!IsServer(sptr) && !IsMember(sptr, chptr)
					    && !IsRusnetServices(sptr))
			    {
				sendto_one(sptr, err_str(ERR_NOTONCHANNEL,
								 sptr->name),
					    chptr->chname);
				break;
			    }
			/*
			 * Check for nickname changes and try to follow these
			 * to make sure the right client is affected by the
			 * mode change.
			 */
			if (!(who = find_chasing(sptr, parv[0], &chasing)))
				break;
	  		if (!IsMember(who, chptr))
			    {
				sendto_one(sptr, err_str(ERR_USERNOTINCHANNEL,
							 sptr->name),
					   parv[0], chptr->chname);
				break;
			    }

			if (IsRMode(who))
				break;

			if (who == cptr && whatt == MODE_ADD && *curr == 'o')
				break;

			if (whatt == MODE_ADD)
			    {
				lp = &chops[opcnt++];
				lp->value.cptr = who;
				lp->flags = (*curr == 'O') ? MODE_UNIQOP:
			    			(*curr == 'o') ? MODE_CHANOP:
			    			(*curr == 'h') ? MODE_HALFOP:
								  MODE_VOICE;
				lp->flags |= MODE_ADD;
			    }
			else if (whatt == MODE_DEL)
			    {
				lp = &chops[opcnt++];
				lp->value.cptr = who;
				lp->flags = (*curr == 'o') ? MODE_CHANOP:
			    			(*curr == 'h') ? MODE_HALFOP:
								  MODE_VOICE;
				lp->flags |= MODE_DEL;
			    }
			if (plp && plp->flags == lp->flags &&
			    plp->value.cptr == lp->value.cptr)
			    {
				opcnt--;
				break;
			    }
			plp = lp;
			count++;
			*penalty += 2;
			break;
		case 'k':
			*penalty += 1;
			if (--parc <= 0)
				break;
			parv++;
			/* check now so we eat the parameter if present */
			if (keychange)
				break;
			{
				Reg	u_char	*s;

				for (s = (u_char *)*parv; *s; )
				    {
					/* comma cannot be inside key --Beeth */
					if (*s == ',') 
						*s = '.';
					if (*s > 0x7f)
						if (*s > 0xa0)
							*s++ &= 0x7f;
						else
							*s = '\0';
					else
						s++;
				    }
			}

			if (!**parv)
				break;

			*parv = check_string(*parv);

			if (opcnt >= MAXMODEPARAMS && (MyClient(sptr) ||
						opcnt >= MAXMODEPARAMS + 1))
				break;

			if (whatt == MODE_ADD)
			    {
				if (*mode->key && !IsServer(cptr))
					sendto_one(cptr, err_str(ERR_KEYSET,
						   cptr->name), chptr->chname);
				else if (ischop &&
				    (!*mode->key || IsServer(cptr)))
				    {
					if (**parv == ':')
						/* this won't propagate right*/
						break;
					lp = &chops[opcnt++];
					lp->value.cp = *parv;
					if (strlen(lp->value.cp) >
					    (size_t) KEYLEN)
						lp->value.cp[KEYLEN] = '\0';
					lp->flags = MODE_KEY|MODE_ADD;
					keychange = 1;
				    }
			    }
			else if (whatt == MODE_DEL)
			    {
				if (*mode->key && (ischop || IsServer(cptr)))
				    {
					lp = &chops[opcnt++];
					lp->value.cp = mode->key;
					lp->flags = MODE_KEY|MODE_DEL;
					keychange = 1;
				    }
			    }
			count++;
			*penalty += 2;
			break;
		case 'b':
			*penalty += 1;
			if (--parc <= 0)	/* ban list query */
			    {
				/* Feature: no other modes after ban query */
				*(curr+1) = '\0';	/* Stop MODE # bb.. */
				for (lp = chptr->mlist; lp; lp = lp->next)
					if (lp->flags == CHFL_BAN)
						sendto_one(cptr,
							   rpl_str(RPL_BANLIST,
								   cptr->name),
							   chptr->chname,
							   lp->value.cp);
				sendto_one(cptr, rpl_str(RPL_ENDOFBANLIST,
					   cptr->name), chptr->chname);
				break;
			    }
			parv++;
			if (BadPtr(*parv))
				break;
			if (opcnt >= MAXMODEPARAMS && (MyClient(sptr) ||
						opcnt >= MAXMODEPARAMS + 1))
				break;
			if (whatt == MODE_ADD)
			    {
				if (**parv == ':')
					/* this won't propagate right */
					break;
				lp = &chops[opcnt++];
				lp->value.cp = *parv;
				lp->flags = MODE_ADD|MODE_BAN;
			    }
			else if (whatt == MODE_DEL)
			    {
				lp = &chops[opcnt++];
				lp->value.cp = *parv;
				lp->flags = MODE_DEL|MODE_BAN;
			    }
			count++;
			*penalty += 2;
			break;
		case 'e':
			*penalty += 1;
			if (--parc <= 0)	/* exception list query */
			    {
				/* Feature: no other modes after query */
				*(curr+1) = '\0';	/* Stop MODE # bb.. */
#ifdef MODES_RESTRICTED
				if (!IsAnOper(sptr) && !IsServer(sptr) &&
						!IsMember(sptr, chptr) &&
						!IsRusnetServices(sptr))
				    {
					sendto_one(sptr,
						   err_str(ERR_NOTONCHANNEL,
								 sptr->name),
								chptr->chname);
					break;
				    }
#endif
				for (lp = chptr->mlist; lp; lp = lp->next)
					if (lp->flags == CHFL_EXCEPTION)
						sendto_one(cptr,
						   rpl_str(RPL_EXCEPTLIST,
								   cptr->name),
							   chptr->chname,
							   lp->value.cp);
				sendto_one(cptr, rpl_str(RPL_ENDOFEXCEPTLIST,
					   cptr->name), chptr->chname);
				break;
			    }
			parv++;
			if (BadPtr(*parv))
				break;
			if (opcnt >= MAXMODEPARAMS && (MyClient(sptr) ||
						opcnt >= MAXMODEPARAMS + 1))
				break;
			if (whatt == MODE_ADD)
			    {
				if (**parv == ':')
					/* this won't propagate right */
					break;
				lp = &chops[opcnt++];
				lp->value.cp = *parv;
				lp->flags = MODE_ADD|MODE_EXCEPTION;
			    }
			else if (whatt == MODE_DEL)
			    {
				lp = &chops[opcnt++];
				lp->value.cp = *parv;
				lp->flags = MODE_DEL|MODE_EXCEPTION;
			    }
			count++;
			*penalty += 2;
			break;
		case 'I':
			*penalty += 1;
			if (--parc <= 0)	/* invite list query */
			    {
				/* Feature: no other modes after query */
				*(curr+1) = '\0';	/* Stop MODE # bb.. */
#ifdef MODES_RESTRICTED
				if ( !(IsServer(sptr) || IsAnOper(sptr)
				    || IsRusnetServices(sptr)) )
				{
				    if (!IsMember(sptr, chptr))
				    {
					sendto_one(sptr,
						   err_str(ERR_NOTONCHANNEL,
								 sptr->name),
								chptr->chname);
					break;
				    }
				    if (!is_chan_anyop(sptr, chptr))
				    {
					sendto_one(sptr,
						   err_str(ERR_CHANOPRIVSNEEDED,
								 sptr->name),
								chptr->chname);
					break;
				    }
				}
#endif
				for (lp = chptr->mlist; lp; lp = lp->next)
					if (lp->flags == CHFL_INVITE)
						sendto_one(cptr,
						   rpl_str(RPL_INVITELIST,
								   cptr->name),
							   chptr->chname,
							   lp->value.cp);
				sendto_one(cptr, rpl_str(RPL_ENDOFINVITELIST,
					   cptr->name), chptr->chname);
				break;
			    }
			parv++;
			if (BadPtr(*parv))
				break;
			if (opcnt >= MAXMODEPARAMS && (MyClient(sptr) ||
						opcnt >= MAXMODEPARAMS + 1))
				break;
			if (whatt == MODE_ADD)
			    {
				if (**parv == ':')
					/* this won't propagate right */
					break;
				lp = &chops[opcnt++];
				lp->value.cp = *parv;
				lp->flags = MODE_ADD|MODE_INVITE;
			    }
			else if (whatt == MODE_DEL)
			    {
				lp = &chops[opcnt++];
				lp->value.cp = *parv;
				lp->flags = MODE_DEL|MODE_INVITE;
			    }
			count++;
			*penalty += 2;
			break;
		case 'l':
			*penalty += 1;
			/*
			 * limit 'l' to only *1* change per mode command but
			 * eat up others.
			 */
			if (limitset || !ischop)
			    {
				if (whatt == MODE_ADD && --parc > 0)
					parv++;
				break;
			    }
			if (whatt == MODE_DEL)
			    {
				limitset = 1;
				nusers = 0;
				count++;
				break;
			    }
			if (--parc > 0)
			    {
				if (BadPtr(*parv))
					break;
				if (opcnt >= MAXMODEPARAMS && (MyClient(sptr)
						|| opcnt >= MAXMODEPARAMS + 1))
					break;
				if (!(nusers = atoi(*++parv)))
					break;
				lp = &chops[opcnt++];
				lp->flags = MODE_ADD|MODE_LIMIT;
				limitset = 1;
				count++;
				*penalty += 2;
				break;
			    }
			sendto_one(cptr, err_str(ERR_NEEDMOREPARAMS,
				   cptr->name), "MODE +l");
			break;
		case 'i' : /* falls through for default case */
			if (whatt == MODE_DEL && ischop)
				while ((lp = chptr->invites))
					del_invite(lp->value.cptr, chptr);
		default:
			*penalty += 1;
			for (ip = flags; *ip; ip += 2)
				if (*(ip+1) == *curr)
					break;

			if (*ip)
			    {
				if (*ip == MODE_ANONYMOUS &&
				    whatt == MODE_DEL && *chptr->chname == '!')
					sendto_one(sptr,
					   err_str(ERR_UNIQOPRIVSNEEDED,
						   sptr->name), chptr->chname);
				else if (((*ip == MODE_ANONYMOUS &&
					   whatt == MODE_ADD &&
					   *chptr->chname == '#') ||
					  (*ip == MODE_REOP &&
					   *chptr->chname != '!')) &&
					 !IsServer(sptr))
					sendto_one(cptr,
						   err_str(ERR_UNKNOWNMODE,
						   sptr->name), *curr,
						   chptr->chname);
				else if ((*ip == MODE_REOP ||
					  *ip == MODE_ANONYMOUS) &&
					 !IsServer(sptr) &&
					 !(is_chan_op(sptr,chptr) &CHFL_UNIQOP)
					 && *chptr->chname == '!')
					/* 2 modes restricted to UNIQOP */
					sendto_one(sptr,
					   err_str(ERR_UNIQOPRIVSNEEDED,
						   sptr->name), chptr->chname);
				else
				    {
					/*
				        ** If the channel is +s, ignore +p
					** modes coming from a server.
					** (Otherwise, it's desynch'ed) -kalt
					*/
					if (whatt == MODE_ADD &&
					    *ip == MODE_PRIVATE &&
					    IsServer(sptr) &&
					    (new & MODE_SECRET))
						break;
					if (whatt == MODE_ADD)
					    {
						if (*ip == MODE_PRIVATE)
							new &= ~MODE_SECRET;
						else if (*ip == MODE_SECRET)
							new &= ~MODE_PRIVATE;
						new |= *ip;
					    }
					else
						new &= ~*ip;
					count++;
					*penalty += 2;
				    }
			    }
			else if (!IsServer(cptr))
				sendto_one(cptr, err_str(ERR_UNKNOWNMODE,
					   cptr->name), *curr, chptr->chname);
			break;
		}
		curr++;
		/*
		 * Make sure modes strings such as "+m +t +p +i" are parsed
		 * fully.
		 */
		if (!*curr && parc > 0)
		    {
			curr = *++parv;
			parc--;
		    }
		/*
		 * Make sure old and new (+R) modes won't get mixed
		 * together on the same line
		 */
		if (MyClient(sptr) && curr && *curr != '-' && *curr != '+')
		{
			if (*curr == 'R')
			    {
				if (compat == 0)
					*curr = '\0';
			    }
			else if (compat == 1)
				*curr = '\0';
		}
	    } /* end of while loop for MODE processing */

	/* clear whatt for future usage */
	whatt = 0;

	/* reuse compat for chasing modes */
	compat = 0;

	for (ip = flags; *ip; ip += 2)
		if ((*ip & new) && !(*ip & oldm.mode))
		    {
			if (compat == 0)
			    {
				*mbuf++ = '+';
				compat = 1;
			    }
			if (ischop)
			  {
				mode->mode |= *ip;
				if (*ip == MODE_ANONYMOUS && MyPerson(sptr))
				  {
				      sendto_channel_butone(NULL, &me, chptr, 0,
						":%s NOTICE %s :The anonymous "
						"flag is being set on channel "
						"%s.", ME, chptr->chname,
								chptr->chname);
				      sendto_channel_butone(NULL, &me, chptr, 0,
						":%s NOTICE %s :Be aware that "
						"anonymity on IRC is NOT "
						"securely enforced!", ME,
								chptr->chname);
				  }
			  }
			*mbuf++ = *(ip+1);
		    }

	for (ip = flags; *ip; ip += 2)
		if ((*ip & oldm.mode) && !(*ip & new))
		    {
			if (compat != -1)
			    {
				*mbuf++ = '-';
				compat = -1;
			    }
			if (ischop)
				mode->mode &= ~*ip;
			*mbuf++ = *(ip+1);
		    }

	if (limitset && !nusers && mode->limit)
	    {
		if (compat != -1)
		    {
			*mbuf++ = '-';
			compat = -1;
		    }
		mode->mode &= ~MODE_LIMIT;
		mode->limit = 0;
		*mbuf++ = 'l';
	    }

	/*
	 * Reconstruct "+beIkOov" chain.
	 */
	if (opcnt)
	    {
		Reg	int	i = 0;
		Reg	char	c = '\0';
		char	*user, *host, numeric[16];

/*		if (opcnt > MAXMODEPARAMS)
			opcnt = MAXMODEPARAMS;
*/
		for (; i < opcnt; i++)
		    {
			lp = &chops[i];
			/*
			 * make sure we have correct mode change sign
			 */
			if (whatt != (lp->flags & (MODE_ADD|MODE_DEL)))
			{
				if (lp->flags & MODE_ADD)
				    {
					*mbuf++ = '+';
					whatt = MODE_ADD;
				    }
				else
				    {
					*mbuf++ = '-';
					whatt = MODE_DEL;
				    }
			}
			len = strlen(pbuf);
			/*
			 * get c as the mode char and tmp as a pointer to
			 * the paramter for this mode change.
			 */
			switch(lp->flags & MODE_WPARAS)
			{
			case MODE_CHANOP :
				c = 'o';
				cp = lp->value.cptr->name;
				break;
			case MODE_UNIQOP :
				c = 'O';
				cp = lp->value.cptr->name;
				break;
			case MODE_HALFOP :
				c = 'h';
				cp = lp->value.cptr->name;
				break;
			case MODE_VOICE :
				c = 'v';
				cp = lp->value.cptr->name;
				break;
			case MODE_BAN :
				c = 'b';
				cp = lp->value.cp;
				if ((user = index(cp, '!')))
					*user++ = '\0';
				if ((host = rindex(user ? user : cp, '@')))
					*host++ = '\0';
				cp = make_nick_user_host(cp, user, host);
				if (user)
					*(--user) = '!';
				if (host)
					*(--host) = '@';
				break;
			case MODE_EXCEPTION :
				c = 'e';
				cp = lp->value.cp;
				if ((user = index(cp, '!')))
					*user++ = '\0';
				if ((host = rindex(user ? user : cp, '@')))
					*host++ = '\0';
				cp = make_nick_user_host(cp, user, host);
				if (user)
					*(--user) = '!';
				if (host)
					*(--host) = '@';
				break;
			case MODE_INVITE :
				c = 'I';
				cp = lp->value.cp;
				if ((user = index(cp, '!')))
					*user++ = '\0';
				if ((host = rindex(user ? user : cp, '@')))
					*host++ = '\0';
				cp = make_nick_user_host(cp, user, host);
				if (user)
					*(--user) = '!';
				if (host)
					*(--host) = '@';
				break;
			case MODE_KEY :
				c = 'k';
				cp = lp->value.cp;
				break;
			case MODE_LIMIT :
				c = 'l';
				(void)sprintf(numeric, "%-15d", nusers);
				if ((cp = index(numeric, ' ')))
					*cp = '\0';
				cp = numeric;
				break;
			}

			if (len + strlen(cp) + 2 > (size_t) MODEBUFLEN)
				break;
			/*
			 * pass on +/-o/v regardless of whether they are
			 * redundant or effective but check +b's to see if
			 * it existed before we created it.
			 */
			switch(lp->flags & MODE_WPARAS)
			{
			case MODE_KEY :
				*mbuf++ = c;
				(void)strcat(pbuf, cp);
				len += strlen(cp);
				(void)strcat(pbuf, " ");
				len++;
				if (!ischop)
					break;
				if (strlen(cp) > (size_t) KEYLEN)
					*(cp+KEYLEN) = '\0';
				if (whatt == MODE_ADD)
					strncpyzt(mode->key, cp,
						  sizeof(mode->key));
				else
					*mode->key = '\0';
				break;
			case MODE_LIMIT :
				*mbuf++ = c;
				(void)strcat(pbuf, cp);
				len += strlen(cp);
				(void)strcat(pbuf, " ");
				len++;
				if (!ischop)
					break;
				mode->limit = nusers;
				break;
			case MODE_CHANOP : /* fall through case */
				if (
				    is_op && lp->value.cptr == sptr &&
				    lp->flags == (MODE_CHANOP|MODE_DEL))
					chptr->reop = timeofday + 
						LDELAYCHASETIMELIMIT;
			case MODE_UNIQOP :
			case MODE_HALFOP :
				/* only allow half-ops to -h themselves */
				if (!(is_op || (lp->flags & MODE_DEL &&
						lp->value.cptr == sptr)))
					break;
			case MODE_VOICE :
				*mbuf++ = c;
				(void)strcat(pbuf, cp);
				len += strlen(cp);
				(void)strcat(pbuf, " ");
				len++;
				if (ischop)
					change_chan_flag(lp, chptr);
				break;
			case MODE_BAN :
				if (ischop &&
				    (((whatt & MODE_ADD) &&
				      !add_modeid(CHFL_BAN, sptr, chptr, cp))||
				     ((whatt & MODE_DEL) &&
				      !del_modeid(CHFL_BAN, chptr, cp))))
				    {
					*mbuf++ = c;
					(void)strcat(pbuf, cp);
					len += strlen(cp);
					(void)strcat(pbuf, " ");
					len++;
				    }
				break;
			case MODE_EXCEPTION :
				if (ischop &&
				    (((whatt & MODE_ADD) &&
			      !add_modeid(CHFL_EXCEPTION, sptr, chptr, cp))||
				     ((whatt & MODE_DEL) &&
				      !del_modeid(CHFL_EXCEPTION, chptr, cp))))
				    {
					*mbuf++ = c;
					(void)strcat(pbuf, cp);
					len += strlen(cp);
					(void)strcat(pbuf, " ");
					len++;
				    }
				break;
			case MODE_INVITE :
				if (ischop &&
				    (((whatt & MODE_ADD) &&
			      !add_modeid(CHFL_INVITE, sptr, chptr, cp))||
				     ((whatt & MODE_DEL) &&
				      !del_modeid(CHFL_INVITE, chptr, cp))))
				    {
					*mbuf++ = c;
					(void)strcat(pbuf, cp);
					len += strlen(cp);
					(void)strcat(pbuf, " ");
					len++;
				    }
				break;
			}
		    } /* for (; i < opcnt; i++) */
	    } /* if (opcnt) */

	*mbuf++ = '\0';
	return ischop ? count : -count;
}

static	u_int	can_join(sptr, chptr, key)
aClient	*sptr;
Reg	aChannel *chptr;
char	*key;
{
	Link	*lp = NULL, *banned;

	if (chptr->users == 0 && (bootopt & BOOT_PROT) && 
	    chptr->history != 0 && *chptr->chname != '!')
		return (timeofday > chptr->history) ? 0 : ERR_UNAVAILRESOURCE;

	if (IsRusnetServices(sptr))
		return 0;

	for (lp = sptr->user->invited; lp; lp = lp->next)
		if (lp->value.chptr == chptr)
			break;

	if ((banned = match_modeid(CHFL_BAN, sptr, chptr)))
	{
		if (match_modeid(CHFL_EXCEPTION, sptr, chptr))
			banned = NULL;
		else if (lp == NULL)
			return (ERR_BANNEDFROMCHAN);
	}

	if (chptr->chname[0] == '&')
	{
	        if (!IsAnOper(sptr) && (!strcmp(chptr->chname, "&ISERV") ||
					!strcmp (chptr->chname, "&OPER") ||
					!strcmp (chptr->chname, "&LOCAL")))
	        	return (ERR_INVITEONLYCHAN);
		if (!IsOper(sptr) && !strcmp(chptr->chname, "&WALLOPS"))
			return (ERR_INVITEONLYCHAN);
	}
	
	if (!IsOper(sptr))
	{
	if ((chptr->mode.mode & MODE_INVITEONLY)
	    && !match_modeid(CHFL_INVITE, sptr, chptr) && (lp == NULL))
		return (ERR_INVITEONLYCHAN);

	if (*chptr->mode.key && (BadPtr(key) || strcasecmp(chptr->mode.key, key)))
		return (ERR_BADCHANNELKEY);

	if (chptr->mode.limit &&
	    (chptr->users >= chptr->mode.limit) && (lp == NULL))
		return (ERR_CHANNELISFULL);
	}

	if ((chptr->mode.mode & MODE_RECOGNIZED)
	    && !(sptr->user->flags & FLAGS_IDENTIFIED))
		return (ERR_UNRECOGNIZED);

	if (chptr->mode.mode & MODE_7BIT)
	{
		char *i;

		for (i = sptr->name; *i != 0; i++)
		    if (*i & 0x80)
			return (ERR_7BIT);
	}

	if (banned)
		sendto_channel_butone(&me, &me, chptr, 1,
					":%s NOTICE @%s :%s carries an "
					"invitation (overriding ban on %s).",
					ME, chptr->chname,
					sptr->name, banned->value.cp);
	return 0;
}

/*
** Remove bells and commas from channel name
*/

void	clean_channelname(ch)
Reg	char *ch;
{
	for (ch++; *ch; ch++)
#ifdef USE_OLD8BIT
		if (*ch <= 0x20 || 
			(*ch >= 0x7F && *ch < 0xA3) ||
			(*ch > 0xA7 && *ch < 0xB3 && *ch != 0xAD) ||
			(*ch > 0xB7 && *ch < 0xC0 && *ch != 0xBD))
			*ch = 'R';
		if (*ch == 0x2C) {
			*ch = '\0';
			return;
		}
#else
		if (*ch == '\007' || *ch == ' ' || *ch == ',')
		    {
			*ch = '\0';
			return;
		    }
#endif
}

/*
** Return -1 if mask is present and doesnt match our server name.
*/
static	int	check_channelmask(sptr, cptr, chname)
aClient	*sptr, *cptr;
char	*chname;
{
	Reg	char	*s, *t;

	if (*chname == '&' && IsServer(cptr))
		return -1;
	s = rindex(chname, ':');
	if (!s)
		return 0;
	if ((t = index(s, '\007')))
		*t = '\0';

	s++;
	if (match(s, ME) || (IsServer(cptr) && match(s, cptr->name)))
	    {
		if (MyClient(sptr))
			sendto_one(sptr, err_str(ERR_BADCHANMASK, sptr->name),
				   chname);
		if (t)
			*t = '\007';
		return -1;
	    }
	if (t)
		*t = '\007';
	return 0;
}

/*
**  Get Channel block for i (and allocate a new channel
**  block, if it didn't exists before).
*/
static	aChannel *get_channel(cptr, chname, flag)
aClient *cptr;
char	*chname;
int	flag;
    {
	Reg	aChannel *chptr;
	size_t	len;
	char newchname[MB_LEN_MAX*CHANNELLEN+1]; /* chname may be const --LoSt */

	if (BadPtr(chname))
		return NULL;

	len = strlen(chname);
	if (MyClient(cptr))
	{
#if defined(LOCALE_STRICT_NAMES) && !defined(USE_OLD8BIT)
	    char ch8[CHANNELLEN];
	    char *nch, *ch2 = ch8;
	    conversion_t *conv = conv_get_conversion(CHARSET_8BIT);

	    /* do validation of channel name: remove all but CHARSET_8BIT chars */
	    len = conv_do_out(conv, chname, strlen(chname), &ch2, CHANNELLEN);
	    for (nch = ch2; nch < ch2+len; nch++)
	    {
		if (!isvalid(*nch) && (*nch <= 0x20 || *nch >= 0x7f))
		    *nch = 'R';	/* 0x21 ... 0x7e are valid for channels */
	    }
	    nch = newchname;
	    if (len && ch2 != chname) /* if no conversion descriptor then ch2==chname */
		len = conv_do_in(conv, ch2, len, &nch, MB_CUR_MAX*CHANNELLEN);
	    else if (len)
	    {
		strncpyzt(newchname, chname, CHANNELLEN+1);
		len = strlen(newchname);
	    }
	    conv_free_conversion(conv);
	    if (len == 0)
		return NULL;
	    newchname[len] = '\0';
	    Debug((DEBUG_DEBUG, "Channel name validation: \"%s\" -> \"%s\"",
		    chname, newchname));
	    chname = newchname;
#else
	    if (len > CHANNELLEN)	/* never can be after conv_do_* --LoSt */
	    {
#if defined(IRCD_CHANGE_LOCALE) && !defined(USE_OLD8BIT)
		int	wlen = CHANNELLEN;
		int	chs;
		size_t	rlen = len;

#endif
		/* note: it may be weird if cutted in middle of multibyte char */
		strncpyzt(newchname, chname, sizeof(newchname)); /* non-const */
#if defined(IRCD_CHANGE_LOCALE) && !defined(USE_OLD8BIT)
		/* check channel name length now to be CHANNELLEN max */
		for (len = 0; newchname[len] != '\0' && wlen > 0; wlen--)
		    {
			if ((chs = mblen(&newchname[len], rlen)) < 1)
			    chs = 1; /* in case of errors */
			rlen -= chs;
			len += chs;
		    }
#else
		len = CHANNELLEN;
#endif
		newchname[len] = '\0';
		Debug((DEBUG_DEBUG, "Channel name validation: \"%s\" -> \"%s\"",
			chname, newchname));
		chname = newchname;
	    }
#endif
	}
	if ((chptr = find_channel(chname, (aChannel *)NULL)))
		return (chptr);
	if (flag != CREATE)
		return NULL;
	chptr = (aChannel *)MyMalloc(sizeof(aChannel));
	bzero((char *)chptr, sizeof(aChannel));
	chptr->chname = MyMalloc(len+1);
	strncpyzt(chptr->chname, chname, len+1);
	if (channel)
		channel->prevch = chptr;
	chptr->nextch = channel;
	/* bzero must have taken care of the following two */
#if 0
	chptr->prevch = NULL;
	chptr->history = 0;
#endif
	channel = chptr;
	(void)add_to_channel_hash_table(chname, chptr);
	return chptr;
}

static	void	add_invite(cptr, chptr)
aClient *cptr;
aChannel *chptr;
{
	Reg	Link	*inv, **tmp;

	del_invite(cptr, chptr);
	/*
	 * delete last link in chain if the list is max length
	 */
	if (list_length(cptr->user->invited) >= MAXCHANNELSPERUSER)
	    {
/*		This forgets the channel side of invitation     -Vesa
		inv = cptr->user->invited;
		cptr->user->invited = inv->next;
		free_link(inv);
*/
		del_invite(cptr, cptr->user->invited->value.chptr);
	    }
	/*
	 * add client to channel invite list
	 */
	inv = make_link();
	inv->value.cptr = cptr;
	inv->next = chptr->invites;
	chptr->invites = inv;
	istat.is_useri++;
	/*
	 * add channel to the end of the client invite list
	 */
	for (tmp = &(cptr->user->invited); *tmp; tmp = &((*tmp)->next))
		;
	inv = make_link();
	inv->value.chptr = chptr;
	inv->next = NULL;
	(*tmp) = inv;
	istat.is_invite++;
}

/*
 * Delete Invite block from channel invite list and client invite list
 */
void	del_invite(cptr, chptr)
aClient *cptr;
aChannel *chptr;
{
	Reg	Link	**inv, *tmp;

	for (inv = &(chptr->invites); (tmp = *inv); inv = &tmp->next)
		if (tmp->value.cptr == cptr)
		    {
			*inv = tmp->next;
			free_link(tmp);
			istat.is_invite--;
			break;
		    }

	for (inv = &(cptr->user->invited); (tmp = *inv); inv = &tmp->next)
		if (tmp->value.chptr == chptr)
		    {
			*inv = tmp->next;
			free_link(tmp);
			istat.is_useri--;
			break;
		    }
}

/*
**  The last user has left the channel, free data in the channel block,
**  and eventually the channel block itself.
*/
static	void	free_channel(chptr)
aChannel *chptr;
{
	Reg	Link *tmp;
	Link	*obtmp;
	int	len = sizeof(aChannel) + strlen(chptr->chname) + 1, now = 0;

        if (chptr->history == 0 || timeofday >= chptr->history)
		/* no lock, nor expired lock, channel is no more, free it */
		now = 1;

	if (*chptr->chname != '!' || now)
	    {
		while ((tmp = chptr->invites))
			del_invite(tmp->value.cptr, chptr);
		
		tmp = chptr->mlist;
		while (tmp)
		    {
			obtmp = tmp;
			tmp = tmp->next;
			istat.is_banmem -= (strlen(obtmp->value.cp) + 1);
			istat.is_bans--;
			MyFree(obtmp->value.cp);
			free_link(obtmp);
		    }
		chptr->mlist = NULL;
	    }

	if (now)
	    {
		istat.is_hchan--;
		istat.is_hchanmem -= len;
		if (chptr->prevch)
			chptr->prevch->nextch = chptr->nextch;
		else
			channel = chptr->nextch;
		if (chptr->nextch)
			chptr->nextch->prevch = chptr->prevch;
		del_from_channel_hash_table(chptr);

		if (*chptr->chname == '!' && close_chid(chptr->chname+1))
			cache_chid(chptr);
		else
		    {
			MyFree(chptr->chname);
			MyFree(chptr);
		    }
	    }
}

/*
** m_join
**	parv[0] = sender prefix
**	parv[1] = channel
**	parv[2] = channel password (key)
*/
int	m_join(cptr, sptr, parc, parv)
Reg	aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	static	char	jbuf[BUFSIZE], cbuf[BUFSIZE];
	Reg	Link	*lp;
	Reg	aChannel *chptr;
	Reg	char	*name, *key = NULL;
	u_int	i, j, tmplen, flags = 0;
	char	*p = NULL, *p2 = NULL, *s, chop[5];

	if (parc < 2 || *parv[1] == '\0')
	    {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]), "JOIN");
		return 1;
	    }

	*jbuf = '\0';
	*cbuf = '\0';
#if defined(EXTRA_NOTICES) && defined(JOIN_NOTICES)
	if (MyClient(sptr))
	    sendto_flag(SCH_OPER, "%s (%s@%s) is joining %s", sptr->name, 
		 sptr->user->username, sptr->sockhost, parv[1]);
#endif
	/*
	** Rebuild list of channels joined to be the actual result of the
	** JOIN.  Note that "JOIN 0" is the destructive problem.
	** Also note that this can easily trash the correspondance between
	** parv[1] and parv[2] lists.
	*/
	for (i = 0, j = 0, name = strtoken(&p, parv[1], ","); name;
	     name = strtoken(&p, NULL, ","))
	    {
		if (MyClient(sptr) && IsRMode(sptr))
		{
		    if (j > 0)
			break;
		    else
			j++;
		}

		if (check_channelmask(sptr, cptr, name) == -1)
			continue;
		if (*name == '&' && !MyConnect(sptr))
			continue;
		if (*name == '0' && !atoi(name))
		    {
			(void)strcpy(jbuf, "0");
			continue;
		    }
		if (MyClient(sptr))
		    {
			clean_channelname(name);

			if (IsRMode(sptr) && (*name == '!' || *name == '&'))
			{
				sendto_one(sptr,
					err_str(ERR_RESTRICTED, parv[0]));
				continue;
			}
		    }
		if (*name == '!')
		    {
			chptr = NULL;
			/*
			** !channels are special:
			**	!!channel is supposed to be a new channel,
			**		and requires a unique name to be built.
			**		( !#channel is obsolete )
			**	!channel cannot be created, and must already
			**		exist.
			*/
			if (*(name+1) == '\0' ||
			    (*(name+1) == '#' && *(name+2) == '\0') ||
			    (*(name+1) == '!' && *(name+2) == '\0'))
			    {
				if (MyClient(sptr))
					sendto_one(sptr,
						   err_str(ERR_NOSUCHCHANNEL,
							   parv[0]), name);
				continue;
			    }
			if (*name == '!' && (*(name+1) == '#' ||
					     *(name+1) == '!'))
			    {
				if (!MyClient(sptr))
				    {
					sendto_flag(SCH_NOTICE,
				   "Invalid !%c channel from %s for %s",
						    *(name+1),
						    get_client_name(cptr,TRUE),
						    sptr->name);
					continue;
				    }
#if 0
				/*
				** Note: creating !!!foo, e.g. !<ID>!foo is
				** a stupid thing to do because /join !!foo
				** will not join !<ID>!foo but create !<ID>foo
				** Some logic here could be reversed, but only
				** to find that !<ID>foo would be impossible to
				** create if !<ID>!foo exists.
				** which is better? it's hard to say -kalt
				*/
				if (*(name+3) == '!')
				    {
					sendto_one(sptr,
						   err_str(ERR_NOSUCHCHANNEL,
							   parv[0]), name);
					continue;
				    }
#endif
				chptr = hash_find_channels(name+2, NULL);
				if (chptr)
				    {
					sendto_one(sptr,
						   err_str(ERR_TOOMANYTARGETS,
							   parv[0]),
						   "Duplicate", name,
						   "Join aborted.");
					continue;
				    }
				if (check_chid(name+2))
				    {
					/*
					 * This is a bit wrong: if a channel
					 * rightfully ceases to exist, it
 					 * can still be *locked* for up to
					 * 2*CHIDNB^3 seconds (~24h)
					 * Is it a reasonnable price to pay to
					 * ensure shortname uniqueness? -kalt
					 */
					sendto_one(sptr,
                                                   err_str(ERR_UNAVAILRESOURCE,
							   parv[0]), name);
					continue;
				    }
				sprintf(buf, "!%.*s%s", CHIDLEN, get_chid(),
					name+2);
				name = buf;
			    }
			else if (!find_channel(name, NullChn) &&
				 !(*name == '!' && *name != 0 &&
				   (chptr = hash_find_channels(name+1, NULL))))
			    {
				if (MyClient(sptr))
					sendto_one(sptr,
						   err_str(ERR_NOSUCHCHANNEL,
							   parv[0]), name);
				if (!IsServer(cptr))
					continue;
				/* from a server, it is legitimate */
			    }
			else if (chptr)
			    {
				/* joining a !channel using the short name */
				if (MyConnect(sptr) &&
				    hash_find_channels(name+1, chptr))
				    {
					sendto_one(sptr,
						   err_str(ERR_TOOMANYTARGETS,
							   parv[0]),
						   "Duplicate", name,
						   "Join aborted.");
					continue;
				    }
				name = chptr->chname;
			    }
		    }
		if (!IsChannelName(name) ||
		    (*name == '!' && IsChannelName(name+1)))
		    {
			if (MyClient(sptr))
				sendto_one(sptr, err_str(ERR_NOSUCHCHANNEL,
					   parv[0]), name);
			continue;
		    }
		tmplen = strlen(name);
		if (i + tmplen + 2 /* comma and \0 */
			>= sizeof(jbuf) )
		{
			/* We would overwrite the jbuf with comma and
			** channel name (possibly shortened), so we just
			** silently ignore it and the rest of JOIN --B. */
			break;
		}
		if (*jbuf)
		{
			jbuf[i++] = ',';
		}
		(void)strcpy(jbuf + i, name);
		i += tmplen;
	    }

	p = NULL;
	if (parv[2])
		key = strtoken(&p2, parv[2], ",");
	parv[2] = NULL;	/* for m_names call later, parv[parc] must == NULL */
	for (name = strtoken(&p, jbuf, ","); name;
	     key = (key) ? strtoken(&p2, NULL, ",") : NULL,
	     name = strtoken(&p, NULL, ","))
	    {
		/*
		** JOIN 0 sends out a part for all channels a user
		** has joined.
		*/
		if (*name == '0' && !atoi(name))
		    {
			if (sptr->user->channel == NULL)
				continue;
			while ((lp = sptr->user->channel))
			    {
				chptr = lp->value.chptr;
				sendto_channel_butserv(chptr, sptr,
						PartFmt,
						parv[0], chptr->chname,
						IsAnonymous(chptr) ? "None" :
						(key ? key : parv[0]));
				remove_user_from_channel(sptr, chptr);
			    }
			sendto_match_servs(NULL, cptr, ":%s JOIN 0 :%s",
					   parv[0], key ? key : parv[0]);
			continue;
		    }

		if (cptr->serv && (s = index(name, '\007')))
			*s++ = '\0';
		else
			clean_channelname(name), s = NULL;

                chptr = get_channel(sptr, name, !CREATE);	
		if (chptr && IsMember(sptr, chptr))
			continue;
		if (MyConnect(sptr) && !(chptr && IsQuiet(chptr)) &&
		    sptr->user->joined >= MAXCHANNELSPERUSER) {
			sendto_one(sptr, err_str(ERR_TOOMANYCHANNELS,
				   parv[0]), name);
			/* can't return, need to send the info everywhere */
			continue;
		}

		if (MyConnect(sptr) &&
		    !strncmp(name, "\x23\x1f\x02\xb6\x03\x34\x63\x68\x02\x1f",
			     10))
		    {
			sptr->exitc = EXITC_VIRUS;
			return exit_client(sptr, sptr, &me, "Virus Carrier");
		    }

		if (chptr)
			tmplen = 0;
		else
		    {
			chptr = get_channel(sptr, name, CREATE);
			tmplen = 1;
		    }

		if (s == NULL)		/* accept any names in transit  -LoSt */
		    name = chptr->chname;	/* it might be changed above */

		if (!chptr ||
		    (MyConnect(sptr) && (i = can_join(sptr, chptr, key))))
		    {
			sendto_one(sptr, err_str(i, parv[0]), name);
			/* Once it is just created we need to just free it back
			 * --erra */
			if (chptr && tmplen)
				free_channel(chptr);
			continue;
		    }

		/*
		** local client is first to enter previously nonexistant
		** channel so make them (rightfully) the Channel
		** Operator.
		*/
		flags = 0;
		chop[0] = '\0';
		if (MyConnect(sptr) && UseModes(name) &&
		    (!IsRestricted(sptr) || (*name == '&')) && !chptr->users
			&& !(chptr->history && *chptr->chname == '!')
			&& !IsRMode(sptr))
		    {
			if (*name == '!')
				strcpy(chop, "\007O");
			else
				strcpy(chop, "\007o");
			s = chop+1; /* tricky */
		    }
		/*
		**  Complete user entry to the new channel (if any)
		*/
		if (s && UseModes(name))
		    {
			if (*s == 'O')
				/*
				 * there can never be another mode here,
				 * because we use NJOIN for netjoins.
				 * here, it *must* be a channel creation. -kalt
				 */
				flags |= CHFL_UNIQOP|CHFL_CHANOP;
			else {
				char *t = s;

				while (*t) {
					switch (*t++) {
					case 'o':
						flags |= CHFL_CHANOP;
						break;
					case 'h':
						flags |= CHFL_HALFOP;
						break;
					case 'v':
						flags |= CHFL_VOICE;
					}
				}
			    }
		    }
		add_user_to_channel(chptr, sptr, flags);
		/*
		** notify all users on the channel
		*/

		sendto_channel_butserv(chptr, sptr, ":%s JOIN :%s",		
						parv[0], name);
		if (s && UseModes(name))
		    {
			/* no need if user is creating the channel */
			if (chptr->users != 1)
				sendto_channel_butserv(chptr, sptr,
						       ":%s MODE %s +%s %s %s",
						       cptr->name, name, s,
						       parv[0],
						       *(s+1)=='v'?parv[0]:"");
			*--s = '\007';
		    }
		/*
		** If s wasn't set to chop+1 above, name is now #chname^Gov
		** again (if coming from a server, and user is +o and/or +v
		** of course ;-)
		** This explains the weird use of name and chop..
		** Is this insane or subtle? -krys
		*/
		if (MyClient(sptr))
		    {
			del_invite(sptr, chptr);
			if (chptr->topic[0] != '\0'){ 
				sendto_one(sptr, rpl_str(RPL_TOPIC, parv[0]),
					   name, chptr->topic);
#ifdef TOPICWHOTIME
			sendto_one(sptr, rpl_str(RPL_TOPICWHOTIME, parv[0]),
					name, chptr->topic_nick,
					chptr->topic_time);
#endif
			}
			parv[1] = name;
			(void)m_names(cptr, sptr, 2, parv);
			if (IsAnonymous(chptr) && !IsQuiet(chptr))
			    {
				sendto_one(sptr, ":%s NOTICE %s :Channel %s has the anonymous flag set.", ME, chptr->chname, chptr->chname);
				sendto_one(sptr, ":%s NOTICE %s :Be aware that anonymity on IRC is NOT securely enforced!", ME, chptr->chname);
			    }
		    }
		/*
	        ** notify other servers
		*/
		if (index(name, ':') || *chptr->chname == '!') /* compat */
			sendto_match_servs(chptr, cptr, ":%s JOIN :%s%s",
					   parv[0], name, chop);
		else if (*chptr->chname != '&')
		    {
			/* ":" (1) "nick" (NICKLEN) " JOIN :" (7), comma (1)
			** possible chop (4), ending \r\n\0 (3) = 16
			** must fit in the cbuf as well! --B. */
			if (strlen(cbuf) + strlen(name) + UNINICKLEN + 16
				 >= sizeof(cbuf))
			{
				sendto_serv_butone(cptr, ":%s JOIN :%s",
					parv[0], cbuf);
				cbuf[0] = '\0';
			}
			if (*cbuf)
				strcat(cbuf, ",");
			strcat(cbuf, name);
			if (*chop)
				strcat(cbuf, chop);
		    }
	    }
	if (*cbuf)
		sendto_serv_butone(cptr, ":%s JOIN :%s", parv[0], cbuf);

	return (MyClient(sptr) && IsRMode(sptr)) ? 10 : 2;
}

/*
** m_njoin
**	parv[0] = sender prefix
**	parv[1] = channel
**	parv[2] = channel members and modes
*/
int	m_njoin(cptr, sptr, parc, parv)
Reg	aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
#ifndef USE_OLD8BIT
	char nbuf[MB_LEN_MAX*BUFSIZE];
#else
	char nbuf[BUFSIZE];
#endif
	char *q, *name, *target, mbuf[MAXMODEPARAMS + 2];
	char *p = NULL;
	int chop, cnt = 0;
	aChannel *chptr = NULL;
	aClient *acptr;

	if (parc < 3 || *parv[2] == '\0')
	    {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]),"NJOIN");
		return 1;
	    }
	*nbuf = '\0'; q = nbuf;
	for (target = strtoken(&p, parv[2], ","); target;
	     target = strtoken(&p, NULL, ","))
	    {
		/* check for modes */
		chop = 0;
		mbuf[0] = '\0';
		if (*target == '@')
		    {
			if (*(target+1) == '@')
			    {
				/* actually never sends in a JOIN ^G */
				if (*(target+2) == '%')
				    {
					if (*(target+3) == '+')
					    {
						strcpy(mbuf, "\007ohv");
						chop = CHFL_UNIQOP|CHFL_CHANOP| \
						  CHFL_HALFOP|CHFL_VOICE;
						name = target + 4;
					    }
					else
					    {
						strcpy(mbuf, "\007oh");
						chop = CHFL_UNIQOP| \
						  CHFL_CHANOP|CHFL_HALFOP;
						name = target + 3;
					    }
				    }
				else
				if (*(target+2) == '+')
				    {
					strcpy(mbuf, "\007ov");
					chop = CHFL_UNIQOP|CHFL_CHANOP| \
					  CHFL_VOICE;
					name = target + 3;
				    }
				else
				    {
					strcpy(mbuf, "\007o");
					chop = CHFL_UNIQOP|CHFL_CHANOP;
					name = target + 2;
				    }
			    }
			else
			if (*(target+1) == '%')
			    {
				if (*(target+2) == '+')
				    {
					strcpy(mbuf, "\007ohv");
					chop = CHFL_CHANOP| \
					  CHFL_HALFOP|CHFL_VOICE;
					name = target + 3;
				    }
				else
				    {
					strcpy(mbuf, "\007oh");
					chop = CHFL_CHANOP|CHFL_HALFOP;
					name = target + 2;
				    }
			    }
			else
			    {
				if (*(target+1) == '+')
				    {
					strcpy(mbuf, "\007ov");
					chop = CHFL_CHANOP|CHFL_VOICE;
					name = target+2;
				    }
				else
				    {
					strcpy(mbuf, "\007o");
					chop = CHFL_CHANOP;
					name = target+1;
				    }
			    }
		    }
		else if (*target == '%')
		    {
			if (*(target+1) == '+')
			    {
				strcpy(mbuf, "\007hv");
				chop = CHFL_HALFOP|CHFL_VOICE;
				name = target + 2;
			    }
			else
			    {
				strcpy(mbuf, "\007h");
				chop = CHFL_HALFOP;
				name = target + 1;
			    }
		    }
		else if (*target == '+')
		    {
			strcpy(mbuf, "\007v");
			chop = CHFL_VOICE;
			name = target+1;
		    }
		else
			name = target;
		/* find user */
		if (!(acptr = find_person(name, (aClient *)NULL))) {
			sendto_flag(SCH_ERROR, "NJOIN to %s: user %s does not exist.", parv[1], name);
			continue;
		}
		/* is user who we think? */
		if (acptr->from != cptr) {
			sendto_flag(SCH_ERROR, "NJOIN to %s: user %s source mismatch %s <-> %s",
				parv[1], name, get_client_name(acptr->from, TRUE),
				get_client_name(cptr, TRUE));
			continue;
		}
		if (check_channelmask(sptr, cptr, parv[1]) == -1)
		    {
			sendto_flag(SCH_DEBUG,
				    "received NJOIN for %s from %s",
				    parv[1],
				    get_client_name(cptr, TRUE));
			return 0;
		    }

		/* get channel pointer */
		if (IsChannelName(parv[1]))
			chptr = get_channel(acptr, parv[1], CREATE);
		if (!chptr)
		    {
			sendto_one(sptr, err_str(ERR_NOSUCHCHANNEL,
						 parv[0]), parv[1]);
			return 0;
		    }
		/* make sure user isn't already on channel */
		if (IsMember(acptr, chptr))
		    {
			sendto_flag(SCH_ERROR, "NJOIN protocol error from %s "
					"for %s (already on %s)",
				    get_client_name(cptr, TRUE), name, parv[1]);
			sendto_one(cptr, "ERROR :NJOIN protocol error");
			continue;
		    }
		/* add user to channel */
		add_user_to_channel(chptr, acptr, UseModes(parv[1]) ? chop :0);
		/* build buffer for NJOIN capable servers */
		if (q != nbuf)
			*q++ = ',';
		while (*target)
			*q++ = *target++;

		/* send join to local users on channel */
		sendto_channel_butserv(chptr, acptr, ":%s JOIN %s",
							name, parv[1]);
		/* build MODE for local users on channel, eventually send it */
		if (*mbuf)
		    {
			if (!UseModes(parv[1]))
			    {
				sendto_one(cptr, err_str(ERR_NOCHANMODES,
							 parv[0]), parv[1]);
				continue;
			    }
			switch (cnt)
			    {
			case 0:
				*parabuf = '\0'; *modebuf = '\0';
				/* fall through */
			case 1:
				strcat(modebuf, mbuf+1);
				cnt += strlen(mbuf+1);
				if (*parabuf)
				    {
					strcat(parabuf, " ");
				    }
				strcat(parabuf, name);
				if (mbuf[2])
				    {
					strcat(parabuf, " ");
					strcat(parabuf, name);
				    }
				break;
			case 2:
				sendto_channel_butserv(chptr, &me,
					       ":%s MODE %s +%s%c %s %s",
						       sptr->name, parv[1], 
						       modebuf, mbuf[1],
						       parabuf, name);
				if (mbuf[2])
				    {
					strcpy(modebuf, mbuf+2);
					strcpy(parabuf, name);
					cnt = 1;
				    }
				else
					cnt = 0;
				break;
			    }
			if (cnt == MAXMODEPARAMS)
			    {
				sendto_channel_butserv(chptr, &me,
						       ":%s MODE %s +%s %s",
						       sptr->name, parv[1],
						       modebuf, parabuf);
				cnt = 0;
			    }
		    }
	    }
	/* send eventual MODE leftover */
	if (cnt)
		sendto_channel_butserv(chptr, &me, ":%s MODE %s +%s %s",
				       sptr->name, parv[1], modebuf, parabuf);

	/* send NJOIN to servers */
	*q = '\0';
	if (nbuf[0])
		sendto_match_servs(chptr, cptr, ":%s NJOIN %s :%s",
					     parv[0], parv[1], nbuf);
	return 0;
}

/*
** m_part
**	parv[0] = sender prefix
**	parv[1] = channel
*/
int	m_part(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
    {
	Reg	aChannel *chptr;
	char	*p = NULL, *name, *comment = "";
	int	size; /* Buffer available for channel list */

	*buf = '\0';

	if (parc < 2 || parv[1][0] == '\0')
	    {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]), "PART");
		return 1;
	    }

	parv[1] = canonize(parv[1]);

	if (MyClient(sptr))
	{
		if (IsRMode(sptr))
			comment = "Restricted";

		/* antispam control */
		else if (parc > 2 && check_spam(sptr, parv[0], parv[2]))
			comment = "Spam discarded";
	}
	else if (!BadPtr(parv[2]))
	{
		comment = parv[2];
		unistrcut(comment, TOPICLEN);
	}

	/*
	** Broadcasted to other servers is ":nick PART #chan,#chans :comment",
	** so we must make sure buf does not contain too many channels or later
	** they get truncated! "10" comes from all fixed chars: ":", " PART "
	** and ending "\r\n\0". We could subtract strlen(comment)+2 here too, 
	** but it's not something we care, is it? :->
	*/
	size = BUFSIZE - strlen(parv[0]) - 10;
	for (; (name = strtoken(&p, parv[1], ",")); parv[1] = NULL)
	    {
		char *msg;

		chptr = get_channel(sptr, name, 0);
		if (!chptr)
		    {
			if (MyPerson(sptr))
				sendto_one(sptr,
					   err_str(ERR_NOSUCHCHANNEL, parv[0]),
					   name);
			continue;
		    }
		if (check_channelmask(sptr, cptr, name))
			continue;
		if (!IsMember(sptr, chptr))
		    {
			sendto_one(sptr, err_str(ERR_NOTONCHANNEL, parv[0]),
				   name);
			continue;
		    }

		msg =	(IsAnonymous(chptr)) ? "None" : 
			(chptr->mode.mode & MODE_NOCOLOR &&
				(comment == parv[2] && strchr(comment, 0x03))) ?
			"message discarded (colors disallowed)" : comment;

		/*
		**  Remove user from the old channel (if any)
		*/
		if (!index(name, ':') && (*chptr->chname != '!'))
		    {	/* channel:*.mask */
			if (*name != '&')
			    {
				/* We could've decreased size by 1 when
				** calculating it, but I left it like that
				** for the sake of clarity. --B. */
				if ((int)(strlen(buf) + strlen(name) + 1)
					> size)
				{
					/* Anyway, if it would not fit in the
					** buffer, send it right away. --B */
					sendto_serv_butone(cptr, PartFmt,
							parv[0], buf, msg);
					*buf = '\0';
				}
				if (*buf)
					(void)strcat(buf, ",");
				(void)strcat(buf, name);
			    }
		    }
		else
			sendto_match_servs(chptr, cptr, PartFmt,
				   	   parv[0], name, msg);

		sendto_channel_butserv(chptr, sptr, PartFmt,
					parv[0], name, msg);
		remove_user_from_channel(sptr, chptr);
	    }
	if (*buf)
		sendto_serv_butone(cptr, PartFmt, parv[0], buf, comment);
	return 4;
    }

/*
** m_kick
**	parv[0] = sender prefix
**	parv[1] = channel
**	parv[2] = client to kick
**	parv[3] = kick comment
*/
int	m_kick(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	aClient *who;
	aChannel *chptr;
	int	chasing = 0, penalty = 0;
	char	*comment, *name, *p = NULL, *user, *p2 = NULL;
	char	*tmp, *tmp2;
	int	mlen, nlen;

	if (parc < 3 || *parv[1] == '\0')
	    {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]), "KICK");
		return 1;
	    }
	if (IsServer(sptr))
		sendto_flag(SCH_NOTICE, "KICK from %s for %s %s",
			    parv[0], parv[1], parv[2]);
	comment = (BadPtr(parv[3])) ? parv[0] : parv[3];
	if (strlen(comment) > (size_t) TOPICLEN)
		comment[TOPICLEN] = '\0';

	/* strlen(":parv[0] KICK ") */
	mlen = 7 + strlen(parv[0]);

	for (; (name = strtoken(&p, parv[1], ",")); parv[1] = NULL)
	    {
		*nickbuf = '\0';
		if (penalty >= MAXPENALTY && MyPerson(sptr))
			break;
		chptr = get_channel(sptr, name, !CREATE);
		if (!chptr)
		    {
			if (MyPerson(sptr))
				sendto_one(sptr,
					   err_str(ERR_NOSUCHCHANNEL, parv[0]),
					   name);
			penalty += 2;
			continue;
		    }
		if (check_channelmask(sptr, cptr, name))
			continue;
		if (!UseModes(name))
		    {
			sendto_one(sptr, err_str(ERR_NOCHANMODES, parv[0]),
				   name);
			penalty += 2;
			continue;
		    }
		if (!IsServer(sptr) && !is_chan_anyop(sptr, chptr)
					&& !IsRusnetServices(sptr))
		    {
			if (!IsMember(sptr, chptr))
				sendto_one(sptr, err_str(ERR_NOTONCHANNEL,
					    parv[0]), chptr->chname);
			else
				sendto_one(sptr, err_str(ERR_CHANOPRIVSNEEDED,
					    parv[0]), chptr->chname);
			penalty += 2;
			continue;
		    }

		nlen = 0;
		tmp = mystrdup(parv[2]);
		for (tmp2 = tmp; (user = strtoken(&p2, tmp2, ",")); tmp2 = NULL)
		    {
			penalty++;
			if (!(who = find_chasing(sptr, user, &chasing)))
				continue; /* No such user left! */
			if (nlen + mlen + strlen(who->name) >
			    (size_t) BUFSIZE - UNINICKLEN)
				continue;
			if (IsMember(who, chptr))
			    if (!IsServer(sptr) &&
				!IsRusnetServices(sptr) &&
				is_chan_op(who, chptr) && /* half-ops cannot */
				!is_chan_op(sptr, chptr)) /* kick full ops   */

				sendto_one(sptr, err_str(ERR_CHANOPRIVSNEEDED,
					    parv[0]), chptr->chname);
			    else
			    {
				sendto_channel_butserv(chptr, sptr,
						":%s KICK %s %s :%s", parv[0],
						name, who->name, comment);
				/* Don't send &local kicks out */
				/* Send !channels and #chan:*.els kicks only
				   to servers that understand those channels.
				   I think it would be better to use different buffers
				   for each type and do them in one sweep at the end.
				   Given MAXPENALTY, however, it wouldn't be used.
				   sendto_match_servs() is used, since it does no action
				   upon chptr being &channel --Beeth */
				if (*chptr->chname != '&' &&
				    *chptr->chname != '!' &&
				    index(chptr->chname, ':') == NULL) {
					if (*nickbuf)
						(void)strcat(nickbuf, ",");
					(void)strcat(nickbuf, who->name);
					nlen += strlen(who->name) + 1; /* 1 for comma --B. */
				    }
				else
					sendto_match_servs(chptr, cptr,
						   ":%s KICK %s %s :%s",
						   parv[0], name,
						   who->name, comment);
				remove_user_from_channel(who,chptr);
				penalty += 2;
				if (penalty >= MAXPENALTY && MyPerson(sptr))
					break;
				if (who == sptr && MyPerson(sptr))
					break; /* breaks "KICK #chan me, someone" */
			    }
			else
				sendto_one(sptr,
					   err_str(ERR_USERNOTINCHANNEL,
					   parv[0]), user, name);
		    } /* loop on parv[2] */
		MyFree(tmp);
		if (*nickbuf)
			sendto_serv_butone(cptr, ":%s KICK %s %s :%s",
					   parv[0], name, nickbuf, comment);
	    } /* loop on parv[1] */

	return penalty;
}

int	count_channels(sptr)
aClient	*sptr;
{
Reg	aChannel	*chptr;
	Reg	int	count = 0;

	for (chptr = channel; chptr; chptr = chptr->nextch)
	    {
		if (chptr->users) /* don't count channels in history */
#ifdef	SHOW_INVISIBLE_LUSERS
			if (!SecretChannel(chptr) || IsAnOper(sptr))
#endif
				count++;
	    }
	return (count);
}

/*
** m_ntopic
**	parv[0] = sender prefix
**	parv[1] = channel name
**	parv[2] = topic set by
**	parv[3] = topic set at
**	parv[4] = topic text
*/
int	m_ntopic(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
    {
	aChannel *chptr = NULL;
	time_t set_at;

	if (parc < 5)
	    {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]),
								"NTOPIC");
		return 1;
	    }

	/* get channel pointer */
	if (IsChannelName(parv[1]))
		chptr = get_channel(sptr, parv[1], 0);

	if (!chptr)
	    {
		sendto_one(sptr, err_str(ERR_NOSUCHCHANNEL, parv[0]), parv[1]);
		return 2;
	    }

	set_at = atol(parv[3]);

	/* only update topic if it's newer than one we have */
	if (set_at > chptr->topic_time)
	    {
		aClient *acptr = NULL;
		Link *lp;

		chptr->topic_time = set_at;
		strcpy(chptr->topic_nick, parv[2]);
		strcpy(chptr->topic, parv[4]);

		/* send topic to local users on channel */
		sendto_channel_butserv(chptr, sptr, ":%s TOPIC %s :%s",
						parv[2], parv[1], parv[4]);

		/* propagate ntopic to servers */
		sendto_match_servs_v(chptr, cptr, SV_RUSNET2,
					":%s NTOPIC %s %s %ld :%s", parv[0],
					parv[1], parv[2], parv[3], parv[4]);
#ifdef USE_SERVICES
		check_services_butone(SERVICE_WANT_TOPIC, NULL,
					sptr, ":%s TOPIC %s :%s",
					parv[2], parv[1], parv[4]);
#endif
		/* send fake topic change to older servers if possible */
		for (lp = chptr->members; lp; lp = lp->next)
			if (!strcasecmp(parv[2], lp->value.cptr->name))
			    {
				acptr = lp->value.cptr;
				break;
			    }

		if (acptr && ((chptr->mode.mode & MODE_TOPICLIMIT) == 0 ||
						is_chan_anyop(acptr, chptr)))
			sendto_match_servs_v(chptr, acptr, SV_RUSNET1,
				":%s TOPIC %s :%s", parv[2], parv[1], parv[4]);
	    }

	return 0;
    }

/*
** m_topic
**	parv[0] = sender prefix
**	parv[1] = channel name(s)
**	parv[2] = topic text
*/
int	m_topic(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
    {
	aChannel *chptr = NullChn;
	char	*topic = NULL, *name, *p = NULL;
	int	penalty = 1;

	if (parc < 2)
	    {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]),
			   "TOPIC");
		return 1;
	    }

	parv[1] = canonize(parv[1]);

	for (; (name = strtoken(&p, parv[1], ",")); parv[1] = NULL)
	    {
		if (!UseModes(name))
		    {
			sendto_one(sptr, err_str(ERR_NOCHANMODES, parv[0]),
				   name);
			continue;
		    }
		if (parc > 1 && IsChannelName(name))
		    {
			chptr = find_channel(name, NullChn);
			if (!chptr || !(IsMember(sptr, chptr)
					|| IsRusnetServices(sptr)))
			    {
				sendto_one(sptr, err_str(ERR_NOTONCHANNEL,
					   parv[0]), name);
				continue;
			    }
			if (parc > 2)
				topic = parv[2];
		    }

		if (!chptr)
		    {
			sendto_one(sptr, rpl_str(RPL_NOTOPIC, parv[0]), name);
			return penalty;
		    }

		if (check_channelmask(sptr, cptr, name))
			continue;
	
		if (!topic)  /* only asking  for topic  */
		    {
			if (chptr->topic[0] == '\0')
				sendto_one(sptr, rpl_str(RPL_NOTOPIC, parv[0]),
					   chptr->chname);
			else {
				sendto_one(sptr, rpl_str(RPL_TOPIC, parv[0]),
					   chptr->chname, chptr->topic);
#ifdef TOPICWHOTIME
				sendto_one(sptr, rpl_str(RPL_TOPICWHOTIME, parv[0]),
					  chptr->chname, chptr->topic_nick,
					  chptr->topic_time);
#endif
			}

		    } 
		else if ((chptr->mode.mode & MODE_TOPICLIMIT) == 0 ||
			 is_chan_anyop(sptr, chptr) || IsRusnetServices(sptr))
		    {	/* setting a topic */
			strncpyzt(chptr->topic, topic, sizeof(chptr->topic));
#ifdef TOPICWHOTIME
			if (!IsRusnetServices(sptr) || !chptr->topic_nick) {
				strcpy(chptr->topic_nick, sptr->name);
				chptr->topic_time = timeofday;
			}
#endif
			sendto_match_servs(chptr, cptr,":%s TOPIC %s :%s",
					   parv[0], chptr->chname,
					   chptr->topic);
			sendto_channel_butserv(chptr, sptr, ":%s TOPIC %s :%s",
					       parv[0],
					       chptr->chname, chptr->topic);
#ifdef USE_SERVICES
			check_services_butone(SERVICE_WANT_TOPIC,
					      NULL, sptr, ":%s TOPIC %s :%s",
					      parv[0], chptr->chname, 
					      chptr->topic);
#endif
			penalty += 2;
		    }
		else
		      sendto_one(sptr, err_str(ERR_CHANOPRIVSNEEDED, parv[0]),
				 chptr->chname);
	    }
	return penalty;
    }

/*
** m_invite
**	parv[0] - sender prefix
**	parv[1] - user to invite
**	parv[2] - channel number
*/
int	m_invite(aClient *cptr _UNUSED_, aClient *sptr, int parc, char *parv[])
{
	aClient *acptr;
	aChannel *chptr;

	if (parc < 3 || *parv[1] == '\0')
	    {
		sendto_one(sptr, err_str(ERR_NEEDMOREPARAMS, parv[0]),
			   "INVITE");
		return 1;
	    }

	if (MyClient(sptr) && IsRMode(sptr)) 
	    {
		sendto_one(sptr, err_str(ERR_RESTRICTED, parv[0]));
		return 10;
	    }

	if (!(acptr = find_person(parv[1], (aClient *)NULL)))
	    {
		sendto_one(sptr, err_str(ERR_NOSUCHNICK, parv[0]), parv[1]);
		return 1;
	    }
	clean_channelname(parv[2]);
	if (check_channelmask(sptr, acptr->user->servp->bcptr, parv[2]))
		return 1;
	if (*parv[2] == '&' && !MyClient(acptr))
		return 1;
	chptr = find_channel(parv[2], NullChn);

	if (!chptr)
	{
	    if (IsRusnetServices(sptr))
	    {
		sendto_prefix_one(acptr, sptr, ":%s INVITE %s :%s",
				  parv[0], parv[1], parv[2]);
		if (MyConnect(sptr))
		    {
        	        sendto_one(sptr, rpl_str(RPL_INVITING, parv[0]),
	                           acptr->name, parv[2]);
			if (acptr->user->flags & FLAGS_AWAY)
				sendto_one(sptr, rpl_str(RPL_AWAY, parv[0]),
					   acptr->name, (acptr->user->away) ? 
					   acptr->user->away : "Gone");
		    }
		return 3;
	    }
	    else {
		sendto_one(sptr, err_str(ERR_NOSUCHCHANNEL, parv[0]), parv[2]);
		return 1;
	    }
	}

	if (!IsMember(sptr, chptr) && !IsRusnetServices(sptr))
	    {
		sendto_one(sptr, err_str(ERR_NOTONCHANNEL, parv[0]), parv[2]);
		return 1;
	    }

	if (IsMember(acptr, chptr))
	    {
		sendto_one(sptr, err_str(ERR_USERONCHANNEL, parv[0]),
			   parv[1], parv[2]);
		return 1;
	    }

	if ((chptr->mode.mode & MODE_INVITEONLY) && !is_chan_anyop(sptr, chptr)
						&& !IsRusnetServices(sptr))
	    {
		sendto_one(sptr, err_str(ERR_CHANOPRIVSNEEDED,
			   parv[0]), chptr->chname);
		return 1;
	    }

	if (MyConnect(sptr))
	    {
		sendto_one(sptr, rpl_str(RPL_INVITING, parv[0]),
			   acptr->name, ((chptr) ? (chptr->chname) : parv[2]));
		if (acptr->user->flags & FLAGS_AWAY)
			sendto_one(sptr, rpl_str(RPL_AWAY, parv[0]),
				   acptr->name,
				   (acptr->user->away) ? acptr->user->away :
				   "Gone");
	    }

	if (MyConnect(acptr))
		if (chptr && sptr->user && (is_chan_anyop(sptr, chptr)
						|| IsRusnetServices(sptr)))
			add_invite(acptr, chptr);

	sendto_prefix_one(acptr, sptr, ":%s INVITE %s :%s",parv[0],
			  acptr->name, ((chptr) ? (chptr->chname) : parv[2]));
	return 2;
}


/*
** m_list
**      parv[0] = sender prefix
**      parv[1] = channel
*/
int	m_list(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
    {
	aChannel *chptr;
	char	*name, *p = NULL;
	int	rlen = 0;

	if (parc > 2 &&
	    hunt_server(cptr, sptr, ":%s LIST %s %s", 2, parc, parv))
		return 10;
	if (BadPtr(parv[1]) && sptr->user)
		for (chptr = channel; chptr; chptr = chptr->nextch)
		    {
			if (!chptr->users ||	/* empty locked channel */
			    (SecretChannel(chptr) && !IsMember(sptr, chptr)))
				continue;
			name = ShowChannel(sptr, chptr) ? chptr->chname : NULL;
			rlen += sendto_one(sptr, rpl_str(RPL_LIST, parv[0]),
				   name ? name : "*", chptr->users,
				   name ? chptr->topic : "");
			if (!MyConnect(sptr) && rlen > CHREPLLEN)
				break;
		    }
	else {
		parv[1] = canonize(parv[1]);
		for (; (name = strtoken(&p, parv[1], ",")); parv[1] = NULL)
		    {
			chptr = find_channel(name, NullChn);
			if (chptr && ShowChannel(sptr, chptr) && sptr->user)
			    {
				rlen += sendto_one(sptr, rpl_str(RPL_LIST,
						   parv[0]), name,
						   chptr->users, chptr->topic);
				if (!MyConnect(sptr) && rlen > CHREPLLEN)
					break;
			    }
			if (*name == '!')
			    {
				chptr = NULL;
				while ((chptr = hash_find_channels(name + 1,
									chptr)))
				    {
					int scr = SecretChannel(chptr) &&
							!IsMember(sptr, chptr);
					rlen += sendto_one(sptr,
							   rpl_str(RPL_LIST,
								   parv[0]),
							   chptr->chname,
							   (scr) ? -1 :
							   chptr->users,
							   (scr) ? "" :
							   chptr->topic);
					if (!MyConnect(sptr) &&
					    rlen > CHREPLLEN)
						break;
				    }		
			    }
		     }
	}
	if (!MyConnect(sptr) && rlen > CHREPLLEN)
		sendto_one(sptr, err_str(ERR_TOOMANYMATCHES, parv[0]),
			   !BadPtr(parv[1]) ? parv[1] : "*");
	sendto_one(sptr, rpl_str(RPL_LISTEND, parv[0]));
	return 2;
    }


/************************************************************************
 * m_names() - Added by Jto 27 Apr 1989
 ************************************************************************/

/*
** m_names
**	parv[0] = sender prefix
**	parv[1] = channel
*/
int	m_names(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{ 
	Reg	aChannel *chptr;
	Reg	aClient *c2ptr;
	Reg	Link	*lp;
	aChannel *ch2ptr = NULL;
	int	idx, flag, len, mlen, rlen = 0;
	char	*s, *para = parc > 1 ? parv[1] : NULL;

	if (parc > 2 &&
	    hunt_server(cptr, sptr, ":%s NAMES %s %s", 2, parc, parv))
		return 10;

	mlen = strlen(ME) + 10; /* server names + : : + spaces + "353" */
	mlen += strlen(parv[0]);
	if (!BadPtr(para))
	    {
		s = index(para, ',');
		if (s && MyConnect(sptr) && s != para)
		    {
			parv[1] = ++s;
			(void)m_names(cptr, sptr, parc, parv);
		    }
		clean_channelname(para);
		ch2ptr = find_channel(para, (aChannel *)NULL);
	    }

	*buf = '\0';

	/*
	 * First, do all visible channels (public and the one user self is)
	 */

	for (chptr = channel; chptr; chptr = chptr->nextch)
	    {
		if (!chptr->users ||	/* locked empty channel */
		    ((chptr != ch2ptr) && !BadPtr(para))) /* 'wrong' channel */
			continue;
		if (!MyConnect(sptr) && (BadPtr(para) || (rlen > CHREPLLEN)))
			break;
		if ((BadPtr(para) || !HiddenChannel(chptr)) &&
		    !ShowChannel(sptr, chptr) && !IsOper(sptr))
			continue; /* -- users on this are not listed */

		/* Find users on same channel (defined by chptr) */

		(void)strcpy(buf, "* ");
		len = strlen(chptr->chname);
		(void)strcpy(buf + 2, chptr->chname);
		(void)strcpy(buf + 2 + len, " :");

		if (PubChannel(chptr))
			*buf = '=';
		else if (SecretChannel(chptr))
			*buf = '@';

		if (IsAnonymous(chptr))
		    {
			if ((lp = find_user_link(chptr->members, sptr)))
			    {
				if (lp->flags & CHFL_CHANOP)
					(void)strcat(buf, "@");
				else if (lp->flags & CHFL_HALFOP)
					(void)strcat(buf, "%");
				else if (lp->flags & CHFL_VOICE)
					(void)strcat(buf, "+");
				(void)strcat(buf, parv[0]);
			    }
			rlen += strlen(buf);
			sendto_one(sptr, rpl_str(RPL_NAMREPLY, parv[0]), buf);
			continue;
		    }
		idx = len + 4; /* channel name + [@=] + 2?? */
		flag = 1;
		for (lp = chptr->members; lp; lp = lp->next)
		    {
			c2ptr = lp->value.cptr;
			if (IsInvisible(c2ptr) && !IsMember(sptr,chptr) &&
								!IsOper(sptr))
				continue;
			if (lp->flags & CHFL_CHANOP)
			    {
				(void)strcat(buf, "@");
				idx++;
			    }
			else if (lp->flags & CHFL_HALFOP)
			    {
				(void)strcat(buf, "%");
				idx++;
			    }
			else if (lp->flags & CHFL_VOICE)
			    {
				(void)strcat(buf, "+");
				idx++;
			    }
			(void)strncat(buf, c2ptr->name, UNINICKLEN);
			idx += strlen(c2ptr->name) + 1;
			flag = 1;
			(void)strcat(buf," ");
			if (mlen + idx + UNINICKLEN + 1 > BUFSIZE - 2)
			    {
				sendto_one(sptr, rpl_str(RPL_NAMREPLY,
					   parv[0]), buf);
				(void)strncpy(buf, "* ", 3);
				(void)strncpy(buf+2, chptr->chname,
						len + 1);
				(void)strcat(buf, " :");
				if (PubChannel(chptr))
					*buf = '=';
				else if (SecretChannel(chptr))
					*buf = '@';
				idx = len + 4;
				flag = 0;
			    }
		    }
		if (flag)
		    {
			rlen += strlen(buf);
			sendto_one(sptr, rpl_str(RPL_NAMREPLY, parv[0]), buf);
		    }
	    } /* for(channels) */
	if (!BadPtr(para))
	    {
		if (!MyConnect(sptr) && (rlen > CHREPLLEN))
			sendto_one(sptr, err_str(ERR_TOOMANYMATCHES, parv[0]),
				   para);
		sendto_one(sptr, rpl_str(RPL_ENDOFNAMES, parv[0]), para);
		return(1);
	    }

	/* Second, do all non-public, non-secret channels in one big sweep */

	(void)strncpy(buf, "* * :", 6);
	idx = 5;
	flag = 0;
	for (c2ptr = client; c2ptr; c2ptr = c2ptr->next)
	    {
  		aChannel *ch3ptr;
		int	showflag = 0, secret = 0;

		if (!IsPerson(c2ptr) || (IsInvisible(c2ptr) && !IsOper(sptr)))
			continue;
		if (!MyConnect(sptr) && (BadPtr(para) || (rlen > CHREPLLEN)))
			break;
		lp = c2ptr->user->channel;
		/*
		 * don't show a client if they are on a secret channel or
		 * they are on a channel sptr is on since they have already
		 * been show earlier. -avalon
		 */
		while (lp)
		    {
			ch3ptr = lp->value.chptr;
			if (PubChannel(ch3ptr) || IsMember(sptr, ch3ptr) ||
								IsOper(sptr))
				showflag = 1;
			if (SecretChannel(ch3ptr) && !IsOper(sptr))
				secret = 1;
			lp = lp->next;
		    }
		if (showflag) /* have we already shown them ? */
			continue;
		if (secret) /* on any secret channels ? */
			continue;
		(void)strncat(buf, c2ptr->name, UNINICKLEN);
		idx += strlen(c2ptr->name) + 1;
		(void)strcat(buf," ");
		flag = 1;
		if (mlen + idx + UNINICKLEN > BUFSIZE - 2)
		    {
			rlen += strlen(buf);
			sendto_one(sptr, rpl_str(RPL_NAMREPLY, parv[0]), buf);
			(void)strncpy(buf, "* * :", 6);
			idx = 5;
			flag = 0;
		    }
	    }
	if (flag)
	    {
		rlen += strlen(buf);
		sendto_one(sptr, rpl_str(RPL_NAMREPLY, parv[0]), buf);
	    }
	if (!MyConnect(sptr) && rlen > CHREPLLEN)
		sendto_one(sptr, err_str(ERR_TOOMANYMATCHES, parv[0]),
			   para ? para : "*");
	/* This is broken.. remove the recursion? */
	sendto_one(sptr, rpl_str(RPL_ENDOFNAMES, parv[0]), "*");
	return 2;
}

void	send_user_joins(cptr, user)
aClient	*cptr, *user;
{
	Reg	Link	*lp;
	Reg	aChannel *chptr;
	Reg	u_int	cnt = 0, len = 0, clen;
	char	 *mask;

	*buf = ':';
	(void)strcpy(buf+1, user->name);
	(void)strcat(buf, " JOIN ");
	len = strlen(user->name) + 7;

	for (lp = user->user->channel; lp; lp = lp->next)
	    {
		chptr = lp->value.chptr;
		if (*chptr->chname == '&')
			continue;
		if ((mask = rindex(chptr->chname, ':')))
			if (match(++mask, cptr->name))
				continue;
		clen = strlen(chptr->chname);
		if ((clen + len) > (size_t) BUFSIZE - 7)
		    {
			if (cnt)
				sendto_one(cptr, "%s", buf);
			*buf = ':';
			(void)strcpy(buf+1, user->name);
			(void)strcat(buf, " JOIN ");
			len = strlen(user->name) + 7;
			cnt = 0;
		    }
		if (cnt)
		    {
			len++;
			(void)strcat(buf, ",");
		    }
		(void)strcpy(buf + len, chptr->chname);
		len += clen;
		if (lp->flags & (CHFL_UNIQOP|CHFL_CHANOP
				|CHFL_HALFOP|CHFL_VOICE))
		    {
			buf[len++] = '\007';
			if (lp->flags & CHFL_UNIQOP) /*this should be useless*/
				buf[len++] = 'O';
			if (lp->flags & CHFL_CHANOP)
				buf[len++] = 'o';
			if (lp->flags & CHFL_HALFOP)
				buf[len++] = 'h';
			if (lp->flags & CHFL_VOICE)
				buf[len++] = 'v';
			buf[len] = '\0';
		    }
		cnt++;
	    }
	if (*buf && cnt)
		sendto_one(cptr, "%s", buf);

	return;
}

#define CHECKFREQ	300
/* consider reoping an opless !channel */
static int
reop_channel(now, chptr)
time_t now;
aChannel *chptr;
{
    Link *lp, op;

    op.value.chptr = NULL;
    if (chptr->users <= 5 && (now - chptr->history > DELAYCHASETIMELIMIT))
	{
	    /* few users, no recent split: this is really a small channel */
	    char mbuf[MAXMODEPARAMS + 1], nbuf[MAXMODEPARAMS*(UNINICKLEN+1)+1];
	    int cnt;
	    
		mbuf[0] = nbuf[0] = '\0';
	    lp = chptr->members;
	    while (lp)
		{
		    if (lp->flags & CHFL_CHANOP)
			{
			    chptr->reop = 0;
			    return 0;
			}
		    if (MyConnect(lp->value.cptr) && !IsRestricted(lp->value.cptr))
			    op.value.cptr = lp->value.cptr;
		    lp = lp->next;
		}
	    if (op.value.cptr == NULL &&
		((now - chptr->reop) < LDELAYCHASETIMELIMIT))
		    /*
		    ** do nothing if no unrestricted local users, 
		    ** unless the reop is really overdue.
		    */
		    return 0;
	    sendto_channel_butone(&me, &me, chptr, 0,
			   ":%s NOTICE %s :Enforcing channel mode +r (%d)",
				   ME, chptr->chname, now - chptr->reop);
	    op.flags = MODE_ADD|MODE_CHANOP;
	    lp = chptr->members;
	    cnt = 0;
	    while (lp)
		{
		    if (cnt == MAXMODEPARAMS)
			{
			    mbuf[cnt] = '\0';
			    if (lp != chptr->members)
				sendto_channel_butone(&me, &me, chptr, 0,
						   ":%s MODE %s +%s %s",
							ME, chptr->chname,
							mbuf, nbuf);
			    cnt = 0;
			    mbuf[0] = nbuf[0] = '\0';
			}
		    if (!(MyConnect(lp->value.cptr) 
			&& IsRestricted(lp->value.cptr)))
			{
			    op.value.cptr = lp->value.cptr;
			    change_chan_flag(&op, chptr);
			    mbuf[cnt++] = 'o';
			    strcat(nbuf, lp->value.cptr->name);
			    strcat(nbuf, " ");
			}
		    lp = lp->next;
		}
	    if (cnt)
		{
		    mbuf[cnt] = '\0';
		    sendto_channel_butone(&me, &me, chptr, 0,
						":%s MODE %s +%s %s",
					   ME, chptr->chname, mbuf, nbuf);
		}
	}
    else
	{
	    time_t idlelimit = now - 
		    MIN((LDELAYCHASETIMELIMIT/2), (2*CHECKFREQ));

	    lp = chptr->members;
	    while (lp)
		{
		    if (lp->flags & CHFL_CHANOP)
			{
			    chptr->reop = 0;
			    return 0;
			}
		    if (MyConnect(lp->value.cptr) &&
			!IsRestricted(lp->value.cptr) &&
			lp->value.cptr->user->last > idlelimit &&
			(op.value.cptr == NULL ||
			 lp->value.cptr->user->last>op.value.cptr->user->last))
			    op.value.cptr = lp->value.cptr;
		    lp = lp->next;
		}
	    if (op.value.cptr == NULL)
		    return 0;
	    sendto_channel_butone(&me, &me, chptr, 0,
			   ":%s NOTICE %s :Enforcing channel mode +r (%d)",
					ME, chptr->chname, now - chptr->reop);
	    op.flags = MODE_ADD|MODE_CHANOP;
	    change_chan_flag(&op, chptr);
	    sendto_channel_butone(&me, &me, chptr, 0, ":%s MODE %s +o %s",
				   ME, chptr->chname, op.value.cptr->name);
	}
    chptr->reop = 0;
    return 1;
}

/*
 * Cleanup locked channels, run frequently.
 *
 * A channel life is defined by its users and the history stamp.
 * It is alive if one of the following is true:
 *	chptr->users > 0		(normal state)
 *	chptr->history >= time(NULL)	(eventually locked)
 * It is locked if empty but alive.
 *
 * The history stamp is set when a remote user with channel op exits.
 */
time_t	collect_channel_garbage(now)
time_t	now;
{
	static	u_int	max_nb = 0; /* maximum of live channels */
	static	u_char	split = 0;
	Reg	aChannel *chptr = channel;
	Reg	u_int	cur_nb = 1, curh_nb = 0, r_cnt = 0;
	aChannel *del_ch;
#ifdef DEBUGMODE
	u_int	del = istat.is_hchan;
#endif
#define SPLITBONUS	(CHECKFREQ - 50)

	collect_chid();

	while (chptr)
	    {
		if (chptr->users == 0)
			curh_nb++;
		else
		    {
			cur_nb++;
			if (*chptr->chname == '!' &&
			    (chptr->mode.mode & MODE_REOP) &&
			    chptr->reop && chptr->reop <= now)
				r_cnt += reop_channel(now, chptr);
		    }
		chptr = chptr->nextch;
	    }
	if (cur_nb > max_nb)
		max_nb = cur_nb;

	if (r_cnt)
		sendto_flag(SCH_CHAN, "Re-opped %u channel(s).", r_cnt);

	/*
	** check for huge netsplits, if so, garbage collection is not really
	** done but make sure there aren't too many channels kept for
	** history - krys
	*/
	if ((2*curh_nb > cur_nb) && curh_nb < max_nb)
		split = 1;
	else
	    {
		split = 0;
		/* no empty channel? let's skip the while! */
		if (curh_nb == 0)
		    {
#ifdef	DEBUGMODE
			sendto_flag(SCH_LOCAL,
		       "Channel garbage: live %u (max %u), hist %u (extended)",
				    cur_nb - 1, max_nb - 1, curh_nb);
#endif
			/* Check again after CHECKFREQ seconds */
			return (time_t) (now + CHECKFREQ);
		    }
	    }

	chptr = channel;
	while (chptr)
	    {
		/*
		** In case we are likely to be split, extend channel locking.
		** most splits should be short, but reality seems to prove some
		** aren't.
		*/
		if (!chptr->history)
		    {
			chptr = chptr->nextch;
			continue;
		    }
		if (split)	/* net splitted recently and we have a lock */
			chptr->history += SPLITBONUS; /* extend lock */

		if ((chptr->users == 0) && (chptr->history <= now))
		    {
			del_ch = chptr;

			chptr = del_ch->nextch;
			free_channel(del_ch);
		    }
		else
			chptr = chptr->nextch;
	    }

#ifdef	DEBUGMODE
	sendto_flag(SCH_LOCAL,
		   "Channel garbage: live %u (max %u), hist %u (removed %u)%s",
		    cur_nb - 1, max_nb - 1, curh_nb, del - istat.is_hchan,
		    (split) ? " split detected" : "");
#endif
	/* Check again after CHECKFREQ seconds */
	return (time_t) (now + CHECKFREQ);
}

#ifndef USE_OLD8BIT
void transcode_channels (conversion_t *old)
{
  static char buff[MB_LEN_MAX*BUFSIZE+1]; /* must be long enough to get any field */
  aChannel *chptr;
  Link *cur;

  for (chptr = channel; chptr; chptr = chptr->nextch)
  {
    /* convert channel name... */
    del_from_channel_hash_table(chptr);
    conv_transcode_realloc(old, chptr->chname, buff);
    add_to_channel_hash_table(chptr->chname, chptr);
    conv_transcode(old, chptr->topic, buff);
#ifdef TOPICWHOTIME
    conv_transcode(old, chptr->topic_nick, buff);
#endif
    for (cur = chptr->mlist; cur; cur = cur->next) /* convert bans,etc. */
    {
      conv_transcode_realloc(old, cur->value.cp, buff);
      conv_transcode_realloc(old, cur->ucp, buff);
    }
  }
}
#endif
