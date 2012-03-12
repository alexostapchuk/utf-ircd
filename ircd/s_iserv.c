/************************************************************************
 *   IRC - Internet Relay Chat, ircd/s_iserv.c
 *   Copyright (C) 2005 skold
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
#define S_ISERV_C
#include "s_externs.h"
#undef S_ISERV_C


#if defined(USE_ISERV)

u_char		iserv_options = 0;
u_int		iserv_spawn = 0;
char		*iserv_version = NULL;

/*
 * sendto_iserv
 *
 *	Send the buffer to the iserv slave daemon.
 *	Return 0 if everything went well, -1 otherwise.
 */
int	vsendto_iserv(char *pattern, va_list va)
{
	static	char	ibuf[BUFSIZ], *p;
	int	i, len;

	if (isfd < 0)
		return -1;

	vsprintf(ibuf, pattern, va);
	strcat(ibuf, "\n");

	p = ibuf;
	len = strlen(p);

	do
	{
		i = write(isfd, ibuf, len);

		if (i == -1)
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK)
			{
				sendto_flag(SCH_ISERV, "Iserv daemon is dead. Respawn...");
				close(isfd);
				isfd = -1;
				start_iserv();
				return -1;
			}
			i = 0;
		}
		p += i;
		len -= i;
	}
	while (len > 0);

	return 0;
}

int	sendto_iserv(char *pattern, ...)
{
	int i;

        va_list va;
        va_start(va, pattern);
        i = vsendto_iserv(pattern, va);
        va_end(va);
	return i;
}

#endif
