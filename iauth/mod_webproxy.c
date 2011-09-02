/************************************************************************
 *   IRC - Internet Relay Chat, iauth/mod_webproxy.c
 *   Copyright (C) 2001 Dan Merillat
 *   
 *   Original code (iauth/mod_socks.c) is
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
#define MOD_WEBGATE_C
#include "a_externs.h"

/****************************** PRIVATE *************************************/

#define CACHETIME 30

struct proxylog {
	struct proxylog *next;
	char ip[HOSTLEN + 1];
	u_char state;				/* 0 = no proxy, 1 = open proxy, 2 = closed proxy */
	time_t expire;
	u_int port;
};

#define OPT_LOG		0x001
#define OPT_DENY    	0x002
#if 0
#define OPT_PARANOID	0x004
#endif

#define PROXY_NONE		0
#define PROXY_OPEN		1
#define PROXY_CLOSE		2
#define PROXY_UNEXPECTED	3
#define PROXY_BADPROTO		4

struct webproxy_private {
	struct proxylog *cache;
	u_int lifetime;
	u_char options;
	u_int ports[128];
	u_int portcount;
	/* stats */
	u_int chitc, chito, chitn, cmiss, cnow, cmax;
	u_int open[128], closed, noproxy;
};
int webproxy_start(u_int);

/*
 * webproxy_open_proxy
 *
 *  Found an open proxy for cl: deal with it!
 */

static void 
webproxy_open_proxy(cl,port)
int cl;
u_int port;
{
	struct webproxy_private *mydata = cldata[cl].instance->data;

	/* open proxy */
	if (mydata->options & OPT_DENY)
	    {
		cldata[cl].state |= A_DENY;
		sendto_ircd("K %d %s %u :open webproxy at %d", cl,
				cldata[cl].itsip, cldata[cl].itsport, port);
	    }
	if (mydata->options & OPT_LOG)
		sendto_log(ALOG_FLOG, LOG_INFO,
				"webproxy: open proxy: %s[%s], port %u",
				cldata[cl].host, cldata[cl].itsip, port);
}

/*
 * webproxy_add_cache
 *
 *  Add an entry to the cache.
 */

static void 
webproxy_add_cache(cl, state)
int cl, state;
{
	struct webproxy_private *mydata = cldata[cl].instance->data;
	struct proxylog *next;


	if (state == PROXY_OPEN) {
		if (cldata[cl].mod_status < mydata->portcount)
			mydata->open[cldata[cl].mod_status] += 1;
			
	} else if (state == PROXY_NONE)
		mydata->noproxy += 1;
	else
		/* state == PROXY_CLOSE|PROXY_UNEXPECTED|PROXY_BADPROTO */ 
		mydata->closed += 1;

	if (mydata->lifetime == 0)
		return;

	mydata->cnow += 1;
	if (mydata->cnow > mydata->cmax)
		mydata->cmax = mydata->cnow;

	next = mydata->cache;
	mydata->cache = (struct proxylog *) malloc(sizeof(struct proxylog));
	mydata->cache->expire = time(NULL) + mydata->lifetime;
	strcpy(mydata->cache->ip, cldata[cl].itsip);
	mydata->cache->port = mydata->ports[cldata[cl].mod_status - 1];
	mydata->cache->state = state;
	mydata->cache->next = next;
	DebugLog(
			(ALOG_DWEBPROXYC, 0,
			 "webproxy_add_cache(%d): new cache %s, open=%d", cl,
			 mydata->cache->ip, state));
}

/*
 * webproxy_check_cache
 *
 *  Check cache for an entry.
 */
static int 
webproxy_check_cache(cl)
{
	struct webproxy_private *mydata = cldata[cl].instance->data;
	struct proxylog **last, *pl;
	time_t now = time(NULL);

	if (!mydata || mydata->lifetime == 0)
		return 0;

	DebugLog(
				(ALOG_DWEBPROXYC, 0,
				 "webproxy_check_cache(%d): Checking cache for %s", cl,
				 cldata[cl].itsip));

	last = &(mydata->cache);
	while ((pl = *last)) {
		DebugLog((ALOG_DWEBPROXYC, 0, "webproxy_check_cache(%d): cache %s",
				  cl, pl->ip));
		if (pl->expire < now) {
			DebugLog((ALOG_DWEBPROXYC, 0,
					  "webproxy_check_cache(%d): free %s (%d < %d)",
					  cl, pl->ip, pl->expire, now));
			*last = pl->next;
			free(pl);
			mydata->cnow -= 1;
			continue;
		}
		if (!strcasecmp(pl->ip, cldata[cl].itsip)) {
			DebugLog((ALOG_DWEBPROXYC, 0,
					  "webproxy_check_cache(%d): match (%u)", cl,
					  pl->state));
			pl->expire = now + mydata->lifetime;	/* dubious */
			if (pl->state == PROXY_OPEN) {
				webproxy_open_proxy(cl,pl->port);
				mydata->chito += 1;
			} else if (pl->state == PROXY_NONE)
				mydata->chitn += 1;
			else
				mydata->chitc += 1;
			return -1;
		}
		last = &(pl->next);
	}
	mydata->cmiss += 1;
	return 0;
}

static int 
webproxy_write(u_int cl)
{

	struct webproxy_private *mydata = cldata[cl].instance->data;
	static char query[64];	/* big enough to hold all queries */
	static int query_len;	/* lenght of socks4 query */
	static int wlen;



	query_len=snprintf(query, 64, "CONNECT %s:%d HTTP/1.0\n\n",
			 cldata[cl].ourip, cldata[cl].ourport);

	DebugLog((ALOG_DWEBPROXY, 0, "webproxy_write(%d): Checking %s:%u",
			  cl, cldata[cl].itsip, mydata->ports[cldata[cl].mod_status]));
	if ((wlen=write(cldata[cl].wfd, query, query_len)) != query_len) {
		/* most likely the connection failed */
		DebugLog(
					(ALOG_DWEBPROXY, 0,
				  "webproxy_write(%d): write() failed: %d %s", cl,
					 wlen, strerror(errno)));
		close(cldata[cl].wfd);
		cldata[cl].rfd = cldata[cl].wfd = 0;
		cldata[cl].buflen=0;
		if (++cldata[cl].mod_status < mydata->portcount)
			return webproxy_start(cl);
		else
			return 1;
	}
	cldata[cl].rfd = cldata[cl].wfd;
	cldata[cl].wfd = 0;
	return 0;
}

static int 
webproxy_read(u_int cl)
{
	/* Looking for "Connection established"
	 * HTTP/1.0 200 Connection established" */

	struct webproxy_private *mydata = cldata[cl].instance->data;
	int again = 1;
	u_char state = PROXY_CLOSE;
	char  * lookfor="HTTP/1.0 200";
	u_int looklen=strlen(lookfor);

	/* data's in from the other end */
	if (cldata[cl].buflen < looklen)
		return 0;
	
	/* zero it out so the debug log is sane */
	cldata[cl].inbuffer[cldata[cl].buflen]=0;

	/* got all we need */
	DebugLog(
				(ALOG_DWEBPROXY, 0, "webproxy_read(%d): %d Got [%s]",
			cl, mydata->ports[cldata[cl].mod_status], cldata[cl].inbuffer));


	if (!strncmp(cldata[cl].inbuffer, lookfor, looklen) ||
	    !strncmp(cldata[cl].inbuffer, "HTTP/1.1 200",looklen)
			)  { /* got it! */
		DebugLog((ALOG_DWEBPROXY, 0, "webproxy_read(%d): Open proxy on %s:%u",
				cl, cldata[cl].itsip, mydata->ports[cldata[cl].mod_status]));
		state = PROXY_OPEN;
		webproxy_open_proxy(cl,mydata->ports[cldata[cl].mod_status]);
		again = 0;
	}

	cldata[cl].mod_status++;
	close(cldata[cl].rfd);
	cldata[cl].rfd = 0;

	if (again && cldata[cl].mod_status < mydata->portcount) {
		cldata[cl].buflen = 0;
		return webproxy_start(cl);
	}
	else {
		webproxy_add_cache(cl, state);
		return -1;
	}
	return 0;
}

/******************************** PUBLIC ************************************/

/*
 * webproxy_init
 *
 *  This procedure is called when a particular module is loaded.
 *  Returns NULL if everything went fine,
 *  an error message otherwise.
 */

char *
webproxy_init(AnInstance * self)
{
	struct webproxy_private *mydata;
	char tmpbuf[80], cbuf[32];
	static char txtbuf[80];
	u_int portcount = 0;
	
	tmpbuf[0] = txtbuf[0] = '\0';

	mydata = 
		(struct webproxy_private *)
		malloc(sizeof(struct webproxy_private));
	bzero((char *) mydata, sizeof(struct webproxy_private));
	mydata->cache = NULL;
	mydata->lifetime = CACHETIME;
	if (!self->opt)
	{
		/* using default config:
		** log,deny,ports=3128;8080 
		*/
		mydata->options=OPT_DENY|OPT_LOG;
		mydata->ports[portcount++] = 3128;
		mydata->ports[portcount++] = 8080;
		mydata->portcount=portcount;
		strcat(tmpbuf, ",log");
		strcat(txtbuf, ", Log");
		strcat(tmpbuf, ",reject");
		strcat(txtbuf, ", Reject");
		strcat(tmpbuf, ",ports=3128;8080");
		strcat(txtbuf, ", Ports=3128;8080");

	}
	else
	{

		char *ch = NULL, *portsp = NULL;
	
		if ((portsp=strstr(self->opt, "ports"))) 

		{
			char xbuf[128];
			char *c;
			size_t slen;
			bzero(xbuf,sizeof(xbuf));
			ch = strchr(portsp, '=');
			if (!ch)
			{
				mydata->ports[portcount++] = 3128;
				mydata->ports[portcount++] = 8080;
				mydata->portcount=portcount;
			}
			else
			{
				c=index(ch, ',');
				if (!c)
				   c=ch+strlen(ch);
			/* now ch is at = and c is at the end of the ports line */
				ch++;
				slen = (size_t) c - (size_t) ch;
				if (slen > 30)
				{
					slen = 30;
				}
				memcpy(xbuf, ch, slen);
				strcat(tmpbuf,",ports=");

				strcat(tmpbuf,xbuf);
				ch=xbuf;
				while((c=index(ch, ';'))) {
				   *c=0;
				   mydata->ports[portcount++]=atoi(ch);
				   ch=c+1;
				}
				mydata->ports[portcount++]=atoi(ch);
				mydata->portcount=portcount;
			}
		}
		if (strstr(self->opt, "log")) {
			mydata->options |= OPT_LOG;
			strcat(tmpbuf, ",log");
			strcat(txtbuf, ", Log");
		}
		if (strstr(self->opt, "reject")) {
			mydata->options |= OPT_DENY;
			strcat(tmpbuf, ",reject");
			strcat(txtbuf, ", Reject");
		}
		if (strstr(self->opt, "cache")) {
			char *ch = index(self->opt, '=');

			if (ch)
				mydata->lifetime = atoi(ch + 1);
		}
	}	
	sprintf(cbuf, ",cache=%d", mydata->lifetime);
	strcat(tmpbuf, cbuf);
	sprintf(cbuf, ", Cache %d (min)", mydata->lifetime);
	strcat(txtbuf, cbuf);
	mydata->lifetime *= 60;
	self->popt = mystrdup(tmpbuf + 1);
	self->data = mydata;
	return txtbuf + 2;
}

/*
 * webproxy_release
 *
 *  This procedure is called when a particular module is unloaded.
 */
void 
webproxy_release(self)
AnInstance *self;
{
	struct webproxy_private *mydata = self->data;
	free(mydata);
	free(self->popt);
}

/*
 * webproxy_stats
 *
 *  This procedure is called regularly to update statistics sent to ircd.
 */
void 
webproxy_stats(self)
AnInstance *self;
{
	struct webproxy_private *mydata = self->data;
	char mybuf[256];
	int len;
	int i;

	len=snprintf(mybuf, 256, "S webproxy port:open");
	for (i=0;i<mydata->portcount;i++)
		len+=snprintf(mybuf+len, 256-len, " %u:%u", 
				mydata->ports[i], mydata->open[i]);

	len+=snprintf(mybuf+len, 256-len, " closed %u noproxy %u",
			mydata->closed, mydata->noproxy);

	sendto_ircd(mybuf);

	sendto_ircd
		("S webproxy cache open %u closed %u noproxy %u miss %u (%u <= %u)",
		 mydata->chito, mydata->chitc, mydata->chitn, mydata->cmiss,
		 mydata->cnow, mydata->cmax);
}

/*
 * webproxy_start
 *
 *  This procedure is called to start the socks check procedure.
 *  Returns 0 if everything went fine,
 *  -1 otherwise (nothing to be done, or failure)
 *
 *  It is responsible for sending error messages where appropriate.
 *  In case of failure, it's responsible for cleaning up (e.g. webproxy_clean
 *  will NOT be called)
 */
int 
webproxy_start(cl)
u_int cl;
{

	struct webproxy_private *mydata = cldata[cl].instance->data;
	char *error;
	int fd;

	if (cldata[cl].state & A_DENY) {
		/* no point of doing anything */
		DebugLog((ALOG_DWEBPROXY, 0,
				  "webproxy_start(%d): A_DENY alredy set ", cl));
		return -1;
	}
	if (cldata[cl].mod_status == 0)
		if (webproxy_check_cache(cl))
			return -1;

	if (strchr(cldata[cl].itsip,':'))
		return -1;

	while (cldata[cl].mod_status < mydata->portcount) {
		DebugLog(
				  (ALOG_DWEBPROXY, 0, "webproxy_start(%d): Connecting to %s:%u",
				   cl, cldata[cl].itsip, mydata->ports[cldata[cl].mod_status]));
		fd = tcp_connect(cldata[cl].ourip, cldata[cl].itsip, 
					mydata->ports[cldata[cl].mod_status],
					&error);
		if (fd > 0) {
						/*so that webproxy_work() is called when connected */
			cldata[cl].wfd = fd;		
			return 0;
		}
		DebugLog((ALOG_DWEBPROXY, 0,
			  "webproxy_start(%d): tcp_connect() reported %s", cl,
			  error));
		cldata[cl].mod_status++;
		continue;
		
	}

	return -1;
}

/*
 * webproxy_work
 *
 *  This procedure is called whenever there's new data in the buffer.
 *  Returns 0 if everything went fine, and there is more work to be done,
 *  Returns -1 if the module has finished its work (and cleaned up).
 *
 *  It is responsible for sending error messages where appropriate.
 */
int 
webproxy_work(cl)
u_int cl;
{
	struct webproxy_private *mydata = cldata[cl].instance->data;

	if (!mydata)
		return -1;

	DebugLog(
			  (ALOG_DWEBPROXY, 0, "webproxy_work(%d): %d %d %d buflen=%d",
			   cl, mydata->ports[cldata[cl].mod_status],
				cldata[cl].rfd, cldata[cl].wfd,
			   cldata[cl].buflen));

	if (cldata[cl].wfd > 0)
		/*
		   ** We haven't sent the query yet, the connection was just
		   ** established.
		 */
		return webproxy_write(cl);
	else
		return webproxy_read(cl);
}

/*
 * webproxy_clean
 *
 *  This procedure is called whenever the module should interrupt its work.
 *  It is responsible for cleaning up any allocated data, and in particular
 *  closing file descriptors.
 */
void 
webproxy_clean(cl)
u_int cl;
{
	DebugLog((ALOG_DWEBPROXY, 0, "webproxy_clean(%d): cleaning up", cl));
	/*
	   ** only one of rfd and wfd may be set at the same time,
	   ** in any case, they would be the same fd, so only close() once
	 */
	if (cldata[cl].rfd)
		close(cldata[cl].rfd);
	else if (cldata[cl].wfd)
		close(cldata[cl].wfd);
	cldata[cl].rfd = cldata[cl].wfd = 0;
}

/*
 * webproxy_timeout
 *
 *  This procedure is called whenever the timeout set by the module is
 *  reached.
 *
 *  Returns 0 if things are okay, -1 if check was aborted.
 */
int 
webproxy_timeout(cl)
u_int cl;
{
	DebugLog(
				(ALOG_DWEBPROXY, 0,
				 "webproxy_timeout(%d): calling webproxy_clean ", cl));
	webproxy_clean(cl);
	return -1;
}

aModule Module_webproxy =
{"webproxy", webproxy_init, webproxy_release, webproxy_stats,
 webproxy_start, webproxy_work, webproxy_timeout, webproxy_clean
};
