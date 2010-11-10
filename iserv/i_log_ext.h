/************************************************************************
 *   IRC - Internet Relay Chat, iserv/i_log_ext.h
 *   Copyright (C) 1998 Christophe Kalt
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
 *   $Id: i_log_ext.h,v 1.4 2010-11-10 13:38:39 gvs Exp $
 */

/*  This file contains external definitions for global variables and functions
    defined in iserv/i_log.c.
 */

/*  External definitions for global functions.
 */
#ifndef I_LOG_C
# define EXTERN extern
#else /* I_LOG_C */
# define EXTERN
#endif /* I_LOG_C */

EXTERN void init_filelogs();
EXTERN void init_syslog();
EXTERN void vsendto_log (int, char *, va_list);
EXTERN void sendto_log (int, char *, ...);
#undef EXTERN
