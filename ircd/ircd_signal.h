/************************************************************************
 *   IRC - Internet Relay Chat, ircd/ircd_signal.h
 *   ircd_signal.h: header file for ircd_signal.c
 *
 *   Copyright (C) 2003 by RusNet development team
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
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *   USA
 */


/*
 * In the perfect world we would hide all these functions behind the ircd_signals wall --skold
 */

/*  
 * External definitions for global functions.
 */
#ifndef BSD_C
#define EXTERN extern
#else /* BSD_C */
#define EXTERN
#endif /* BSD_C */
EXTERN RETSIGTYPE dummy(int s);
EXTERN RETSIGTYPE s_die(int s);
EXTERN RETSIGTYPE s_restart(int s);
EXTERN void setup_signals(void);
EXTERN void restore_sigchld(void);
#undef EXTERN

