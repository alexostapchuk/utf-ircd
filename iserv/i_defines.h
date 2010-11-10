/************************************************************************
 *   IRC - Internet Relay Chat, iserv/i_defines.h
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
 *   $Id: i_defines.h,v 1.6 2005/08/28 16:30:24 skold Exp $
 */

/*  This file includes all files defining constants, macros and types
    definitions used by the configuration slave process.
 */

#undef DEBUGLVL	/* Debug level from 1(min) to 3(max) or undef */


#if defined(DEBUGLVL)
# define ISERV_DEBUG
#endif

#include "config.h"
#include "patchlevel.h"
#include "common_def.h"
#include "dbuf_def.h"	/* needed for struct_def.h, sigh */
#include "class_def.h"	/* needed for struct_def.h, sigh */
#include "struct_def.h"
#ifdef INET6
# include "../ircd/nameser_def.h"
#endif
#include "support_def.h"
#include "sys_def.h"
#include "i_log_def.h"

