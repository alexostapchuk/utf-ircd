/************************************************************************
 *   IRC - Internet Relay Chat, ircd/s_iserv_ext.h
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
 *   $Id: s_iserv_ext.h,v 1.3 2005/08/23 22:28:22 skold Exp $
 */

/*  This file contains external definitions for global variables and functions
    defined in ircd/s_iserv.c.
 */

/*  External definitions for global functions.
 */
#ifndef S_ISERV_C
# if defined(USE_ISERV)
extern u_char iserv_options;
extern u_int iserv_spawn;
# endif
#
# define EXTERN extern
#else /* S_ISERV_C */
# define EXTERN
#endif /* S_ISERV_C */

#if defined(USE_ISERV)
EXTERN int vsendto_iserv (char *pattern, va_list va);
EXTERN int sendto_iserv (char *pattern, ...);
#endif
#undef EXTERN
