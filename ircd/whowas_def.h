/************************************************************************
 *   IRC - Internet Relay Chat, ircd/whowas_def.h
 *   Copyright (C) 1990  Markku Savela
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

typedef struct aname {
	anUser	*ww_user;
	aClient	*ww_online;
	time_t	ww_logout;
	char	ww_nick[UNINICKLEN + 1];
#ifndef USE_OLD8BIT
	char	ww_info[MB_LEN_MAX*REALLEN + 1];
#else
	char	ww_info[REALLEN + 1];
#endif
#ifndef USE_OLD8BIT
	char	ww_ucnick[UNINICKLEN+1]; /* for comparisons */
#endif
	char	ww_host[HOSTLEN + 1];
} aName;

typedef struct alock {
	time_t	logout;
	char	nick[UNINICKLEN + 1];
	int	released;
	char	server[HOSTLEN + 1];
} aLock;
