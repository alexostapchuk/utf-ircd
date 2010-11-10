/* 
** Copyright (c) 1998 [RusNet IRC Network]
**
** File     : rusnet_misc.c
** Desc.    : rusnet specific commands
**
** This program is distributed under the GNU General Public License in the 
** hope that it will be useful, but without any warranty. Without even the 
** implied warranty of merchantability or fitness for a particular purpose. 
** See the GNU General Public License for details.
** $Id: rusnet_misc.c,v 1.10 2010-11-10 13:38:39 gvs Exp $
 */

#include "os.h"
#include "s_defines.h"

#ifdef RUSNET_IRCD

#ifdef USE_OLD8BIT
int rusnet_getclientport(int fd)
{
    struct	sockaddr_in myaddr;
    u_int32_t	myaddr_len = sizeof(struct sockaddr);

    if (getsockname(fd, (struct sockaddr *) &myaddr, &myaddr_len) == 0)
        return ntohs(myaddr.sin_port);
    else
        return 0;
}

void rusnet_changecodepage(struct Client *cptr, char *pageid, char *id)
{
    FILE *fp;
    struct Codepage *work = rusnet_getptrbyname(pageid);
   
    if (work != NULL) 
    {
        cptr->transptr = work;
        sendto_one(cptr, rpl_str(RPL_CODEPAGE, id), pageid);
    }
    else 
        sendto_one(cptr, err_str(ERR_NOCODEPAGE, id), pageid);
}
#endif

aChannel *rusnet_isagoodnickname(struct Client *cptr, char *nickname)
{
	Reg Link *lp;
	Reg aChannel *chptr;
	unsigned char flag_8bit;
#ifdef USE_OLD8BIT
	int i;

	lp = cptr->user->channel;
#else
	register unsigned char *ch;
#endif

	/*
	 ** Now let's figure out if the nickname has 8bit up chars
	 ** nickname is always null-terminated string after do_nick_name
	 ** in m_nick() ! -LoSt
	 */
	
	flag_8bit = 0;
#ifdef USE_OLD8BIT
	for (i=0; (i<NICKLEN) && (nickname[i]!=0); i++)
		if ((unsigned char)(nickname[i])&0x80)
#else
	for (ch = (unsigned char *)nickname; *ch; ch++)
		if (*ch & 0x80)
#endif
		{
			flag_8bit = 1;
			break;
		}

	if (flag_8bit == 0) 
		return NULL;

#ifndef USE_OLD8BIT
	lp = cptr->user->channel;
#endif
	while ( lp != NULL )
	{
		chptr = lp->value.chptr;
		if ((chptr->mode.mode & MODE_7BIT) != 0)
		{
			return chptr;
		}
		lp = lp->next;
	}

	return NULL;
}
#endif
