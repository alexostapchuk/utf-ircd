/************************************************************************
 *   IRC - Internet Relay Chat, common/struct_def.h
 *   Copyright (C) 1990 Jarkko Oikarinen and
 *                      University of Oulu, Computing Center
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
 *   $Id: struct_def.h,v 1.26 2010-11-10 13:38:39 gvs Exp $
 */


typedef	struct	ConfItem aConfItem;
typedef	struct 	Client	aClient;
typedef	struct	Channel	aChannel;
typedef	struct	User	anUser;
typedef	struct	Server	aServer;
typedef	struct	Service	aService;
typedef	struct	SLink	Link;
typedef	struct	SMode	Mode;
typedef	struct	fdarray	FdAry;
typedef	struct	CPing	aCPing;
typedef	struct	Zdata	aZdata;
typedef struct        LineItem aMotd;
#if defined(USE_IAUTH)
typedef struct        LineItem aExtCf;
typedef struct        LineItem aExtData;
#endif

#ifdef LOG_EVENTS
typedef struct        Logger  aLogger;
typedef struct	      LogFile aLogFile;
#ifdef USE_SYSLOG
typedef struct        Syslog  aSyslog;
#endif
#endif

#define	HOSTLEN		63	/* Length of hostname.  Updated to         */
				/* comply with RFC1123                     */

#ifdef RUSNET_IRCD
#define	NICKLEN		31	/* Necessary to put 31 here instead of 32
				** This preserves compatibility with services
				** --erra
				*/
#ifdef USE_OLD8BIT
#define	UNINICKLEN	NICKLEN
#else
#define	UNINICKLEN	63	/* utf-8 char has 1 to 6 bytes --LoSt
				** Cannot be more than HOSTLEN due to aClient
				*/
#endif
#define	TOPICLEN	255
#define	MAXBANS		60
#else
#define	NICKLEN		9	/* Necessary to put 9 here instead of 10
				** if s_msg.c/m_nick has been corrected.
				** This preserves compatibility with old
				** servers --msa
				*/
#define	UNINICKLEN	NICKLEN
#define	TOPICLEN	80
#define	MAXBANS		30	
#endif
#define	USERLEN		10
#define	REALLEN	 	50
#define	CHANNELLEN	50
#define	PASSWDLEN 	20
#define	KEYLEN		23
#define	BUFSIZE		512		/* WARNING: *DONT* CHANGE THIS!!!! */
#define	MAXRECIPIENTS 	20
#define	MAXBANLENGTH	6144
#define	BANLEN		(USERLEN + UNINICKLEN + HOSTLEN + 3)
#define MAXPENALTY	10
#define	CHIDLEN		5		/* WARNING: *DONT* CHANGE THIS!!!! */

#define	READBUF_SIZE	16384	/* used in s_bsd.c *AND* s_zip.c ! */

#define	MAXMODEPARAMS	3	/* do _!not!_ change this */
 
/*
 * Make up some numbers which should reflect average leaf server connect
 * queue max size.
 * queue=(<# of channels> * <channel size> + <user size> * <# of users>) * 2
 * pool=<queue per client> * <avg. # of clients having data in queue>
 */
#define	QUEUELEN	(((MAXCONNECTIONS / 10) * (CHANNELLEN + BANLEN + 16) +\
			  (HOSTLEN * 4 + REALLEN + NICKLEN + USERLEN + 24) *\
			  (MAXCONNECTIONS / 2)) * 2)

#define	BUFFERPOOL	(DBUFSIZ * MAXCONNECTIONS * 2) + \
			(QUEUELEN * MAXSERVERS)

#define	USERHOST_REPLYLEN	(UNINICKLEN+HOSTLEN+USERLEN+5)


/*
** flags for bootup options (command line flags)
*/
#define	BOOT_QUICK	0x001
#define	BOOT_DEBUG	0x002
#define	BOOT_INETD	0x004
#define	BOOT_TTY	0x008
#define	BOOT_AUTODIE	0x010
#define	BOOT_BADTUNE	0x020
#define	BOOT_PROT	0x040
#define	BOOT_STRICTPROT	0x080
#define	BOOT_NOIAUTH	0x100
#define BOOT_NOISERV	0x200

#define	STAT_RECONNECT	-7	/* Reconnect attempt for server connections */
#define	STAT_MASTER	-5	/* Local ircd master before identification */
#define	STAT_CONNECTING	-4
#define	STAT_HANDSHAKE	-3
#define	STAT_UNKNOWN	-2
#define	STAT_ME		-1
#define	STAT_SERVER	0
#define	STAT_CLIENT	1
#define	STAT_SERVICE	2

/*
 * status macros.
 */
#define	IsRegisteredUser(x)	((x)->status == STAT_CLIENT && (x)->user)
#define	IsRegistered(x)		((x)->status >= STAT_SERVER || \
				 (x)->status == STAT_ME)
#define	IsConnecting(x)		((x)->status == STAT_CONNECTING)
#define	IsHandshake(x)		((x)->status == STAT_HANDSHAKE)
#define	IsMe(x)			((x)->status == STAT_ME)
#define	IsUnknown(x)		((x)->status == STAT_UNKNOWN || \
				 (x)->status == STAT_MASTER)
#define	IsServer(x)		((x)->status == STAT_SERVER)
#define	IsClient(x)		((x)->status == STAT_CLIENT)
#define	IsService(x)		((x)->status == STAT_SERVICE && (x)->service)
#define	IsReconnect(x)		((x)->status == STAT_RECONNECT)

#define	SetMaster(x)		((x)->status = STAT_MASTER)
#define	SetConnecting(x)	((x)->status = STAT_CONNECTING)
#define	SetHandshake(x)		((x)->status = STAT_HANDSHAKE)
#define	SetMe(x)		((x)->status = STAT_ME)
#define	SetUnknown(x)		((x)->status = STAT_UNKNOWN)
#define	SetServer(x)		((x)->status = STAT_SERVER)
#define	SetClient(x)		((x)->status = STAT_CLIENT)
#define	SetService(x)		((x)->status = STAT_SERVICE)

#define	FLAGS_PINGSENT	  0x00000001	/* Unreplied ping sent */
#define	FLAGS_DEADSOCKET  0x00000002	/* Local socket is dead--Exiting soon */
#define	FLAGS_KILLED      0x00000004	/* Prevents "QUIT" from being sent for this */
#define	FLAGS_BLOCKED     0x00000008	/* socket is in a blocked condition [unused] */
#ifdef UNIXPORT
#define	FLAGS_UNIX	  0x00000010	/* socket is in the unix domain, not inet */
#endif
#define	FLAGS_CLOSING     0x00000020	/* set when closing to suppress errors */
#define	FLAGS_LISTEN      0x00000040 /* used to mark clients which we listen() on */
#define	FLAGS_XAUTHDONE   0x00000080 /* iauth is finished with this client */
#define	FLAGS_DOINGDNS    0x00000100 /* client is waiting for a DNS response */
#define	FLAGS_AUTH	  0x00000200 /* client is waiting on rfc931 response */
#define	FLAGS_WRAUTH	  0x00000400	/* set if we havent writen to ident server */
#define	FLAGS_LOCAL	  0x00000800 /* set for local clients */
#define	FLAGS_GOTID	  0x00001000	/* successful ident lookup achieved */
#define	FLAGS_XAUTH	  0x00002000	/* waiting on external authentication */
#define	FLAGS_WXAUTH	  0x00004000	/* same as above, but also prevent parsing */
#define	FLAGS_NONL	  0x00008000 /* No \n in buffer */
#define	FLAGS_CBURST	  0x00010000	/* set to mark connection burst being sent */
#define FLAGS_RILINE      0x00020000 /* Restricted i-line [unused?] */
#define FLAGS_QUIT        0x00040000 /* QUIT :comment shows it's not a split */
#define FLAGS_SPLIT       0x00080000 /* client QUITting because of a netsplit */
#define FLAGS_HIDDEN      0x00100000 /* netsplit is behind a hostmask */
#define	FLAGS_UNKCMD      0x00200000	/* has sent an unknown command */
#define	FLAGS_ZIP         0x00400000 /* link is zipped */
#define	FLAGS_ZIPRQ       0x00800000 /* zip requested */
#define	FLAGS_ZIPSTART    0x01000000 /* start of zip (ignore any CRLF) */
#ifdef RUSNET_IRCD
#define FLAGS_COLLMAP     0x02000000 /* nick has collided and mapped */
#define FLAGS_XMODE       0x04000000 /* user requested usermode +x at connect */
#endif
#ifdef USE_SSL
#define FLAGS_SSL         0x10000000 /* SSL-connection requested */
#endif /* USE_SSL */
#ifdef RUSNET_RLINES
#define FLAGS_RLINE       0x20000000 /* User has got active R-line */
#endif
#ifndef USE_OLD8BIT
#define FLAGS_UNICODE     0x40000000 /* it's UTF-8 connection */
#endif

#define	FLAGS_OPER        0x0001	/* Operator */
#define	FLAGS_LOCOP       0x0002 /* Local operator -- SRB */
#define	FLAGS_WALLOP      0x0004 /* send wallops to them */
#define	FLAGS_INVISIBLE   0x0008 /* makes user invisible */
#define FLAGS_RESTRICTED  0x0010 /* Restricted user */
#define FLAGS_AWAY        0x0020 /* user is away */
#ifdef RUSNET_IRCD
#define FLAGS_VHOST       0x0080 /* user @ masked host */
#endif
#ifdef USE_SSL
#define FLAGS_SMODE	  0x0100 /* User is using SSL connection*/
#endif
#ifdef RUSNET_RLINES
#define FLAGS_RMODE	  0x0200 /* User mode +b (Rusnet restriction)*/
#endif

/* Rehash flags */
#define REHASH_ALL	0x0001	/* Rehash everything */
#define REHASH_DNS	0x0002	/* Flush DNS cache, re-read resolv.conf */
#ifdef USE_SSL
# define REHASH_SSL	0x0004	/* Rehash SSL structures and p-lines */
#endif
#define REHASH_I	0x0008
#define REHASH_Y	0x0010
#define REHASH_P	0x0020
#define REHASH_K	0x0040
#define REHASH_R	0x0080
#define REHASH_MOTD	0x0100
#define REHASH_O	0x0200
#define REHASH_C	0x0400
#define REHASH_B	0x0800
#define REHASH_E	0x1000

#ifdef RUSNET_IRCD
# ifdef USE_SSL
# define SEND_UMODES	(FLAGS_INVISIBLE|FLAGS_OPER|FLAGS_WALLOP|FLAGS_AWAY|FLAGS_VHOST|FLAGS_RMODE|FLAGS_SMODE)
# else /* USE_SSL */
# define SEND_UMODES	(FLAGS_INVISIBLE|FLAGS_OPER|FLAGS_WALLOP|FLAGS_AWAY|FLAGS_VHOST|FLAGS_RMODE)
# endif /* USE_SSL */
#else /* RUSNET_IRCD */
# ifdef USE_SSL
# define SEND_UMODES	(FLAGS_INVISIBLE|FLAGS_OPER|FLAGS_WALLOP|FLAGS_AWAY|FLAGS_SMODE)
# else /* USE_SSL */
# define SEND_UMODES	(FLAGS_INVISIBLE|FLAGS_OPER|FLAGS_WALLOP|FLAGS_AWAY)
# endif /* USE_SSL */
#endif /* RUSNET_IRCD */
#define ALL_UMODES	(SEND_UMODES|FLAGS_LOCOP|FLAGS_RESTRICTED)

/*
 * flags macros.
 */
#define	IsOper(x)		((x)->user && (x)->user->flags & FLAGS_OPER)
#define	IsLocOp(x)		((x)->user && (x)->user->flags & FLAGS_LOCOP)
#define	IsInvisible(x)		((x)->user->flags & FLAGS_INVISIBLE)
#define IsRestricted(x)         ((x)->user && \
				 (x)->user->flags & FLAGS_RESTRICTED)
#define	IsAnOper(x)		((x)->user && \
				 (x)->user->flags & (FLAGS_OPER|FLAGS_LOCOP))
#define	IsPerson(x)		((x)->user && IsClient(x))
#define	IsPrivileged(x)		(IsServer(x) || IsAnOper(x))
#ifdef WALLOPS_TO_CHANNEL
#define SendWallops(x)		0
#else
#define	SendWallops(x)		((x)->user->flags & FLAGS_WALLOP)
#endif
#define	IsUnixSocket(x)		((x)->flags & FLAGS_UNIX)
#define	IsListener(x)		((x)->flags & FLAGS_LISTEN)
#define	IsLocal(x)		(MyConnect(x) && (x)->flags & FLAGS_LOCAL)
#define	IsDead(x)		((x)->flags & FLAGS_DEADSOCKET)
#ifdef USE_SSL
# define IsSSLListener(x)	(IsListener(x) && sslable && (x)->confs && \
				(x)->confs->value.aconf && \
				((x)->confs ->value.aconf->status & \
				CONF_SSL_LISTEN_PORT))
# define IsSSL(x)		(sslable && ((x)->flags & FLAGS_SSL))
# define IsSMode(x)		((x)->user && (x)->user->flags & FLAGS_SMODE)
#endif /* USE_SSL */
#ifdef RUSNET_RLINES
# define IsRMode(x)		((x)->user && (x)->user->flags & FLAGS_RMODE)
# define IsRLine(x)		((x)->flags & FLAGS_RLINE)
#endif
#define	CBurst(x)		((x)->flags & FLAGS_CBURST)
#define	SetOper(x)		((x)->user->flags |= FLAGS_OPER)
#define	SetLocOp(x)    		((x)->user->flags |= FLAGS_LOCOP)
#define	SetInvisible(x)		((x)->user->flags |= FLAGS_INVISIBLE)
#define SetRestricted(x)        ((x)->user->flags |= FLAGS_RESTRICTED)
#define	SetWallops(x)  		((x)->user->flags |= FLAGS_WALLOP)
#define	SetUnixSock(x)		((x)->flags |= FLAGS_UNIX)
#define	SetDNS(x)		((x)->flags |= FLAGS_DOINGDNS)
#define	SetDoneXAuth(x)		((x)->flags |= FLAGS_XAUTHDONE)
#ifdef USE_SSL
# define SetSSL(x)		((x)->flags |= FLAGS_SSL)
# define SetSMode(x)		((x)->user->flags |= FLAGS_SMODE)
#endif /* USE_SSL */
#ifdef RUSNET_RLINES
# define SetRMode(x)		((x)->user->flags = (x)->user->flags \
				    | FLAGS_RMODE & ~FLAGS_VHOST)
# define SetRLine(x)		(((x)->flags |= FLAGS_RLINE) && SetRMode(x))
#endif
#define	DoingDNS(x)		((x)->flags & FLAGS_DOINGDNS)
#define	DoingAuth(x)		((x)->flags & FLAGS_AUTH)
#define	DoingXAuth(x)		((x)->flags & FLAGS_XAUTH)
#define	WaitingXAuth(x)		((x)->flags & FLAGS_WXAUTH)
#define	DoneXAuth(x)		((x)->flags & FLAGS_XAUTHDONE)
#define	NoNewLine(x)		((x)->flags & FLAGS_NONL)

#define	ClearOper(x)		((x)->user->flags &= ~FLAGS_OPER)
#define	ClearInvisible(x)	((x)->user->flags &= ~FLAGS_INVISIBLE)
#define ClearRestricted(x)      ((x)->user->flags &= ~FLAGS_RESTRICTED)
#define	ClearWallops(x)		((x)->user->flags &= ~FLAGS_WALLOP)
#define	ClearDNS(x)		((x)->flags &= ~FLAGS_DOINGDNS)
#define	ClearAuth(x)		((x)->flags &= ~FLAGS_AUTH)
#define	ClearXAuth(x)		((x)->flags &= ~FLAGS_XAUTH)
#define	ClearWXAuth(x)		((x)->flags &= ~FLAGS_WXAUTH)

#ifdef RUSNET_IRCD
#define	HasVHost(x)		((x)->user && (x)->user->flags & FLAGS_VHOST)
#define	SetVHost(x)		(!IsRMode(x) && ((x)->user->flags |= FLAGS_VHOST))
#define	ClearVHost(x)		((x)->user->flags &= ~FLAGS_VHOST)
#define IsRusnetServices(x)	((x)->user && (x)->user->servp && \
				    ((x)->user->servp->crc == invincible))
#endif

#ifdef WALLOPS_TO_CHANNEL
#define	ClearWallop(x)		((x)->user->flags &= ~FLAGS_WALLOP)
#endif
/*
 * Character map macros
 */
#define PRINT_CHAR	0x001
#define CNTRL_CHAR	0x002
#define ALPHA_CHAR	0x004
#define PUNCT_CHAR	0x008
#define DIGIT_CHAR	0x010
#define SPACE_CHAR	0x020
#define NICK_CHAR	0x040
#define CHAN_CHAR	0x080
#define LWILD_CHAR	0x100
#define CHANPFX_CHAR	0x200
#define USER_CHAR	0x400
#define HOST_CHAR	0x800
#define NONEOS_CHAR	0x1000
#define SERV_CHAR	0x2000
#define EOL_CHAR	0x4000

/* Deprecated */
#undef isalpha
#undef isdigit
#undef isxdigit
#if !defined(RUSNET_IRCD) || defined(USE_OLD8BIT) /* may be multibyte! */
#undef isalnum
#endif
#undef isprint
#undef isascii
#undef isgraph
#undef ispunct
#undef islower
#undef isupper
#undef isspace

#define isalpha(c)	(char_atribs[(u_char)(c)] & ALPHA_CHAR)
#define isspace(c)	(char_atribs[(u_char)(c)] & SPACE_CHAR)
#define islower(c)	((char_atribs[(u_char)(c)] & ALPHA_CHAR) && \
				((u_char)(c) > (u_char)0x5f))
#define isupper(c)	((char_atribs[(u_char)(c)] & ALPHA_CHAR) && \
				((u_char)(c) < (u_char)0x60))
#define isdigit(c)	(char_atribs[(u_char)(c)] & DIGIT_CHAR)
#define	isxdigit(c)	(isdigit(c) || \
		    	    ((u_char)'a' <= (u_char)(c) && \
			    (u_char)(c) <= (u_char)'f') || \
		    	    ((u_char)'A' <= (u_char)(c) && \
		    	    (u_char)(c) <= (u_char)'F'))
#if !defined(RUSNET_IRCD) || defined(USE_OLD8BIT)
#define isalnum(c)	(char_atribs[(u_char)(c)] & (DIGIT_CHAR | ALPHA_CHAR))
#endif
#define isprint(c)	(char_atribs[(u_char)(c)] & PRINT_CHAR)
#define isascii(c)	(/*((u_char)(c) >= (u_char)'\0') &&*/ \
			    ((u_char)(c) <= (u_char)0x7f))
#define isgraph(c)	((char_atribs[(u_char)(c)] & PRINT_CHAR) && \
			    ((u_char)(c) != (u_char)0x20))
#define ispunct(c)	(!(char_atribs[(u_char)(c)] & (CNTRL_CHAR | ALPHA_CHAR | DIGIT_CHAR)))
/* End of deprecated */

/* Needs adoption for RusNet yet */
#define IsHostChar(c)	(CharAttrs[(u_char)(c)] & HOST_CHAR)
#define IsUserChar(c)   (CharAttrs[(u_char)(c)] & USER_CHAR)
#define IsChanPrefix(c) (CharAttrs[(u_char)(c)] & CHANPFX_CHAR)
#define IsChanChar(c)   (CharAttrs[(u_char)(c)] & CHAN_CHAR)
#define IsLWildChar(c)  (CharAttrs[(u_char)(c)] & LWILD_CHAR)
#define IsNickChar(c)   (CharAttrs[(u_char)(c)] & NICK_CHAR)
#define IsServChar(c)   (CharAttrs[(u_char)(c)] & (NICK_CHAR | SERV_CHAR))
#define IsCntrl(c)      (CharAttrs[(u_char)(c)] & CNTRL_CHAR)
#define IsAlpha(c)      (CharAttrs[(u_char)(c)] & ALPHA_CHAR)
#define IsSpace(c)      (CharAttrs[(u_char)(c)] & SPACE_CHAR)
#define IsLower(c)      (IsAlpha((c)) && ((u_char)(c) > 0x5f))
#define IsUpper(c)      (IsAlpha((c)) && ((u_char)(c) < 0x60))
#define IsDigit(c)      (CharAttrs[(u_char)(c)] & DIGIT_CHAR)
#define IsXDigit(c)	(IsDigit(c) || ('a' <= (c) && (c) <= 'f') || \
    			    ('A' <= (c) && (c) <= 'F'))
#define IsAlNum(c) 	(CharAttrs[(u_char)(c)] & (DIGIT_CHAR | ALPHA_CHAR))
#define IsPrint(c) 	(CharAttrs[(u_char)(c)] & PRINT_CHAR)
#define IsAscii(c) 	((u_char)(c) < 0x80)
#define IsGraph(c) 	(IsPrint((c)) && ((u_char)(c) != 0x32))
#define IsPunct(c) 	(!(CharAttrs[(u_char)(c)] & \
                            (CNTRL_CHAR | ALPHA_CHAR | DIGIT_CHAR)))
#define IsNonEOS(c)	(CharAttrs[(u_char)(c)] & NONEOS_CHAR)
#define IsEol(c) 	(CharAttrs[(u_char)(c)] & EOL_CHAR)

 

/*
 * defined debugging levels
 */
#define	DEBUG_FATAL  0
#define	DEBUG_ERROR  1	/* report_error() and other errors that are found */
#define	DEBUG_READ   2
#define	DEBUG_WRITE  2
#define	DEBUG_NOTICE 3
#define	DEBUG_DNS    4	/* used by all DNS related routines - a *lot* */
#define	DEBUG_INFO   5	/* general usful info */
#define	DEBUG_NUM    6	/* numerics */
#define	DEBUG_SEND   7	/* everything that is sent out */
#define	DEBUG_DEBUG  8	/* anything to do with debugging, ie unimportant :) */
#define	DEBUG_MALLOC 9	/* malloc/free calls */
#define	DEBUG_LIST  10	/* debug list use */
#define	DEBUG_L10   10
#define	DEBUG_L11   11

/*
 * defines for curses in client
 */
#define	DUMMY_TERM	0
#define	CURSES_TERM	1
#define	TERMCAP_TERM	2

struct	CPing	{
	u_short	port;		/* port to send pings to */
	u_long	rtt;		/* average RTT */
	u_long	ping;
	u_long	seq;		/* # sent still in the "window" */
	u_long	lseq;		/* sequence # of last sent */
	u_long	recvd;		/* # received still in the "window" */
	u_long	lrecvd;		/* # received */
};

struct	ConfItem	{
	u_int	status;		/* If CONF_ILLEGAL, delete when no clients */
	int	clients;	/* Number of *LOCAL* clients using this */
	struct	IN_ADDR ipnum;	/* ip number of host field */
	char	*host;
	char	*passwd;
	char	*name;
	int	port;
	u_int	pref;		/* preference value */
#ifdef RUSNET_IRCD
	u_int	localpref;	/* administrative preference value */
#endif
	struct	CPing	*ping;
	time_t	hold;	/* Hold action until this time (calendar time) */
#ifndef VMSP
	aClass	*class;  /* Class of connection */
#endif
	struct	ConfItem *next;
};

#define	CONF_ILLEGAL		0x80000000
#define	CONF_MATCH		0x40000000
#define	CONF_QUARANTINED_SERVER	0x0000001
#define	CONF_CLIENT		0x0000002
#define CONF_RCLIENT            0x0000004
#define	CONF_CONNECT_SERVER	0x0000008
#define	CONF_NOCONNECT_SERVER	0x0000010
#define	CONF_ZCONNECT_SERVER	0x0000020
#define	CONF_LOCOP		0x0000040
#define	CONF_OPERATOR		0x0000080
#define	CONF_ME			0x0000100
#define	CONF_KILL		0x0000200
#define	CONF_ADMIN		0x0000400

#define	CONF_CLASS		0x0001000
#define	CONF_SERVICE		0x0002000
#define	CONF_LEAF		0x0004000
#define	CONF_LISTEN_PORT	0x0008000
#define	CONF_HUB		0x0010000
#define	CONF_VER		0x0020000
#define	CONF_BOUNCE		0x0040000
#define	CONF_DENY		0x0100000

#ifdef 	RUSNET_IRCD
#define	CONF_INTERFACE		0x0200000
#endif
#define	CONF_EXEMPT		0x0400000

#ifdef USE_SSL
#define CONF_SSL_LISTEN_PORT	0x0800000
#endif /* USE_SSL */

#ifdef RUSNET_RLINES
#define CONF_RUSNETRLINE	0x1000000
#endif
#ifdef LOG_EVENTS
#define	CONF_LOG		0x2000000
#endif

#define	CONF_OPS		(CONF_OPERATOR | CONF_LOCOP)
#define	CONF_SERVER_MASK	(CONF_CONNECT_SERVER | CONF_NOCONNECT_SERVER |\
				 CONF_ZCONNECT_SERVER)
#define	CONF_CLIENT_MASK	(CONF_CLIENT | CONF_RCLIENT | CONF_SERVICE | CONF_OPS | \
				 CONF_SERVER_MASK)
#ifdef RUSNET_RLINES
# define CONF_TLINE		(CONF_KILL | CONF_EXEMPT | CONF_RUSNETRLINE)
#else
# define CONF_TLINE 		(CONF_KILL | CONF_EXEMPT)
#endif

#define	IsIllegal(x)		((x)->status & CONF_ILLEGAL)
typedef	struct	{
	u_long	pi_id;
	u_long	pi_seq;
	struct	timeval	pi_tv;
	aConfItem *pi_cp;
} Ping;


#define	PING_REPLY	0x01
#define	PING_CPING	0x02

#ifdef	ZIP_LINKS
/* the minimum amount of data needed to trigger compression */
# define	ZIP_MINIMUM	4096

/* the maximum amount of data to be compressed (can actually be a bit more) */
# define	ZIP_MAXIMUM	8192	/* WARNING: *DON'T* CHANGE THIS!!!! */

struct Zdata {
	z_stream	*in;		/* input zip stream data */
	z_stream	*out;		/* output zip stream data */
	char		outbuf[ZIP_MAXIMUM]; /* outgoing (unzipped) buffer */
	int		outcount;	/* size of outbuf content */
};
#endif

struct LineItem
{ 
    char    *line;
    struct  LineItem *next;
};

/*
 * Client structures
 */
struct	User	{
	Link	*channel;	/* chain of channel pointer blocks */
	Link	*invited;	/* chain of invite pointer blocks */
	Link	*uwas;		/* chain of whowas pointer blocks */
	char	*away;		/* pointer to away message */
	time_t	last;		/* "idle" time */
	int	refcnt;		/* Number of times this block is referenced
				** from aClient (field user), aServer (field
				** by) and whowas array (field ww_user).
				*/
	int	joined;		/* number of channels joined */
	int	flags;		/* user modes */
        struct	Server	*servp;
				/*
				** In a perfect world the 'server' name
				** should not be needed, a pointer to the
				** client describing the server is enough.
				** Unfortunately, in reality, server may
				** not yet be in links while USER is
				** introduced... --msa
				*/
	aClient	*bcptr;
	char	username[USERLEN+1];
	char	host[HOSTLEN+1];
	char	*server;
};

struct	Server	{
	anUser	*user;		/* who activated this connection */
	aClient	*up;		/* uplink for this server */
	aConfItem *nline;	/* N-line pointer for this server */
	int	version;        /* version id for local client */
	int	snum;
	int	stok,
		ltok;
	int	refcnt;		/* Number of times this block is referenced
				** from anUser (field servp), aService (field
				** servp) and aClient (field serv)
				*/
	struct	Server	*nexts, *prevs, *shnext;
	aClient	*bcptr;
	char	by[HOSTLEN+1];	/* see Client::name */
	char	tok[5];
#ifdef RUSNET_IRCD
	unsigned long	crc;	/* CRC32 from server name, or any unique ID */
#endif
	time_t	lastload;	/* penalty like counters, see s_serv.c
				** should be in the local part, but..
				*/
};

struct	Service	{
	int	wants;
	int	type;
	char	*server;
	aServer	*servp;
	struct	Service	*nexts, *prevs;
	aClient	*bcptr;
	char	dist[HOSTLEN+1];
};

struct Client	{
	struct	Client *next,*prev, *hnext;
	anUser	*user;		/* ...defined, if this is a User */
	aServer	*serv;		/* ...defined, if this is a server */
	aService *service;
	u_int	hashv;		/* raw hash value */
	long	flags;		/* client flags */
	aClient	*from;		/* == self, if Local Client, *NEVER* NULL! */
	int	fd;		/* >= 0, for local clients */
	int	hopcount;	/* number of servers to this 0 = local */
	short	status;		/* Client type */
	char	name[HOSTLEN+1]; /* Unique name of the client, nick or host */
	char	username[USERLEN+1]; /* username here now for auth stuff */
	char	*info;		/* Free form additional client information */
	char	sockhost[HOSTLEN+1]; /* This is the host name from the socket
				  ** and after which the connection was
				  ** accepted.
				  */
#if defined(RUSNET_IRCD) && !defined(USE_OLD8BIT)
	struct	conversion *conv; /* charset conversion structure pointer */
	char	ucname[HOSTLEN+1]; /* uppercase version of name */
#endif
	/*
	** The following fields are allocated only for local clients
	** (directly connected to *this* server with a socket.
	** The first of them *MUST* be the "count"--it is the field
	** to which the allocation is tied to! *Never* refer to
	** these fields, if (from != self).
	*/
	int	count;		/* Amount of data in buffer */
#if defined(RUSNET_IRCD) && !defined(USE_OLD8BIT)
	char	buffer[MB_LEN_MAX*BUFSIZE]; /* Incoming message buffer */
#else
	char	buffer[BUFSIZE]; /* Incoming message buffer */
#endif
#ifdef	ZIP_LINKS
	aZdata	*zip;		/* zip data */
#endif
	short	lastsq;		/* # of 2k blocks when sendqueued called last*/
	dbuf	sendQ;		/* Outgoing message queue--if socket full */
	dbuf	recvQ;		/* Hold for data incoming yet to be parsed */
	long	sendM;		/* Statistics: protocol messages send */
	long	sendK;		/* Statistics: total k-bytes send */
	long	receiveM;	/* Statistics: protocol messages received */
	long	receiveK;	/* Statistics: total k-bytes received */
	u_short	sendB;		/* counters to count upto 1-k lots of bytes */
	u_short	receiveB;	/* sent and received. */
	time_t	lasttime;	/* last time we received data */
	time_t	firsttime;	/* time client was created */
	time_t	since;		/* last time we parsed something */
	aClient	*acpt;		/* listening client which we accepted from */
	Link	*confs;		/* Configuration record associated */
	int	authfd;		/* fd for rfc931 authentication */
	char	*auth;
	u_short	port;		/* and the remote port# too :-) */
	struct	IN_ADDR	ip;	/* keep real ip# too */
	struct	hostent	*hostp;
	char	passwd[PASSWDLEN+1];
	char	exitc;

#ifdef RUSNET_IRCD
#ifdef USE_OLD8BIT
	struct Codepage *transptr; /* Translation table structure pointer */
#endif
	u_int	flood;  /* client flood allowed (from I:line)  --erra */
#endif
#ifdef USE_SSL
	struct SSL *ssl;
	struct X509 *client_cert;
#endif /* USE SSL */
	
};

#define	CLIENT_LOCAL_SIZE sizeof(aClient)
#define	CLIENT_REMOTE_SIZE offsetof(aClient,count)

/*
 * statistics structures
 */
struct	stats {
	u_int	is_cl;	/* number of client connections */
	u_int	is_sv;	/* number of server connections */
	u_int	is_ni;	/* connection but no idea who it was
			 * (can be a P: line that has been removed -krys) */
	u_short	is_cbs;	/* bytes sent to clients */
	u_short	is_cbr;	/* bytes received to clients */
	u_short	is_sbs;	/* bytes sent to servers */
	u_short	is_sbr;	/* bytes received to servers */
	u_long	is_cks;	/* k-bytes sent to clients */
	u_long	is_ckr;	/* k-bytes received to clients */
	u_long	is_sks;	/* k-bytes sent to servers */
	u_long	is_skr;	/* k-bytes received to servers */
	time_t	is_cti;	/* time spent connected by clients */
	time_t	is_sti;	/* time spent connected by servers */
	u_int	is_ac;	/* connections accepted */
	u_int	is_ref;	/* accepts refused */
	u_int	is_unco; /* unknown commands */
	u_int	is_wrdi; /* command going in wrong direction */
	u_int	is_unpf; /* unknown prefix */
	u_int	is_empt; /* empty message */
	u_int	is_num;	/* numeric message */
	u_int	is_kill; /* number of kills generated on collisions */
	u_int	is_fake; /* MODE 'fakes' */
	u_int	is_asuc; /* successful auth requests */
	u_int	is_abad; /* bad auth requests */
	u_int	is_udpok;	/* packets recv'd on udp port */
	u_int	is_udperr;	/* packets recvfrom errors on udp port */
	u_int	is_udpdrop;	/* packets recv'd but dropped on udp port */
	u_int	is_loc;	/* local connections made */
	u_int	is_nosrv; /* user without server */
	u_long	is_wwcnt; /* number of nicks overwritten in whowas[] */
	unsigned long long	is_wwt;	/* sum of elapsed time on when 
					** overwriting whowas[] */
	u_long	is_wwMt;  /* max elapsed time on when overwriting whowas[] */
	u_long	is_wwmt;  /* min elapsed time on when overwriting whowas[] */
	u_long	is_lkcnt; /* number of nicks overwritten in locked[] */
	unsigned long long	is_lkt;	/* sum of elapsed time on when
					** overwriting locked[]*/
	u_long	is_lkMt;  /* max elapsed time on when overwriting locked[] */
	u_long	is_lkmt;  /* min elapsed time on when overwriting locked[] */
	u_int	is_ckl;   /* calls to check_link() */
	u_int	is_cklQ;  /* rejected: SendQ too high */
	u_int	is_ckly;  /* rejected: link too young */
	u_int	is_cklno; /* rejected: "flood" */
	u_int	is_cklok; /* accepted */
	u_int	is_cklq;  /* accepted early */
};

/* mode structure for channels */

struct	SMode	{
	u_int	mode;
	int	limit;
	char	key[KEYLEN+1];
};

/* Message table structure */

struct	Message	{
	char	*cmd;
	int	(* func)();
	int	parameters;
	u_int	flags;
		/* bit 0 set means that this command is allowed to be used
		 * only on the average of once per 2 seconds -SRB */
	u_int	count;	/* total count */
	u_int	rcount;	/* remote count */
	u_long	bytes;
};

#define	MSG_LAG		0x0001
#define	MSG_NOU		0x0002	/* Not available to users */
#define	MSG_SVC		0x0004	/* Services only */
#define	MSG_NOUK	0x0008	/* Not available to unknowns */
#define	MSG_REG		0x0010	/* Must be registered */
#define	MSG_REGU	0x0020	/* Must be a registered user */
/*#define	MSG_PP		0x0040*/
/*#define	MSG_FRZ		0x0080*/
#define	MSG_OP		0x0100	/* opers only */
#define	MSG_LOP		0x0200	/* locops only */

/* fd array structure */

struct	fdarray	{
	int	fd[MAXCONNECTIONS];
	int	highest;
};

/* general link structure used for chains */

struct	SLink	{
	struct	SLink	*next;
	union {
		aClient	*cptr;
		aChannel *chptr;
		aConfItem *aconf;
		char	*cp;
		int	i;
	} value;
#if defined(RUSNET_IRCD) && !defined(USE_OLD8BIT)
	char	*ucp;	/* uppercase value */
#endif
	int	flags;
};

/* channel structure */

struct Channel	{
	struct	Channel *nextch, *prevch, *hnextch;
	u_int	hashv;		/* raw hash value */
	Mode	mode;
#if defined(RUSNET_IRCD) && !defined(USE_OLD8BIT)
	/* utf-8 char may have 1 to 6 bytes, assume average is 2  -LoSt */
	char    topic[2*TOPICLEN+1];
#else
	char	topic[TOPICLEN+1];
#endif
#ifdef TOPICWHOTIME
        char    topic_nick[UNINICKLEN+1];
        time_t  topic_time;
#endif
	int	users;		/* current membership total */
	Link	*members;	/* channel members */
	Link	*invites;	/* outstanding invitations */
	Link	*mlist;		/* list of extended modes: +b/+e/+I */
	Link	*clist;		/* list of local! connections which are members */
	time_t	history;	/* channel history (aka channel delay) */
	time_t	reop;		/* server reop stamp for !channels */
	char	*chname;
#if defined(RUSNET_IRCD) && !defined(USE_OLD8BIT)
	char	*ucname;
#endif
};

/*
** Channel Related macros follow
*/

/* Channel related flags */

#define	CHFL_UNIQOP     0x0001 /* Channel creator */
#define	CHFL_CHANOP     0x0002 /* Channel operator */
#define	CHFL_VOICE      0x0004 /* the power to speak */
#define	CHFL_BAN	0x0008 /* ban channel flag */
#define	CHFL_EXCEPTION	0x0010 /* exception channel flag */
#define	CHFL_INVITE	0x0020 /* invite channel flag */
#ifdef RUSNET_IRCD
#define	CHFL_HALFOP	0x80000 /* Channel half-operator */
#endif

/* Channel Visibility macros */

#define	MODE_UNIQOP	CHFL_UNIQOP
#define	MODE_CHANOP	CHFL_CHANOP
#define	MODE_VOICE	CHFL_VOICE
#define	MODE_PRIVATE	0x0008
#define	MODE_SECRET	0x0010
#define	MODE_MODERATED  0x0020
#define	MODE_TOPICLIMIT 0x0040
#define	MODE_INVITEONLY 0x0080
#define	MODE_NOPRIVMSGS 0x0100
#define	MODE_KEY	0x0200
#define	MODE_BAN	0x0400
#define	MODE_LIMIT	0x0800
#define	MODE_ANONYMOUS	0x1000
#define	MODE_QUIET	0x2000
#define	MODE_EXCEPTION	0x4000
#define	MODE_INVITE	0x8000
#define	MODE_REOP	0x10000

#ifndef RUSNET_IRCD
#define	MODE_FLAGS	0x1ffff
#else
#define	MODE_HALFOP	CHFL_HALFOP
#define	MODE_7BIT	0x20000
#define	MODE_NOCOLOR	0x40000
#define	MODE_FLAGS	0xfffff
#endif

/*
 * mode flags which take another parameter (With PARAmeterS)
 */
#ifndef RUSNET_IRCD
#define	MODE_WPARAS	(MODE_UNIQOP|MODE_CHANOP|MODE_VOICE|MODE_BAN|MODE_KEY\
			 |MODE_LIMIT|MODE_INVITE|MODE_EXCEPTION)
#else
#define	MODE_WPARAS	(MODE_UNIQOP|MODE_CHANOP|MODE_VOICE|MODE_BAN|MODE_KEY\
			 |MODE_LIMIT|MODE_INVITE|MODE_EXCEPTION|MODE_HALFOP)
#endif
/*
 * Undefined here, these are used in conjunction with the above modes in
 * the source.
#define	MODE_DEL       0x40000000
#define	MODE_ADD       0x80000000
 */

#define	HoldChannel(x)		(!(x))
/* name invisible */
#define	SecretChannel(x)	((x) && ((x)->mode.mode & MODE_SECRET))
/* channel not shown but names are */
#define	HiddenChannel(x)	((x) && ((x)->mode.mode & MODE_PRIVATE))
/* channel visible */
#define	ShowChannel(v,c)	(PubChannel(c) || IsMember((v),(c)))
#define	IsAnonymous(c)		((c) && ((c)->mode.mode & MODE_ANONYMOUS))
#define	PubChannel(x)		((!x) || ((x)->mode.mode &\
				 (MODE_PRIVATE | MODE_SECRET)) == 0)

/*
#define	IsMember(u, c)		(assert(*(c)->chname != '\0'), find_user_link((c)->members, u) ? 1 : 0)
#define	IsMember(u, c)		(find_user_link((c)->members, u) ? 1 : 0)
*/
#define IsMember(u, c)          (u && (u)->user && \
		    			find_channel_link((u)->user->channel, c) ? 1 : 0)
#ifdef CLIENT_COMPILE
# define IsChannelName(n)	((n) && (*(n) == '#' || *(n) == '&' ||\
					*(n) == '+' || *(n) == '!'))
#else
# define IsChannelName(n)	((n) && (*(n) == '#' || *(n) == '&' ||\
					*(n) == '+' || \
					(*(n) == '!' && cid_ok(n))))
#endif
#define	IsQuiet(x)		((x)->mode.mode & MODE_QUIET)
#define	UseModes(n)		((n) && (*(n) == '#' || *(n) == '&' || \
					 *(n) == '!'))


#define	MyConnect(x)		((x)->fd >= 0)
#define	MyClient(x)		(MyConnect(x) && IsClient(x))
#define	MyPerson(x)		(MyConnect(x) && IsPerson(x))
#define	MyOper(x)		(MyConnect(x) && IsOper(x))
#define	MyService(x)		(MyConnect(x) && IsService(x))
#define	ME			me.name

#define	GotDependantClient(x)	(x->prev &&				\
		 		 ((IsRegisteredUser(x->prev) &&		\
				  x->prev->user->servp == x->serv) ||	\
				  (IsService(x->prev) &&		\
				  x->prev->service->servp == x->serv)))

typedef	struct	{
	u_long	is_user[2];	/* users, non[0] invis and invis[1] */
	u_long	is_serv;	/* servers */
	u_long	is_service;	/* services */
	u_long	is_chan;	/* channels */
	u_long	is_chanmem;
	u_long	is_chanusers;	/* channels users */
	u_long	is_hchan;	/* channels in history */
	u_long	is_hchanmem;
	u_long	is_cchan;	/* channels in cache */
	u_long	is_cchanmem;
	u_long	is_away;	/* away sets */
	u_long	is_awaymem;
	u_long	is_oper;	/* opers */
	u_long	is_bans;	/* bans */
	u_long	is_banmem;
	u_long	is_invite;	/* invites */
	u_long	is_class;	/* classes */
	u_long	is_conf;	/* conf lines */
	u_long	is_confmem;
	u_long	is_conflink;	/* attached conf lines */
	u_long	is_myclnt;	/* local clients */
	u_long	is_myserv;	/* local servers */
	u_long	is_myservice;	/* local services */
	u_long	is_unknown;	/* unknown (local) connections */
	u_long	is_wwusers;	/* users kept for whowas[] */
	u_long	is_wwaways;	/* aways in users in whowas[] */
	u_long	is_wwawaysmem;
	u_long	is_wwuwas;	/* uwas links */
	u_long	is_localc;	/* local items (serv+service+client+..) */
	u_long	is_remc;	/* remote clients */
	u_long	is_users;	/* user structs */
	u_long	is_useri;	/* user invites */
	u_long	is_userc;	/* user links to channels */
	u_long	is_auth;	/* OTHER ident reply block */
	u_long	is_authmem;
	u_int	is_dbuf;	/* number of dbuf allocated (originally) */
	u_int	is_dbufnow;	/* number of dbuf allocated */
	u_int	is_dbufuse;	/* number of dbuf in use */
	u_int	is_dbufmin;	/* min number of dbuf in use */
	u_int	is_dbufmax;	/* max number of dbuf in use */
	u_int	is_dbufmore;	/* how many times we increased the bufferpool*/
#ifdef EXTRA_STATISTICS
	time_t  is_last_cnt_t;  /* timestamp for last count */
	u_long  is_last_cnt;    /* last count */
	u_long  is_m_users;     /* maximum users connected */
	u_long  is_m_serv;      /* maximum servers connected */
	u_long  is_m_service;   /* maximum services connected */
	u_long  is_m_myclnt;    /* maximum local clients */
	u_long  is_m_myserv;    /* maximum local servers */
	u_long  is_m_myservice; /* maximum local services */
#endif
#ifdef DELAY_CLOSE
	u_long	is_delayclose;	/* number of fds that got delayed close() */
	u_int	is_delayclosewait;	/* number of fds that wait for delayed close() */
#endif
} istat_t;


/* used in SetMode() in channel.c and m_umode() in s_msg.c */

#define	MODE_NULL      0
#define	MODE_ADD       0x40000000
#define	MODE_DEL       0x20000000

/* return values for hunt_server() */

#define	HUNTED_NOSUCH	(-1)	/* if the hunted server is not found */
#define	HUNTED_ISME	0	/* if this server should execute the command */
#define	HUNTED_PASS	1	/* if message passed onwards successfully */

/* used when sending to #mask or $mask */

#define	MATCH_SERVER  1
#define	MATCH_HOST    2

/* used for sendto_serv */

#define	SV_OLD		0x0000
#define	SV_29		0x0001	/* useless, but preserved for coherence */
#define	SV_NJOIN	0x0002	/* server understands the NJOIN command */
#define	SV_NMODE	0x0004	/* server knows new MODEs (+e/+I) */
#define	SV_NCHAN	0x0008	/* server knows new channels -????name */
				/* ! SV_NJOIN implies ! SV_NCHAN */
#define	SV_2_10		(SV_29|SV_NJOIN|SV_NMODE|SV_NCHAN)
#define	SV_OLDSQUIT	0x1000	/* server uses OLD SQUIT logic */

#ifdef RUSNET_IRCD
#define	SV_FORCE	0x0100	/* FORCE protocol command */
#define	SV_RMODE	0x0200	/* RMODE and RCPAGE protocol command */
#define	SV_KLINE	0x0400	/* KLINE protocol command */
#define SV_RLINE	0x0800	/* RLINE protocol command */
#define SV_RUSNET_1_5	0x1000	/* full RusNet 1.5 */
#define	PROTO_CAPS_K	'K' 	/* KLINE handshake */
#define	PROTO_CAPS_R	'R' 	/* RMODE and RCPAGE handshake */
#define PROTO_CAPS_RL	'L'	/* RLINE hs */
#endif


/* used for sendto_flag */

#define	SCH_ERROR	1
#define	SCH_NOTICE	2
#define	SCH_KILL	3
#define	SCH_CHAN	4
#define	SCH_NUM		5
#define	SCH_SERVER	6
#define	SCH_HASH	7
#define	SCH_LOCAL	8
#define	SCH_SERVICE	9
#define	SCH_DEBUG	10
#define	SCH_AUTH	11
#define	SCH_OPER        12
#define SCH_ISERV	13
#define SCH_WALLOPS	14
#define	SCH_MAX         14

#ifdef LOG_EVENTS
struct Logger {
	int     logf;
#ifdef USE_SYSLOG
	int	syslog;
#endif
	struct	Logger *next;
};

#ifdef USE_SYSLOG
struct Syslog {
       char	*name;
       int	level;
};
#endif
#endif /* LOG_EVENTS */

typedef struct  {
       int	svc_chan;
       char	*svc_chname;
       aChannel	*svc_ptr;
#ifdef LOG_EVENTS
       struct	Logger  *log;
#endif
}      SChan;

/* used for async dns values */

#define	ASYNC_NONE	(-1)
#define	ASYNC_CLIENT	0
#define	ASYNC_CONNECT	1
#define	ASYNC_CONF	2
#define	ASYNC_SERVER	3

/* Client exit codes for log file */
#define EXITC_UNDEF	'-'	/* unregistered client */
#define EXITC_REG	'0'	/* normal exit */
#define EXITC_AUTHFAIL	'A'	/* Authentication failure (iauth problem) */
#define EXITC_AUTHTOUT	'a'	/* Authentication time out */
#define EXITC_CLONE     'C'     /* CLONE_CHECK */
#define EXITC_DIE	'd'	/* server died */
#define EXITC_DEAD	'D'	/* socket died */
#define EXITC_ERROR	'E'	/* socket error */
#define EXITC_FLOOD	'F'	/* client flooding */
#define EXITC_FAILURE	'f'	/* connect failure */
#define EXITC_GHMAX	'G'	/* global clients per host max limit */
#define EXITC_GUHMAX	'g'	/* global clients per user@host max limit */
#define EXITC_NOILINE	'I'	/* No matching I:line */
#define EXITC_KLINE	'k'	/* K-lined */
#define EXITC_KILL	'K'	/* KILLed */
#define EXITC_LHMAX	'L'	/* local clients per host max limit */
#define EXITC_LUHMAX	'l'	/* local clients per user@host max limit */
#define EXITC_MBUF	'M'	/* mem alloc error */
#define EXITC_PING	'P'	/* ping timeout */
#define EXITC_BADPASS	'p'	/* bad password */
#define EXITC_SENDQ	'Q'	/* send queue exceeded */
#define EXITC_RLINE	'r'	/* R-lined */
#define EXITC_REF	'R'	/* Refused */
#define EXITC_AREF	'U'	/* Unauthorized by iauth */
#define EXITC_AREFQ	'u'	/* Unauthorized by iauth, be quiet */
#define EXITC_VIRUS	'v'	/* joined a channel used by PrettyPark virus */
#define EXITC_YLINEMAX  'Y'     /* Y:line max clients limit */


/* eXternal authentication slave OPTions */
#define	XOPT_REQUIRED	0x01	/* require authentication be done by iauth */
#define	XOPT_NOTIMEOUT	0x02	/* disallow iauth time outs */
#define XOPT_EXTWAIT	0x10	/* extend registration ping timeout */
#define XOPT_EARLYPARSE	0x20	/* allow early parsing and send USER/PASS
				   information to iauth */

/* misc defines */

#define	FLUSH_BUFFER	-2
#define	UTMP		"/etc/utmp"
#define	COMMA		","

#define	SAP		struct SOCKADDR *

/* IRC client structures */

#ifdef	CLIENT_COMPILE
typedef	struct	Ignore {
	char	user[NICKLEN+1];
	char	from[USERLEN+HOSTLEN+2];
	int	flags;
	struct	Ignore	*next;
} anIgnore;

#define	IGNORE_PRIVATE	1
#define	IGNORE_PUBLIC	2
#define	IGNORE_TOTAL	3

#define	HEADERLEN	200

#endif /* CLIENT_COMPILE */
