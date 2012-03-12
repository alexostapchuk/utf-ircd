/************************************************************************
 *   IRC - Internet Relay Chat, iauth/mod_dnsbl.c
 *   Copyright (C) 2003 erra@RusNet
 *   based on iauth/mod_socks.c
 *   Copyright (C) 1998 Christophe Kalt
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
#include "a_defines.h"
#define MOD_DNSBL_C
#include "a_externs.h"
#undef MOD_DNSBL_C

/****************************** PRIVATE *************************************/

#define CACHETIME 30

struct hostlog
{
	struct hostlog *next;
	char ip[HOSTLEN+1];
	u_char state; /* 0 = not found, 1 = found, 2 = timeout */
	time_t expire;
};

#define OPT_LOG     	0x001
#define OPT_DENY    	0x002
#define OPT_PARANOID	0x004

#define	OK		0
#define DNSBL_FOUND	1
#define DNSBL_FAILED	2

struct dnsbl_list
{
	char *host;
	char is_whitelist;
	struct dnsbl_list *next;
};

struct dnsbl_private
{
	struct hostlog *cache;
	u_int lifetime;
	u_char options;
	/* stats */
	u_int chitc, chito, chitn, cmiss, cnow, cmax;
	u_int found, failed, good, total, rejects;
	struct dnsbl_list *host_list;
};

/*
 * dnsbl_succeed
 *
 *	Found a host in DNSBL. Deal with it.
 */
static void
dnsbl_succeed(cl, listname, result)
int cl;
char *listname, *result;
{
    struct dnsbl_private *mydata = cldata[cl].instance->data;

    if (mydata->options & OPT_PARANOID || (mydata->options & OPT_DENY &&
		result[0] == '\177' && result[1] == '\0' &&
		result[2] == '\0' /*  && result[3] == '\2' */) )
	{
	    cldata[cl].state |= A_DENY;
	    sendto_ircd("K %d %s %u :found in DNSBL (%s)", cl,
			cldata[cl].itsip, cldata[cl].itsport, listname);
	    mydata->rejects++;
	}
    if (mydata->options & OPT_LOG)
	    sendto_log(ALOG_FLOG, LOG_INFO, "%s: found: %s[%s]",
		       listname, cldata[cl].host, cldata[cl].itsip);
}

/*
 * dnsbl_add_cache
 *
 *	Add an entry to the cache.
 */
static void
dnsbl_add_cache(cl, state)
int cl, state;
{
    struct dnsbl_private *mydata = cldata[cl].instance->data;
    struct hostlog *next;

    if (state == DNSBL_FOUND)
	mydata->found ++;
    else if (state == DNSBL_FAILED)
	mydata->failed ++;
    else /* state == OK */
	mydata->good ++;

    if (mydata->lifetime == 0)
	    return;

    mydata->cnow ++;
    if (mydata->cnow > mydata->cmax)
	    mydata->cmax = mydata->cnow;

    next = mydata->cache;
    mydata->cache = (struct hostlog *)malloc(sizeof(struct hostlog));
    mydata->cache->expire = time(NULL) + mydata->lifetime;
    strcpy(mydata->cache->ip, cldata[cl].itsip);
    mydata->cache->state = state;
    mydata->cache->next = next;
    DebugLog((ALOG_DNSBLC, 0, "dnsbl_add_cache(%d): new cache %s, result=%d",
	      cl, mydata->cache->ip, state));
}

/*
 * dnsbl_check_cache
 *
 *	Check cache for an entry.
 */
static int
dnsbl_check_cache(cl)
int cl;
{
    struct dnsbl_private *mydata = cldata[cl].instance->data;
    struct hostlog **last, *pl;
    time_t now = time(NULL);

    if (!mydata || mydata->lifetime == 0)
                return 0;

    DebugLog((ALOG_DNSBLC, 0, "dnsbl_check_cache(%d): Checking cache for %s",
	      cl, cldata[cl].itsip));

    last = &(mydata->cache);
    while ((pl = *last))
	{
	    DebugLog((ALOG_DNSBLC, 0, "dnsbl_check_cache(%d): cache %s",
		      cl, pl->ip));
	    if (pl->expire < now)
		{
		    DebugLog((ALOG_DNSBLC, 0,
			      "dnsbl_check_cache(%d): free %s (%d < %d)",
			      cl, pl->ip, pl->expire, now));
		    *last = pl->next;
		    free(pl);
		    mydata->cnow --;
		    continue;
		}
	    if (!strcasecmp(pl->ip, cldata[cl].itsip))
		{
		    DebugLog((ALOG_DNSBLC, 0,
			      "dnsbl_check_cache(%d): match (%u)",
			      cl, pl->state));
		    pl->expire = now + mydata->lifetime; /* dubious */
		    if (pl->state == DNSBL_FOUND)
			{
			    dnsbl_succeed(cl, "cached", "\177\0\0");
			    mydata->chito ++;
			}
		    else if (pl->state == OK)
			    mydata->chitn ++;
		    else
			    mydata->chitc ++;
		    return -1;
		}
	    last = &(pl->next);
	}
    mydata->cmiss ++;
    return 0;
}

/******************************** PUBLIC ************************************/

/*
 * dnsbl_init
 *
 *	This procedure is called when a particular module is loaded.
 *	Returns NULL if everything went fine,
 *	an error message otherwise.
 */
char *
dnsbl_init(self)
AnInstance *self;
{
	struct dnsbl_private *mydata;
	struct dnsbl_list *l;
	char tmpbuf[255], cbuf[32], *s;
	static char txtbuf[255];
    
	if (self->opt == NULL)
		return "Aie! no option(s): nothing to be done!";

	mydata = (struct dnsbl_private *) malloc(sizeof(struct dnsbl_private));
	bzero((char *) mydata, sizeof(struct dnsbl_private));
	self->data = mydata;
	mydata->cache = NULL;
	mydata->host_list = NULL;
	mydata->lifetime = CACHETIME;
    
	tmpbuf[0] = txtbuf[0] = '\0';
	if (strstr(self->opt, "log"))
	{
		mydata->options |= OPT_LOG;
		strcat(tmpbuf, ",log");
		strcat(txtbuf, ", Log");
	}
	if (strstr(self->opt, "reject"))
	{
		mydata->options |= OPT_DENY;
		strcat(tmpbuf, ",reject");
		strcat(txtbuf, ", Reject");
	}
	if (strstr(self->opt, "paranoid"))
	{
		mydata->options |= OPT_PARANOID;
		strcat(tmpbuf, ",paranoid");
		strcat(txtbuf, ", Paranoid");
	}

	if (mydata->options == 0)
	{
		DebugLog((ALOG_DNSBL, 0, "dnsbl_init: Aiee! no option(s)"));
		return "Aiee! no option(s)";
	}

	if ((s = strstr(self->opt, "list")))
	{
		char *ch = index(s, '=');
	    
		if (ch)
		{
		    char *x = ch, *t;

		    for (t = index(++ch, ','); t || ch == x + 1;
							t = index(ch, ','))
		    {
			for (; isspace(*ch); ch++) ;

			if (t)
			        *t = '\0';

			l = (struct dnsbl_list *)
					malloc(sizeof(struct dnsbl_list));

			if (*ch == '^') {
				l->is_whitelist = 1;
				ch++;
			} else
				l->is_whitelist = 0;

			l->host = strdup(ch);
			l->next = mydata->host_list;
			sendto_log(ALOG_DMISC, LOG_NOTICE,
					"dnsbl_init: Added %s as dns%cl", ch,
						l->is_whitelist ? "w" : "b");
			mydata->host_list = l;

			if (t)
			{
			        *t = ',';
				x  = t;	
				ch = ++t;
			}
			else
				x = ch;	/* need it to end the cycle */
		    }
		}
	}

	if (mydata->host_list == NULL)
	{
		DebugLog((ALOG_DNSBL, 0, "dnsbl_init: Aiee! No DNSBL host: "
						"nothing to be done!"));
		return "Aiee! No DNSBL host: nothing to be done!";
	}
    
	if (strstr(self->opt, "cache"))
	{
		char *ch = index(self->opt, '=');
	    
		if (ch)
			mydata->lifetime = atoi(++ch);
	}
	sprintf(cbuf, ",cache=%d", mydata->lifetime);
	strcat(tmpbuf, cbuf);
	sprintf(cbuf, ", Cache %d (min)", mydata->lifetime);
	strcat(txtbuf, cbuf);
	mydata->lifetime *= 60;
	strcat(tmpbuf, ",list=");
	strcat(txtbuf, ", List(s): ");
	l = mydata->host_list;
	strcat(tmpbuf, l->host);
	strcat(txtbuf, l->host);

	for (l = l->next; l; l = l->next)
	{
		strcat(tmpbuf, ",");
		strcat(txtbuf, ", ");
		strcat(tmpbuf, l->host);
		strcat(txtbuf, l->host);
	}
    
	self->popt = strdup(tmpbuf + 1);
	return txtbuf + 2;
}

/*
 * dnsbl_release
 *
 *	This procedure is called when a particular module is unloaded.
 */
void
dnsbl_release(self)
AnInstance *self;
{
    struct dnsbl_private *mydata = self->data;
    struct dnsbl_list *l, *n;

    for (l = mydata->host_list; l; l = n)
    {
	free(l->host);
	n = l->next;
	free(l);
    }

    free(mydata);
    free(self->popt);
}

/*
 * dnsbl_stats
 *
 *	This procedure is called regularly to update statistics sent to ircd.
 */
void
dnsbl_stats(self)
AnInstance *self;
{
    struct dnsbl_private *mydata = self->data;

    sendto_ircd("S dnsbl verified %u rejected %u",
				mydata->total, mydata->rejects);
}

/*
 * dnsbl_start
 *
 *	This procedure is called to start the host check procedure.
 *	Returns 0 if everything went fine,
 *	-1 otherwise (nothing to be done, or failure)
 *
 *	It is responsible for sending error messages where appropriate.
 *	In case of failure, it's responsible for cleaning up (e.g. dnsbl_clean
 *	will NOT be called)
 */
int
dnsbl_start(cl)
u_int cl;
{
    struct dnsbl_private *mydata = cldata[cl].instance->data;
    struct dnsbl_list *l, *m = NULL;
    char *req;
    char *ip, *c1, *c2, *c3;
    int  ipl;
    struct hostent *hst = NULL;
    
    if (cldata[cl].state & A_DENY)
	{
	    /* no point of doing anything */
	    DebugLog((ALOG_DNSBL, 0,
		      "dnsbl_start(%d): A_DENY alredy set ", cl));
	    return -1;
	}
   
    if (strchr(cldata[cl].itsip,':')) {
                DebugLog((ALOG_DNSBL, 0,
                                "dnsbl_start(%d): %s is IPv6, skipping ", cl, cldata[cl].itsip));
                return -1;
        }
 
    if (dnsbl_check_cache(cl))
	    return -1;
    
    ip = strdup(cldata[cl].itsip);
    ipl = strlen(ip);
    DebugLog((ALOG_DNSBL, 0, "dnsbl_start(%d): checking %s", cl, ip));

    strtok(ip, ".");
    c3 = strtok(NULL, ".");
    c2 = strtok(NULL, ".");
    c1 = strtok(NULL, ".");

    for (l = mydata->host_list; l && !hst; l = l->next)
    {
	int len;

	req = malloc(strlen(l->host) + ipl + 4);
	len = sprintf( req, "%s.%s.%s.%s.%s", c1, c2, c3, ip, l->host);
DebugLog((ALOG_DNSBL, 0,
                      "dnsbl_start(%d): gethostbyname() for %s",
                                                cl, req ));
	if (req[len] != '.') {	/* wrap around buggy DNS servers */
		req[len++]	= '.';
		req[len]	= '\0';
	}
	hst = gethostbyname(req);
	m = l;
	free(req);
    }
    mydata->total++;

    if (hst && m->is_whitelist == 0) /* do not cache whitelist hits 'cause */
	{				/* we won't suffer much from that */
	    unsigned int c = mydata->rejects;

	    dnsbl_succeed(cl, m->host, hst->h_addr_list[0]);
	    dnsbl_add_cache(cl, (c == mydata->rejects) ?
					DNSBL_FAILED : DNSBL_FOUND);
	    /* cache failure when not actually rejected, otherwise
	     * client will be shot at the 2nd try  -erra
	     */
	}
    else
	{
	    DebugLog((ALOG_DNSBL, 0,
		      "dnsbl_start(%d): gethostbyname() reported %s",
						cl, hstrerror(h_errno) ));
	    dnsbl_add_cache(cl, DNSBL_FAILED);
	}

    free(ip);
    return -1;
}

/*
 * dnsbl_work
 *
 *	This procedure is called whenever there's new data in the buffer.
 *	Returns 0 if everything went fine, and there is more work to be done,
 *	Returns -1 if the module has finished its work (and cleaned up).
 *
 *	It is responsible for sending error messages where appropriate.
 */
int
dnsbl_work(cl)
u_int cl;
{
	/*
	** There' nothing to do here
	*/
	DebugLog((ALOG_DNSBL, 0,
		      "dnsbl_work(%d) invoked but why?", cl ));
	return cl;
}

/*
 * dnsbl_clean
 *
 *	This procedure is called whenever the module should interrupt its work.
 *	It is responsible for cleaning up any allocated data, and in particular
 *	closing file descriptors.
 */
void
dnsbl_clean(u_int cl _A_UNUSED_)
{
    DebugLog((ALOG_DNSBL, 0, "dnsbl_clean(%d): cleaning up", cl));
    /*
    ** only one of rfd and wfd may be set at the same time,
    ** in any case, they would be the same fd, so only close() once
    */
}

/*
 * dnsbl_timeout
 *
 *	This procedure is called whenever the timeout set by the module is
 *	reached.
 *
 *	Returns 0 if things are okay, -1 if check was aborted.
 */
int
dnsbl_timeout(cl)
u_int cl;
{
    DebugLog((ALOG_DNSBL, 0, "dnsbl_timeout(%d): calling dnsbl_clean ", cl));
    dnsbl_clean(cl);
    return -1;
}

aModule Module_dnsbl =
	{ "dnsbl", dnsbl_init, dnsbl_release, dnsbl_stats,
	  dnsbl_start, dnsbl_work, dnsbl_timeout, dnsbl_clean };
