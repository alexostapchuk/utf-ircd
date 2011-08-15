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
 */

#ifndef RUSNET_MISC_C
#define RUSNET_MISC_C

#include "os.h"
#include "s_defines.h"

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
#endif

aChannel *rusnet_zmodecheck(struct Client *cptr, char *nickname)
{
	Reg Link *lp;
	Reg unsigned char *ch;

	/*
	 ** Now let's figure out if the nickname has 8bit up chars
	 ** nickname is always null-terminated string after do_nick_name
	 ** in m_nick() ! -LoSt
	 */
	
	for (ch = (unsigned char *)nickname; *ch; ch++)
		if (*ch & 0x80)
			break;

	if (!*ch)		/* end-of-line reached */
		return NULL;

	/* Now check whether there are channels in 7bit mode */
	for (lp = cptr->user->channel; lp; lp = lp->next)
	{
		Reg aChannel *chptr = lp->value.chptr;

		if ((chptr->mode.mode & MODE_7BIT) != 0)
			return chptr;
	}

	return NULL;
}
#endif
