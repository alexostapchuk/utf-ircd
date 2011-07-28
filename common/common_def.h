/************************************************************************
 *   IRC - Internet Relay Chat, common/common_def.h
 *   Copyright (C) 1990 Armin Gruner
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

#ifdef DEBUGMODE
# define Debug(x)		debug x
# define DO_DEBUG_MALLOC
#else
# define Debug(x) 		;
# define LOGFILE		"/dev/null"
#endif

#if defined(CHKCONF_COMPILE) || defined(CLIENT_COMPILE)
# undef	ZIP_LINKS
#endif

#define DupString(x,y)		do {x = (char *)MyMalloc(strlen((char *)y) + 1);\
				    (void)strcpy((char *)x, (char *)y);\
				    } while(0)
#undef tolower
#define tolower(c)		(tolowertab[(u_char)(c)])
#ifndef USE_OLD8BIT
#undef toupper
#define toupper(c)		(touppertab[(u_char)(c)])
#endif

#ifndef HAVE_TOWUPPER
# define towupper toupper
#endif

/*
** 'offsetof' is defined in ANSI-C. The following definition
** is not absolutely portable (I have been told), but so far
** it has worked on all machines I have needed it. The type
** should be size_t but...  --msa
*/
#ifndef offsetof
#define	offsetof(t,m)		(int)((&((t *)0L)->m))
#endif

#define	elementsof(x)		(sizeof(x)/sizeof(x[0]))

/* Misc macros */

#define	BadPtr(x)		(!(x) || (*(x) == '\0'))
#define EmptyString(x)		(!(x) || (*(x) == '\0'))

#if !defined(USE_OLD8BIT) && !defined(LOCALE_STRICT_NAMES)
#define	isvalid(c) (((c) >= 'A' && (c) <= '~') || isalnum(c) || \
				(c) == '-' || (c) >= 0xc0 || \
				((c) >= 0xa3 && (c) <= 0xa7) || \
				((c) >= 0xb3 && (c) <= 0xb7) || \
				(c) == 0xad || (c) == 0xbd)
#else
#define	isvalid(c)		(validtab[(u_char)(c)])
#endif

/* String manipulation macros */

/* strncopynt --> strncpyzt to avoid confusion, sematics changed
   N must be now the number of bytes in the array --msa */
#define	strncpyzt(x, y, N)	do{(void)strncpy(x,y,N);x[N-1]='\0';}while(0)
#define	StrEq(x,y)		(!strcmp((x),(y)))
