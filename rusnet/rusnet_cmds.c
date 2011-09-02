/* 
** Copyright (c) 1998-2008 [RusNet IRC Network]
**
** File     : rusnet_cmds.c
** Desc.    : rusnet specific commands
**
** This program is distributed under the GNU General Public License in the 
** hope that it will be useful, but without any warranty. Without even the 
** implied warranty of merchantability or fitness for a particular purpose. 
** See the GNU General Public License for details.
**/

#ifndef RUSNET_CMDS_C
#define RUSNET_CMDS_C
 
#include "os.h"
#include "s_defines.h"
#include "match_ext.h"
#include "s_externs.h"

#include "rusnet_cmds_ext.h"

static void rusnet_changecodepage(struct Client *cptr, char *pageid, char *id)
{
#ifdef USE_OLD8BIT
    FILE *fp;
    struct Codepage *work = rusnet_getptrbyname(pageid);
   
    if (work != NULL) 
    {
        cptr->transptr = work;
        sendto_one(cptr, rpl_str(RPL_CODEPAGE, id), pageid);
    }
#else
    conversion_t *conv = conv_get_conversion(pageid);
   
    if (conv != NULL) 
    {
	if (cptr->conv != conv) /*send nickchange to client */
	{
	    unsigned char nick1[UNINICKLEN+1];
	    unsigned char nick2[UNINICKLEN+1];
	    unsigned char *oldnick = nick1, *newnick = nick2;
	    size_t s;

	    if (cptr->conv)
	    {
		s = conv_do_out(cptr->conv, cptr->name, strlen(cptr->name),
				&oldnick, UNINICKLEN);
		oldnick[s] = '\0'; /* nick in old charset */
	    }
	    s = conv_do_out(conv, cptr->name, strlen(cptr->name),
			    &newnick, UNINICKLEN);
	    newnick[s] = '\0'; /* nick in new charset */
	    conv_free_conversion(cptr->conv);
	    cptr->conv = NULL;
	    if (strcmp(oldnick, newnick)) /* check if it was not changed */
		sendto_one(cptr, ":%s NICK %s", oldnick, newnick);
	}
	else
	    conv_free_conversion(cptr->conv);
	cptr->conv = conv;
	sendto_one(cptr, rpl_str(RPL_CODEPAGE, id), pageid);
    }
#endif
    else 
        sendto_one(cptr, err_str(ERR_NOCODEPAGE, id), pageid);
}

int m_codepage(cptr, sptr, parc, parv)
aClient *cptr;
aClient *sptr;
int     parc;
char    *parv[];
{
#ifdef USE_OLD8BIT
    int  i;
#endif

    if (parc < 2) return -1;

#ifdef USE_OLD8BIT
    for (i = 0; parv[1][i] != '\0'; i++) parv[1][i] = toupper(parv[1][i]); 
#else
    /* it is save to overwrite the string because replacement is shorter */
    if (!strcasecmp(parv[1], "translit"))
	strcpy(parv[1], "ascii");
#endif

    rusnet_changecodepage(sptr, parv[1], parv[0]);

    return 0;    
}

int m_svsnick(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int     parc;
char    *parv[];
{
    aClient *acptr;

    if (parc < 4)
	return -1;

    /* Check if parv[2] is a registered nickname */
    if (find_client(parv[2], NULL) != NULL)
	return -1; /* Could not force nickchange, new nick is registered */

    /* Check if parv[1] exists */
    if ((acptr = find_client(parv[1], NULL)) == NULL)
	return -1; /* No such user */

    if (MyConnect(acptr))
    {  
	char    *pparv[] = { parv[1], parv[2], "1", NULL };

	/* Do nick change procedure */
	return m_nick(acptr, acptr, 3, pparv);
    }

	/* Propagate FORCE to single server where $nick is registered */
	acptr = acptr->from;
	if (acptr != cptr)	/* Split horizon (prevent loops) */
	{
		if (acptr->serv->version & SV_RUSNET2)
			sendto_one(acptr, "SVSNICK %s %s :%ld",
						parv[1], parv[2], parv[3]);
		else
			sendto_one(acptr, "FORCE %s %s", parv[1], parv[2]);
	}

	return 0;
}

int m_force(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int     parc;
char    *parv[];
{
	return m_svsnick(cptr, sptr, parc, parv);
}

/*
 * parv[0] - sender
 * parv[1] - username to change mode for
 * parv[2] - modes to change
 */
int m_svsmode(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int     parc;
char    *parv[];
{
    aClient *acptr;
    char    *s;

    if (parc < 3)
	return -1;

    /* prohibit remote O-line */
    if ( *parv[2] == '+' ) 
	for (s = (char *) parv[2] + 1; *s != '\0'; s++) 
		if (*s == 'o' || *s == 'O') 
			return -1 ;

    /* Check if parv[1] exists and local */
    if ((acptr = find_client(parv[1], NULL)) == NULL)
	return -1; /* No such user */

    if (MyConnect(acptr))
    {
	char    *pparv[] = { parv[1], parv[1], parv[2], NULL };
	/* Do change mode procedure */
	return m_umode(&me, acptr, 3, pparv); /* internal fake call */
    }

	acptr = acptr->from;

	if (acptr != cptr)	/* Split horizon (prevent loops) */
	{
		if (acptr->serv->version & SV_RUSNET2)
			sendto_one(acptr, "SVSMODE %s :%s", parv[1], parv[2]);
		else
		{
			/* do not pass +I/+R to older servers */
			for (s = (char *) parv[2] + 1; *s != '\0'; s++) 
				if (*s == 'I' || *s == 'R') 
					return 0;

			sendto_one(acptr,"RMODE %s %s", parv[1], parv[2]);
		}
	}

	return 0;
}

int m_rmode(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int     parc;
char    *parv[];
{
	return m_svsmode(cptr, sptr, parc, parv);
}

int m_rcpage(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int     parc;
char    *parv[];
{
    aClient *acptr, *workptr, *zptr;

    if (parc < 3)
	return -1;

    /* Check if parv[1] exists and local */

    workptr = NULL;

    if ((zptr = find_client(parv[1], workptr)) == NULL)
       return -1; /* No such user */

    if (MyConnect(zptr))
    { 
#ifdef USE_OLD8BIT
	int i;

#endif
        /* Do change codepage procedure */
#ifdef USE_OLD8BIT
	for (i = 0; parv[2][i] != '\0'; i++) parv[2][i] = toupper(parv[2][i]); 
#endif
        rusnet_changecodepage(zptr, parv[2], parv[1]);
    }
    else
    {
	acptr = zptr->from;
	if (acptr != cptr)	/* Split horizon */
		sendto_one(acptr,"RCPAGE %s %s", parv[1], parv[2]);
    }

    return 0;
}

/*
 * Internal method for usage with *serv commands e.g. /nickserv
 */

int m_services(cptr, sptr, nick, parc, parv)
aClient *cptr, *sptr;
int     parc;
char    *nick, *parv[];
{
    aClient *acptr;

    if (parc < 2 || *parv[1] == '\0')
    {
	sendto_one(sptr, err_str(ERR_NOTEXTTOSEND, parv[0]));
	return 1;
    }

    /* Check if nick is a registered nickname and user@host matches */
    if ((acptr = find_person(nick, NULL)) &&
	!strcasecmp(SERVICES_IDENT, acptr->user->username) &&
	!strcasecmp(SERVICES_HOST, acptr->user->host))
    {
	sendto_prefix_one(acptr, sptr, ":%s PRIVMSG %s@%s :%s",
				parv[0], nick, acptr->user->server, parv[1]);
	return 0;
    }

    sendto_one(sptr, err_str(ERR_NOSUCHSERVICE, nick),
			"Not found:", nick, "No Message Delivered");
    return 1;
}

int m_nickserv(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int     parc;
char    *parv[];
{
	return m_services(cptr, sptr, NICKSERV_NICK, parc, parv);
}

int m_chanserv(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int     parc;
char    *parv[];
{
	return m_services(cptr, sptr, CHANSERV_NICK, parc, parv);
}

int m_memoserv(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int     parc;
char    *parv[];
{
	return m_services(cptr, sptr, MEMOSERV_NICK, parc, parv);
}

int m_operserv(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int     parc;
char    *parv[];
{
	return m_services(cptr, sptr, OPERSERV_NICK, parc, parv);
}

#endif
