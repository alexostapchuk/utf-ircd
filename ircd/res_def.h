/*
 *   ircd/res_def.h (C)opyright 1992 Darren Reed.
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

#define	RES_INITLIST	1
#define	RES_CALLINIT	2
#define RES_INITSOCK	4
#define RES_INITDEBG	8
#define RES_INITCACH    16

#define MAXPACKET	1024
#define MAXALIASES	35
#define MAXADDRS	35

#define	AR_TTL		600	/* TTL in seconds for dns cache entries */

struct	hent {
	char	*h_name;	/* official name of host */
	char	*h_aliases[MAXALIASES];	/* alias list */
	int	h_addrtype;	/* host address type */
	int	h_length;	/* length of address */
	/* list of addresses from name server */
	struct	IN_ADDR	h_addr_list[MAXADDRS];
#define	h_addr	h_addr_list[0]	/* address, for backward compatiblity */
};

typedef	struct	reslist {
	int	id;
	int	sent;	/* number of requests sent */
	int	srch;
	time_t	ttl;
	char	type;
	char	retries; /* retry counter */
	char	sends;	/* number of sends (>1 means resent) */
	char	resend;	/* send flag. 0 == dont resend */
	time_t	sentat;
	time_t	timeout;
	struct	IN_ADDR	addr;
	char	*name;
	struct	reslist	*next;
	Link	cinfo;
	struct	hent he;
	} ResRQ;

typedef	struct	cache {
	time_t	expireat;
	time_t	ttl;
	struct	hostent	he;
	struct	cache	*hname_next, *hnum_next, *list_next;
	} aCache;

typedef struct	cachetable {
	aCache	*num_list;
	aCache	*name_list;
	} CacheTable;

/* must be a prime */
#define ARES_CACSIZE	1009
/* should be around twice smaller */
#define	MAXCACHED	512
