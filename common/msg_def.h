/************************************************************************
 *   IRC - Internet Relay Chat, common/msg_def.h
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
 */

#define MSG_PRIVATE  "PRIVMSG"	/* PRIV */
#define MSG_WHO      "WHO"	/* WHO  -> WHOC */
#define MSG_WHOIS    "WHOIS"	/* WHOI */
#define MSG_WHOWAS   "WHOWAS"	/* WHOW */
#define MSG_USER     "USER"	/* USER */
#define MSG_NICK     "NICK"	/* NICK */
#define MSG_SERVER   "SERVER"	/* SERV */
#define MSG_LIST     "LIST"	/* LIST */
#define MSG_TOPIC    "TOPIC"	/* TOPI */
#define MSG_NTOPIC   "NTOPIC"	/* NTOPIC */
#define MSG_INVITE   "INVITE"	/* INVI */
#define MSG_VERSION  "VERSION"	/* VERS */
#define MSG_QUIT     "QUIT"	/* QUIT */
#define MSG_SQUIT    "SQUIT"	/* SQUI */
#define MSG_KILL     "KILL"	/* KILL */
#define MSG_INFO     "INFO"	/* INFO */
#define MSG_LINKS    "LINKS"	/* LINK */
#define MSG_SUMMON   "SUMMON"	/* SUMM */
#define MSG_STATS    "STATS"	/* STAT */
#define MSG_USERS    "USERS"	/* USER -> USRS */
#define MSG_HELP     "HELP"	/* HELP */
#define MSG_ERROR    "ERROR"	/* ERRO */
#define MSG_AWAY     "AWAY"	/* AWAY */
#define MSG_CONNECT  "CONNECT"	/* CONN */
#define MSG_PING     "PING"	/* PING */
#define MSG_PONG     "PONG"	/* PONG */
#define MSG_OPER     "OPER"	/* OPER */
#define MSG_PASS     "PASS"	/* PASS */
#define MSG_WALLOPS  "WALLOPS"	/* WALL */
#define MSG_TIME     "TIME"	/* TIME */
#define MSG_NAMES    "NAMES"	/* NAME */
#define MSG_ADMIN    "ADMIN"	/* ADMI */
#define MSG_TRACE    "TRACE"	/* TRAC */
#define MSG_NOTICE   "NOTICE"	/* NOTI */
#define MSG_JOIN     "JOIN"	/* JOIN */
#define MSG_NJOIN     "NJOIN"	/* NJOIN */
#define MSG_PART     "PART"	/* PART */
#define MSG_LUSERS   "LUSERS"	/* LUSE */
#define MSG_MOTD     "MOTD"	/* MOTD */
#define MSG_MODE     "MODE"	/* MODE */
#define MSG_UMODE    "UMODE"	/* UMOD */
#define MSG_KICK     "KICK"	/* KICK */
#define	MSG_RECONECT "RECONNECT" /* RECONNECT -> RECO */
#define MSG_SERVICE  "SERVICE"	/* SERV -> SRVI */
#define MSG_USERHOST "USERHOST"	/* USER -> USRH */
#define MSG_ISON     "ISON"	/* ISON */
#define MSG_NOTE     "NOTE"	/* NOTE */
#define MSG_EOB	     "EOB"	/* EOB */
#define MSG_EOBACK   "EOBACK"	/* EOBACK */
#define MSG_SQUERY   "SQUERY"	/* SQUE */
#define MSG_SERVLIST "SERVLIST"	/* SERV -> SLIS */
#define MSG_SERVSET  "SERVSET"	/* SERV -> SSET */
#define	MSG_REHASH   "REHASH"	/* REHA */
#define	MSG_RESTART  "RESTART"	/* REST */
#define	MSG_CLOSE    "CLOSE"	/* CLOS */
#define	MSG_DIE	     "DIE"
#define	MSG_HASH     "HAZH"	/* HASH */
#define	MSG_DNS      "DNS"	/* DNS  -> DNSS */
#define MSG_KLINE	"KLINE"		/* KLINE */
#define MSG_ELINE	"ELINE"		/* ELINE (EXEMPT) */
#define MSG_RLINE	"RLINE"		/* RLINE (restricted) COMMAND */
#define MSG_TRIGGER	"TRIGGER"	/* TRIGGER antispam COMMAND */
#define MSG_UNKLINE	"UNKLINE"	/* UNKLINE */
#define MSG_UNELINE	"UNELINE"	/* UNELINE (un-EXEMPT) */
#define MSG_UNRLINE	"UNRLINE"	/* UNRLINE (unrestricting) COMMAND */
#define MSG_UNTRIGGER	"UNTRIGGER"	/* UNTRIGGER antispam COMMAND */

#define MSG_CODEPAGE	"CODEPAGE"	/* RUSNET CODEPAGE SYSTEM - obsolete */
#ifndef USE_OLD8BIT
#define MSG_CHARSET	"CHARSET"	/* IANA CHARSET */
#endif
#define MSG_SVSNICK	"SVSNICK"	/* FORCED NICK CHANGE, NEW */
#define MSG_FORCE	"FORCE"		/* FORCED NICK CHANGE, OLD */
#define MSG_SVSMODE	"SVSMODE"	/* SERVER REMOTE USER MODE, NEW */
#define MSG_RMODE	"RMODE"		/* SERVER REMOTE USER MODE, OLD */
#define MSG_RCPAGE	"RCPAGE"	/* SERVER REMOTE CHANGE CODEPAGE */
#define	MSG_NICKSERV	"NICKSERV"	/* msg to NickServ service */
#define	MSG_CHANSERV	"CHANSERV"	/* msg to ChanServ service */
#define	MSG_MEMOSERV	"MEMOSERV"	/* msg to MemoServ service */
#define	MSG_INFOSERV	"INFOSERV"	/* msg to InfoServ service */
#define	MSG_STATSERV	"STATSERV"	/* msg to StatServ service */
#define	MSG_OPERSERV	"OPERSERV"	/* msg to OperServ service */

#define MAXPARA    15 
