/************************************************************************
 *   IRC - Internet Relay Chat, ircd/ircd_ext.h
 *   Copyright (C) 1997 Alain Nissen
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
 *   $Id: ircd_ext.h,v 1.15 2010-11-10 13:38:39 gvs Exp $
 */

/*  This file contains external definitions for global variables and functions
    defined in ircd/ircd.c.
 */

/*  External definitions for global variables.
 */
#ifndef IRCD_C
extern aClient me;
extern aClient *client;
extern istat_t istat;
extern char **myargv;
extern int restart_iauth;
extern int dorehash;
extern int dorestart;
extern int rehashed;
extern int portnum;
extern int serverbooting;
extern char *configfile;
#if defined(RUSNET_IRCD) && !defined(USE_OLD8BIT)
extern char *config_charset;
#endif
extern int debuglevel;
extern int bootopt;
extern char debugmode[4];
extern char *sbrk0;
extern char *tunefile;
#ifdef USE_SSL
extern char *strerror_ssl;
#endif
#ifdef DELAY_CLOSE
extern time_t nextdelayclose;
#endif
extern time_t nextconnect;
extern time_t nextgarbage;
extern time_t nextping;
extern time_t nextdnscheck;
extern time_t nextexpire;
extern aClient *ListenerLL;
#if defined(RUSNET_IRCD)
extern unsigned long invincible;
#endif
#endif /* IRCD_C */

/*  External definitions for global functions.
 */
#ifndef IRCD_C
#define EXTERN extern
#else /* IRCD_C */
#define EXTERN
#endif /* IRCD_C */

EXTERN void server_reboot();
EXTERN void ircd_writetune(char *filename);
EXTERN void ircd_readtune(char *filename);
EXTERN void open_debugfile(void);
#undef EXTERN
