/************************************************************************
 *   IRC - Internet Relay Chat, iserv/i_io_ext.h
 *   Copyright (C) 2005 RusNet
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
 *   $Id: i_io_ext.h,v 1.3 2005/08/27 16:05:24 skold Exp $
 */

/*  This file contains external definitions for global variables and functions
    defined in iserv/i_io.c.
 */

/*  External definitions for global functions.
 */
#ifndef I_IO_C
#define EXTERN extern
#else /* I_IO_C */
#define EXTERN
#endif /* I_IO_C */

EXTERN void loop_io();

#undef EXTERN
