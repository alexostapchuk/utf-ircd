/************************************************************************
 *   IRC - Internet Relay Chat, iserv/i_log_def.h
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
 *   $Id: i_log_def.h,v 1.4 2010-11-10 13:38:39 gvs Exp $
 */


#if defined(ISERV_DEBUG)
# define DebugLog(x)    sendto_log x
#else
# define DebugLog(x)
#endif

#define FNAME_ISERVLOG	"iserv.flog"	/* own log, not &ISERV channel! */
#define ISERVDEBUGLOG ISERVDBG_PATH

#define LOG_INFO	6
#define LOG_NOTICE	5
#define LOG_ERR		3
#define LOG_CRIT	2
#define LOG_WARNING	4
#define LOG_ALERT	1

#ifdef ISERV_DEBUG
#define LOG_DEBUG	7
#define	DEBUG_IO	10	/* debug: IO stuff */
#define	DEBUG_MISC	11
#define	DEBUG_RAW	12
#endif

