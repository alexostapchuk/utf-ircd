/************************************************************************
 *   IRC - Internet Relay Chat, common/match.c
 *   Copyright (C) 1990 Jarkko Oikarinen
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


#include "os.h"
#include "s_defines.h"
#define MATCH_C
#include "s_externs.h"
#undef MATCH_C

/* we don't have MyMalloc/MyFree here */
#undef	free

#include <ctype.h>

#if !defined(USE_OLD8BIT) || defined(USE_LIBIDN)
# include <wchar.h>
# include <wctype.h>

# define HANDLE_MULTIBYTE

# ifndef MB_LEN_MAX
#  define MB_LEN_MAX	16
# endif

#else
# ifndef MB_LEN_MAX
#  define MB_LEN_MAX	1
# endif
#endif

/* extended flags not available in most libc fnmatch versions, but we undef
   them to avoid any possible warnings. */
#undef FNM_CASEFOLD
#undef FNM_EXTMATCH

/* Value returned by `strmatch' if STRING does not match PATTERN.  */
#undef FNM_NOMATCH

#define	FNM_NOMATCH	1
#define FNM_RNGMATCH	(1 << 4) /* Match ranges ([a-z]).           */
#define FNM_CASEFOLD	(1 << 4) /* Compare without regard to case. */
#define FNM_EXTMATCH	(1 << 5) /* Use ksh-like extended matching. */

unsigned char tolowertab[] =
		{ 0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa,
		  0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14,
		  0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
		  0x1e, 0x1f,
		  ' ', '!', '"', '#', '$', '%', '&', 0x27, '(', ')',
		  '*', '+', ',', '-', '.', '/',
		  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		  ':', ';', '<', '=', '>', '?',
		  '@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		  'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
		  't', 'u', 'v', 'w', 'x', 'y', 'z', '[', '\\', ']', '^',
		  '_',
		  '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		  'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
		  't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',
		  0x7f,
		  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
		  0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
		  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
		  0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
		  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
		  0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
		  0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
		  0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
		  0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
		  0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
		  0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
		  0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
		  0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
		  0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
		  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
		  0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff };

unsigned char touppertab[] =
		{ 0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa,
		  0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14,
		  0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
		  0x1e, 0x1f,
		  ' ', '!', '"', '#', '$', '%', '&', 0x27, '(', ')',
		  '*', '+', ',', '-', '.', '/',
		  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		  ':', ';', '<', '=', '>', '?',
		  '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
		  'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
		  'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^',
		  0x5f,
		  '`', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
		  'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S',
		  'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '{', '|', '}', '~',
		  0x7f,
		  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
		  0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
		  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
		  0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
		  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
		  0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
		  0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
		  0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
		  0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
		  0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
		  0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
		  0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
		  0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
		  0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
		  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
		  0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff };

#if defined(USE_OLD8BIT) || defined(LOCALE_STRICT_NAMES)
unsigned char validtab[256];
#endif

/* Deprecated -skold*/

unsigned char char_atribs[] = {
/* 0-7 */	CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR,
/* 8-12 */	CNTRL_CHAR, CNTRL_CHAR|SPACE_CHAR, CNTRL_CHAR|SPACE_CHAR, CNTRL_CHAR|SPACE_CHAR, CNTRL_CHAR|SPACE_CHAR,
/* 13-15 */	CNTRL_CHAR|SPACE_CHAR, CNTRL_CHAR, CNTRL_CHAR,
/* 16-23 */	CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR,
/* 24-31 */	CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR, CNTRL_CHAR,
/* space */	PRINT_CHAR|SPACE_CHAR,
/* !"#$%&'( */	PRINT_CHAR, PRINT_CHAR, PRINT_CHAR, PRINT_CHAR, PRINT_CHAR, PRINT_CHAR, PRINT_CHAR, PRINT_CHAR,
/* )*+,-./ */	PRINT_CHAR, PRINT_CHAR, PRINT_CHAR, PRINT_CHAR, PRINT_CHAR, PRINT_CHAR, PRINT_CHAR,
/* 0123 */	PRINT_CHAR|DIGIT_CHAR, PRINT_CHAR|DIGIT_CHAR, PRINT_CHAR|DIGIT_CHAR, PRINT_CHAR|DIGIT_CHAR,
/* 4567 */	PRINT_CHAR|DIGIT_CHAR, PRINT_CHAR|DIGIT_CHAR, PRINT_CHAR|DIGIT_CHAR, PRINT_CHAR|DIGIT_CHAR,
/* 89:; */	PRINT_CHAR|DIGIT_CHAR, PRINT_CHAR|DIGIT_CHAR, PRINT_CHAR, PRINT_CHAR,
/* <=>? */	PRINT_CHAR, PRINT_CHAR, PRINT_CHAR, PRINT_CHAR,
/* @ */		PRINT_CHAR,
/* ABC */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* DEF */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* GHI */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* JKL */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* MNO */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* PQR */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* STU */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* VWX */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* YZ[ */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* \]^ */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* _`  */	PRINT_CHAR, PRINT_CHAR,
/* abc */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* def */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* ghi */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* jkl */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* mno */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* pqr */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* stu */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* vwx */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* yz{ */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* \}~ */	PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR, PRINT_CHAR|ALPHA_CHAR,
/* del */	0,
/* 80-8f */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 90-9f */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* a0-af */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* b0-bf */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* c0-cf */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* d0-df */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* e0-ef */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* f0-ff */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		};

/* Use this instead */
unsigned int CharAttrs[] = {
/* 0  */     CNTRL_CHAR,
/* 1  */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 2  */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 3  */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 4  */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 5  */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 6  */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 7 BEL */  CNTRL_CHAR|NONEOS_CHAR,
/* 8  \b */  CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 9  \t */  CNTRL_CHAR|SPACE_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 10 \n */  CNTRL_CHAR|SPACE_CHAR|CHAN_CHAR|NONEOS_CHAR|EOL_CHAR,
/* 11 \v */  CNTRL_CHAR|SPACE_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 12 \f */  CNTRL_CHAR|SPACE_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 13 \r */  CNTRL_CHAR|SPACE_CHAR|CHAN_CHAR|NONEOS_CHAR|EOL_CHAR,
/* 14 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 15 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 16 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 17 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 18 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 19 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 20 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 21 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 22 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 23 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 24 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 25 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 26 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 27 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 28 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 29 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 30 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 31 */     CNTRL_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* SP */     PRINT_CHAR|SPACE_CHAR,
/* ! */      PRINT_CHAR|LWILD_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* " */      PRINT_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* # */      PRINT_CHAR|CHANPFX_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* $ */      PRINT_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR,
/* % */      PRINT_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* & */      PRINT_CHAR|CHANPFX_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* ' */      PRINT_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* ( */      PRINT_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* ) */      PRINT_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* * */      PRINT_CHAR|LWILD_CHAR|CHAN_CHAR|NONEOS_CHAR|SERV_CHAR,
/* + */      PRINT_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* , */      PRINT_CHAR|NONEOS_CHAR,
/* - */      PRINT_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* . */      PRINT_CHAR|LWILD_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR|SERV_CHAR,
/* / */      PRINT_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* 0 */      PRINT_CHAR|DIGIT_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* 1 */      PRINT_CHAR|DIGIT_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* 2 */      PRINT_CHAR|DIGIT_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* 3 */      PRINT_CHAR|DIGIT_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* 4 */      PRINT_CHAR|DIGIT_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* 5 */      PRINT_CHAR|DIGIT_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* 6 */      PRINT_CHAR|DIGIT_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* 7 */      PRINT_CHAR|DIGIT_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* 8 */      PRINT_CHAR|DIGIT_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* 9 */      PRINT_CHAR|DIGIT_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* : */      PRINT_CHAR|CHAN_CHAR|NONEOS_CHAR|HOST_CHAR,
/* ; */      PRINT_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* < */      PRINT_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* = */      PRINT_CHAR|LWILD_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* > */      PRINT_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* ? */      PRINT_CHAR|LWILD_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* @ */      PRINT_CHAR|LWILD_CHAR|CHAN_CHAR|NONEOS_CHAR,
/* A */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* B */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* C */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* D */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* E */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* F */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* G */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* H */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* I */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* J */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* K */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* L */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* M */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* N */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* O */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* P */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* Q */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* R */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* S */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* T */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* U */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* V */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* W */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* X */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* Y */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* Z */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* [ */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR,
/* \ */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR,
/* ] */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR,
/* ^ */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR,
/* _ */      PRINT_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR,
/* ` */      PRINT_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR,
/* a */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* b */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* c */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* d */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* e */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* f */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* g */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* h */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* i */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* j */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* k */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* l */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* m */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* n */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* o */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* p */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* q */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* r */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* s */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* t */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* u */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* v */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* w */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* x */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* y */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* z */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR|HOST_CHAR,
/* { */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR,
/* | */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR,
/* } */      PRINT_CHAR|ALPHA_CHAR|NICK_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR,
/* ~ */      PRINT_CHAR|ALPHA_CHAR|CHAN_CHAR|NONEOS_CHAR|USER_CHAR,
/* del  */   CHAN_CHAR|NONEOS_CHAR,
/* 0x80 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x81 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x82 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x83 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x84 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x85 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x86 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x87 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x88 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x89 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x8A */   CHAN_CHAR|NONEOS_CHAR,
/* 0x8B */   CHAN_CHAR|NONEOS_CHAR,
/* 0x8C */   CHAN_CHAR|NONEOS_CHAR,
/* 0x8D */   CHAN_CHAR|NONEOS_CHAR,
/* 0x8E */   CHAN_CHAR|NONEOS_CHAR,
/* 0x8F */   CHAN_CHAR|NONEOS_CHAR,
/* 0x90 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x91 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x92 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x93 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x94 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x95 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x96 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x97 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x98 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x99 */   CHAN_CHAR|NONEOS_CHAR,
/* 0x9A */   CHAN_CHAR|NONEOS_CHAR,
/* 0x9B */   CHAN_CHAR|NONEOS_CHAR,
/* 0x9C */   CHAN_CHAR|NONEOS_CHAR,
/* 0x9D */   CHAN_CHAR|NONEOS_CHAR,
/* 0x9E */   CHAN_CHAR|NONEOS_CHAR,
/* 0x9F */   CHAN_CHAR|NONEOS_CHAR,
/* 0xA0 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xA1 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xA2 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xA3 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xA4 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xA5 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xA6 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xA7 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xA8 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xA9 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xAA */   CHAN_CHAR|NONEOS_CHAR,
/* 0xAB */   CHAN_CHAR|NONEOS_CHAR,
/* 0xAC */   CHAN_CHAR|NONEOS_CHAR,
/* 0xAD */   CHAN_CHAR|NONEOS_CHAR,
/* 0xAE */   CHAN_CHAR|NONEOS_CHAR,
/* 0xAF */   CHAN_CHAR|NONEOS_CHAR,
/* 0xB0 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xB1 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xB2 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xB3 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xB4 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xB5 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xB6 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xB7 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xB8 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xB9 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xBA */   CHAN_CHAR|NONEOS_CHAR,
/* 0xBB */   CHAN_CHAR|NONEOS_CHAR,
/* 0xBC */   CHAN_CHAR|NONEOS_CHAR,
/* 0xBD */   CHAN_CHAR|NONEOS_CHAR,
/* 0xBE */   CHAN_CHAR|NONEOS_CHAR,
/* 0xBF */   CHAN_CHAR|NONEOS_CHAR,
/* 0xC0 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xC1 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xC2 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xC3 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xC4 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xC5 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xC6 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xC7 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xC8 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xC9 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xCA */   CHAN_CHAR|NONEOS_CHAR,
/* 0xCB */   CHAN_CHAR|NONEOS_CHAR,
/* 0xCC */   CHAN_CHAR|NONEOS_CHAR,
/* 0xCD */   CHAN_CHAR|NONEOS_CHAR,
/* 0xCE */   CHAN_CHAR|NONEOS_CHAR,
/* 0xCF */   CHAN_CHAR|NONEOS_CHAR,
/* 0xD0 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xD1 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xD2 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xD3 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xD4 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xD5 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xD6 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xD7 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xD8 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xD9 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xDA */   CHAN_CHAR|NONEOS_CHAR,
/* 0xDB */   CHAN_CHAR|NONEOS_CHAR,
/* 0xDC */   CHAN_CHAR|NONEOS_CHAR,
/* 0xDD */   CHAN_CHAR|NONEOS_CHAR,
/* 0xDE */   CHAN_CHAR|NONEOS_CHAR,
/* 0xDF */   CHAN_CHAR|NONEOS_CHAR,
/* 0xE0 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xE1 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xE2 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xE3 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xE4 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xE5 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xE6 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xE7 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xE8 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xE9 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xEA */   CHAN_CHAR|NONEOS_CHAR,
/* 0xEB */   CHAN_CHAR|NONEOS_CHAR,
/* 0xEC */   CHAN_CHAR|NONEOS_CHAR,
/* 0xED */   CHAN_CHAR|NONEOS_CHAR,
/* 0xEE */   CHAN_CHAR|NONEOS_CHAR,
/* 0xEF */   CHAN_CHAR|NONEOS_CHAR,
/* 0xF0 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xF1 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xF2 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xF3 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xF4 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xF5 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xF6 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xF7 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xF8 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xF9 */   CHAN_CHAR|NONEOS_CHAR,
/* 0xFA */   CHAN_CHAR|NONEOS_CHAR,
/* 0xFB */   CHAN_CHAR|NONEOS_CHAR,
/* 0xFC */   CHAN_CHAR|NONEOS_CHAR,
/* 0xFD */   CHAN_CHAR|NONEOS_CHAR,
/* 0xFE */   CHAN_CHAR|NONEOS_CHAR,
/* 0xFF */   CHAN_CHAR|NONEOS_CHAR
};

/*
** The whole lot of strmatch code taken from GNU bash
** It means that you can use any bash-like patterns, huh
*/

#define FREE(x)		if (x) free (x)

#if defined STDC_HEADERS || (!defined isascii && !defined HAVE_ISASCII)
#  define IN_CTYPE_DOMAIN(c) 1
#else
#  define IN_CTYPE_DOMAIN(c) isascii(c)
#endif

#if !defined (isspace) && !defined (HAVE_ISSPACE)
#  define isspace(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\f')
#endif

#if !defined (isprint) && !defined (HAVE_ISPRINT)
#  define isprint(c) (isalpha(c) || isdigit(c) || ispunct(c))
#endif

#if defined (isblank) || defined (HAVE_ISBLANK)
#  define ISBLANK(c) (IN_CTYPE_DOMAIN (c) && isblank (c))
#else
#  define ISBLANK(c) ((c) == ' ' || (c) == '\t')
#endif

#if defined (isgraph) || defined (HAVE_ISGRAPH)
#  define ISGRAPH(c) (IN_CTYPE_DOMAIN (c) && isgraph (c))
#else
#  define ISGRAPH(c) (IN_CTYPE_DOMAIN (c) && isprint (c) && !isspace (c))
#endif

#if !defined (isxdigit) && !defined (HAVE_ISXDIGIT)
#  define isxdigit(c)	(((c) >= '0' && (c) <= '9') || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))
#endif

#undef ISPRINT

#define ISPRINT(c) (IN_CTYPE_DOMAIN (c) && isprint (c))
#define ISDIGIT(c) (IN_CTYPE_DOMAIN (c) && isdigit (c))
#define ISALNUM(c) (IN_CTYPE_DOMAIN (c) && isalnum (c))
#define ISALPHA(c) (IN_CTYPE_DOMAIN (c) && isalpha (c))
#define ISCNTRL(c) (IN_CTYPE_DOMAIN (c) && iscntrl (c))
#define ISLOWER(c) (IN_CTYPE_DOMAIN (c) && islower (c))
#define ISPUNCT(c) (IN_CTYPE_DOMAIN (c) && ispunct (c))
#define ISSPACE(c) (IN_CTYPE_DOMAIN (c) && isspace (c))
#define ISUPPER(c) (IN_CTYPE_DOMAIN (c) && isupper (c))
#define ISXDIGIT(c) (IN_CTYPE_DOMAIN (c) && isxdigit (c))

#define ISLETTER(c)	(ISALPHA(c))

#define TOLOWER(c)	(ISUPPER(c) ? tolower(c) : (c))
#define TOUPPER(c)	(ISLOWER(c) ? toupper(c) : (c))

#define MB_INVALIDCH(x)		((x) == (size_t)-1 || (x) == (size_t)-2)
#define MB_NULLWCH(x)		((x) == 0)

/* is_basic(c) tests whether the single-byte character c is in the
   ISO C "basic character set".
   This is a convenience function, and is in this file only to share code
   between mbiter_multi.h and mbfile_multi.h.  */
#if (' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
    && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
    && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
    && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
    && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
    && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
    && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
    && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
    && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
    && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
    && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
    && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
    && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
    && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
    && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
    && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
    && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
    && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
    && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
    && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
    && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
    && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
    && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126)
/* The character set is ISO-646, not EBCDIC. */

/* Bit table of characters in the ISO C "basic character set".  */
const unsigned int is_basic_table [UCHAR_MAX / 32 + 1] =
{
  0x00001a00,           /* '\t' '\v' '\f' */
  0xffffffef,           /* ' '...'#' '%'...'?' */
  0xfffffffe,           /* 'A'...'Z' '[' '\\' ']' '^' '_' */
  0x7ffffffe            /* 'a'...'z' '{' '|' '}' '~' */
  /* The remaining bits are 0.  */
};

static inline int
is_basic (char c)
{
  return (is_basic_table [(unsigned char) c >> 5] >> ((unsigned char) c & 31))
         & 1;
}

#else
static inline int
is_basic (char c)
{
  switch (c)
    {
    case '\t': case '\v': case '\f':
    case ' ': case '!': case '"': case '#': case '%':
    case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case ':': case ';': case '<': case '=': case '>':
    case '?':
    case 'A': case 'B': case 'C': case 'D': case 'E':
    case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z':
    case '[': case '\\': case ']': case '^': case '_':
    case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o':
    case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~':
      return 1;
    default:
      return 0;
    }
}

#endif /* IS_BASIC_ASCII */

/* First, compile `sm_loop.c' for single-byte characters. */
#define CHAR	unsigned char
#define U_CHAR	unsigned char
#define XCHAR	char
#define INT	int
#define L(CS)	CS
#define INVALID	-1

#undef STREQ
#undef STREQN
#define STREQ(a, b) ((a)[0] == (b)[0] && strcmp(a, b) == 0)
#define STREQN(a, b, n) ((a)[0] == (b)[0] && strncmp(a, b, n) == 0)

#if defined (HAVE_STRCOLL)
/* Helper function for collating symbol equivalence. */
static int rangecmp (c1, c2)
     int c1, c2;
{
  static char s1[2] = { ' ', '\0' };
  static char s2[2] = { ' ', '\0' };
  int ret;

  /* Eight bits only.  Period. */
  c1 &= 0xFF;
  c2 &= 0xFF;

  if (c1 == c2)
    return (0);

  s1[0] = c1;
  s2[0] = c2;

  if ((ret = strcoll (s1, s2)) != 0)
    return ret;
  return (c1 - c2);
}
#else /* !HAVE_STRCOLL */
#  define rangecmp(c1, c2)	((int)(c1) - (int)(c2))
#endif /* !HAVE_STRCOLL */

#if defined (HAVE_STRCOLL)
static int
collequiv (c1, c2)
     int c1, c2;
{
  return (rangecmp (c1, c2) == 0);
}
#else
#  define collequiv(c1, c2)	((c1) == (c2))
#endif

#define _COLLSYM	_collsym
#define __COLLSYM	__collsym
#define POSIXCOLL	posix_collsyms
#include "collsyms.h"

static int
collsym (s, len)
     CHAR *s;
     int len;
{
  register struct _collsym *csp;
  char *x;

  x = (char *)s;
  for (csp = posix_collsyms; csp->name; csp++)
    {
      if (STREQN(csp->name, x, len) && csp->name[len] == '\0')
	return (csp->code);
    }
  if (len == 1)
    return s[0];
  return INVALID;
}

/* unibyte character classification */
#if !defined (isascii) && !defined (HAVE_ISASCII)
#  define isascii(c)	((unsigned int)(c) <= 0177)
#endif

enum char_class
  {
    CC_NO_CLASS = 0,
    CC_ASCII, CC_ALNUM, CC_ALPHA, CC_BLANK, CC_CNTRL, CC_DIGIT, CC_GRAPH,
    CC_LOWER, CC_PRINT, CC_PUNCT, CC_SPACE, CC_UPPER, CC_WORD, CC_XDIGIT
  };

static char const *const cclass_name[] =
  {
    "",
    "ascii", "alnum", "alpha", "blank", "cntrl", "digit", "graph",
    "lower", "print", "punct", "space", "upper", "word", "xdigit"
  };

#define N_CHAR_CLASS (sizeof(cclass_name) / sizeof (cclass_name[0]))

static int
is_cclass (c, name)
     int c;
     const char *name;
{
  enum char_class char_class = CC_NO_CLASS;
  int i, result;

  for (i = 1; i < N_CHAR_CLASS; i++)
    {
      if (STREQ (name, cclass_name[i]))
	{
	  char_class = (enum char_class)i;
	  break;
	}
    }

  if (char_class == 0)
    return -1;

  switch (char_class)
    {
      case CC_ASCII:
	result = isascii (c);
	break;
      case CC_ALNUM:
	result = ISALNUM (c);
	break;
      case CC_ALPHA:
	result = ISALPHA (c);
	break;
      case CC_BLANK:  
	result = ISBLANK (c);
	break;
      case CC_CNTRL:
	result = ISCNTRL (c);
	break;
      case CC_DIGIT:
	result = ISDIGIT (c);
	break;
      case CC_GRAPH:
	result = ISGRAPH (c);
	break;
      case CC_LOWER:
	result = ISLOWER (c);
	break;
      case CC_PRINT: 
	result = ISPRINT (c);
	break;
      case CC_PUNCT:
	result = ISPUNCT (c);
	break;
      case CC_SPACE:
	result = ISSPACE (c);
	break;
      case CC_UPPER:
	result = ISUPPER (c);
	break;
      case CC_WORD:
        result = (ISALNUM (c) || c == '_');
	break;
      case CC_XDIGIT:
	result = ISXDIGIT (c);
	break;
      default:
	result = -1;
	break;
    }

  return result;  
}

/* Now include `sm_loop.c' for single-byte characters. */
/* The result of FOLD is an `unsigned char' */
# define FOLD(c) ((flags & FNM_CASEFOLD) \
	? TOLOWER ((unsigned char)c) \
	: ((unsigned char)c))

#define FCT			internal_strmatch
#define GMATCH			gmatch
#define COLLSYM			collsym
#define PARSE_COLLSYM		parse_collsym
#define BRACKMATCH		brackmatch
#define PATSCAN			patscan
#define STRCOMPARE		strcompare
#define EXTMATCH		extmatch
#define STRCHR(S, C)		strchr((S), (C))
#define STRCOLL(S1, S2)		strcoll((S1), (S2))
#define STRLEN(S)		strlen(S)
#define STRCMP(S1, S2)		strcmp((S1), (S2))
#define RANGECMP(C1, C2)	rangecmp((C1), (C2))
#define COLLEQUIV(C1, C2)	collequiv((C1), (C2))
#define CTYPE_T			enum char_class
#define IS_CCLASS(C, S)		is_cclass((C), (S))
#include "sm_loop.c"

#ifdef HANDLE_MULTIBYTE

#  define CHAR		wchar_t
#  define U_CHAR	wint_t
#  define XCHAR		wchar_t
#  define INT		wint_t
#  define L(CS)		L##CS
#  define INVALID	WEOF

#  undef STREQ
#  undef STREQN
#  define STREQ(s1, s2) ((wcscmp (s1, s2) == 0))
#  define STREQN(a, b, n) ((a)[0] == (b)[0] && wcsncmp(a, b, n) == 0)

static int
rangecmp_wc (c1, c2)
     wint_t c1, c2;
{
  static wchar_t s1[2] = { L' ', L'\0' };
  static wchar_t s2[2] = { L' ', L'\0' };

  if (c1 == c2)
    return 0;

  s1[0] = c1;
  s2[0] = c2;

  return (wcscoll (s1, s2));
}

static int
collequiv_wc (c, equiv)
     wint_t c, equiv;
{
  return (!(c - equiv));
}

/* Helper function for collating symbol. */
#  define _COLLSYM	_collwcsym
#  define __COLLSYM	__collwcsym
#  define POSIXCOLL	posix_collwcsyms
#  include "collsyms.h"

static wint_t
collwcsym (s, len)
     wchar_t *s;
     int len;
{
  register struct _collwcsym *csp;

  for (csp = posix_collwcsyms; csp->name; csp++)
    {
      if (STREQN(csp->name, s, len) && csp->name[len] == L'\0')
	return (csp->code);
    }
  if (len == 1)
    return s[0];
  return INVALID;
}

static int
is_wcclass (wc, name)
     wint_t wc;
     wchar_t *name;
{
  char *mbs;
  mbstate_t state;
  size_t mbslength;
  wctype_t desc;
  int want_word;

  if ((wctype ("ascii") == (wctype_t)0) && (wcscmp (name, L"ascii") == 0))
    {
      int c;

      if ((c = wctob (wc)) == EOF)
	return 0;
      else
        return (c <= 0x7F);
    }

  want_word = (wcscmp (name, L"word") == 0);
  if (want_word)
    name = L"alnum";

  bzero (&state, sizeof (mbstate_t));
  mbs = (char *) malloc (wcslen(name) * MB_CUR_MAX + 1);
  mbslength = wcsrtombs (mbs, (const wchar_t **)&name, (wcslen(name) * MB_CUR_MAX + 1), &state);

  if (mbslength == (size_t)-1 || mbslength == (size_t)-2)
    {
      free (mbs);
      return -1;
    }
  desc = wctype (mbs);
  free (mbs);

  if (desc == (wctype_t)0)
    return -1;

  if (want_word)
    return (iswctype (wc, desc) || wc == L'_');
  else
    return (iswctype (wc, desc));
}

/* Now include `sm_loop.c' for multibyte characters. */
#define FOLD(c) ((flags & FNM_CASEFOLD) && iswupper (c) ? towlower (c) : (c))
#define FCT			internal_wstrmatch
#define GMATCH			gmatch_wc
#define COLLSYM			collwcsym
#define PARSE_COLLSYM		parse_collwcsym
#define BRACKMATCH		brackmatch_wc
#define PATSCAN			patscan_wc
#define STRCOMPARE		wscompare
#define EXTMATCH		extmatch_wc
#define STRCHR(S, C)		wcschr((S), (C))
#define STRCOLL(S1, S2)		wcscoll((S1), (S2))
#define STRLEN(S)		wcslen(S)
#define STRCMP(S1, S2)		wcscmp((S1), (S2))
#define RANGECMP(C1, C2)	rangecmp_wc((C1), (C2))
#define COLLEQUIV(C1, C2)	collequiv_wc((C1), (C2))
#define CTYPE_T			enum char_class
#define IS_CCLASS(C, S)		is_wcclass((C), (S))
#include "sm_loop.c"

/* Return pointer to first multibyte char in S, or NULL if none. */
char *
mbsmbchar (s)
     const char *s;
{
  char *t;
  size_t clen;
  mbstate_t mbs;

  bzero (&mbs, sizeof (mbstate_t));

  for (t = (char *)s; *t; t++)
    {
      if (is_basic (*t))
	continue;

      clen = mbrlen (t, MB_CUR_MAX, &mbs);

      if (clen == 0)
        return 0;
      if (MB_INVALIDCH(clen))
	continue;

      if (clen > 1)
	return t;
    }
  return 0;
}

#if HAVE_MBSNRTOWCS
/* Convert a multibyte string to a wide character string. Memory for the
   new wide character string is obtained with malloc.

   Fast multiple-character version of xdupmbstowcs used when the indices are
   not required and mbsnrtowcs is available. */

static size_t
xdupmbstowcs2 (destp, src)
    wchar_t **destp;	/* Store the pointer to the wide character string */
    const char *src;	/* Multibyte character string */
{
  const char *p;	/* Conversion start position of src */
  wchar_t *wsbuf;	/* Buffer for wide characters. */
  size_t wsbuf_size;	/* Size of WSBUF */
  size_t wcnum;		/* Number of wide characters in WSBUF */
  mbstate_t state;	/* Conversion State */
  size_t wcslength;	/* Number of wide characters produced by the conversion. */
  const char *end_or_backslash;
  size_t nms;	/* Number of multibyte characters to convert at one time. */
  mbstate_t tmp_state;
  const char *tmp_p;

  bzero (&state, sizeof(mbstate_t));

  wsbuf_size = 0;
  wsbuf = NULL;

  p = src;
  wcnum = 0;
  do
    {
      end_or_backslash = strchrnul(p, '\\');
      nms = (end_or_backslash - p);
      if (*end_or_backslash == '\0')
	nms++;

      /* Compute the number of produced wide-characters. */
      tmp_p = p;
      tmp_state = state;
      wcslength = mbsnrtowcs(NULL, &tmp_p, nms, 0, &tmp_state);

      /* Conversion failed. */
      if (wcslength == (size_t)-1)
	{
	  free (wsbuf);
	  *destp = NULL;
	  return (size_t)-1;
	}

      /* Resize the buffer if it is not large enough. */
      if (wsbuf_size < wcnum+wcslength+1)	/* 1 for the L'\0' or the potential L'\\' */
	{
	  wchar_t *wstmp;

	  wsbuf_size = wcnum+wcslength+1;	/* 1 for the L'\0' or the potential L'\\' */

	  wstmp = (wchar_t *) realloc (wsbuf, wsbuf_size * sizeof (wchar_t));
	  if (wstmp == NULL)
	    {
	      free (wsbuf);
	      *destp = NULL;
	      return (size_t)-1;
	    }
	  wsbuf = wstmp;
	}

      /* Perform the conversion. This is assumed to return 'wcslength'.
       * It may set 'p' to NULL. */
      mbsnrtowcs(wsbuf+wcnum, &p, nms, wsbuf_size-wcnum, &state);

      wcnum += wcslength;

      if (mbsinit (&state) && (p != NULL) && (*p == '\\'))
	{
	  wsbuf[wcnum++] = L'\\';
	  p++;
	}
    }
  while (p != NULL);

  *destp = wsbuf;

  /* Return the length of the wide character string, not including `\0'. */
  return wcnum;
}
#endif /* HAVE_MBSNRTOWCS */

/* Convert a multibyte string to a wide character string. Memory for the
   new wide character string is obtained with malloc.

   The return value is the length of the wide character string. Returns a
   pointer to the wide character string in DESTP. If INDICESP is not NULL,
   INDICESP stores the pointer to the pointer array. Each pointer is to
   the first byte of each multibyte character. Memory for the pointer array
   is obtained with malloc, too.
   If conversion is failed, the return value is (size_t)-1 and the values
   of DESTP and INDICESP are NULL. */

#define WSBUF_INC 32

size_t
xdupmbstowcs (destp, indicesp, src)
    wchar_t **destp;	/* Store the pointer to the wide character string */
    char ***indicesp;	/* Store the pointer to the pointer array. */
    const char *src;	/* Multibyte character string */
{
  const char *p;	/* Conversion start position of src */
  wchar_t wc;		/* Created wide character by conversion */
  wchar_t *wsbuf;	/* Buffer for wide characters. */
  char **indices; 	/* Buffer for indices. */
  size_t wsbuf_size;	/* Size of WSBUF */
  size_t wcnum;		/* Number of wide characters in WSBUF */
  mbstate_t state;	/* Conversion State */

  /* In case SRC or DESP is NULL, conversion doesn't take place. */
  if (src == NULL || destp == NULL)
    {
      if (destp)
	*destp = NULL;
      return (size_t)-1;
    }

#if HAVE_MBSNRTOWCS
  if (indicesp == NULL)
    return (xdupmbstowcs2 (destp, src));
#endif

  bzero (&state, sizeof(mbstate_t));
  wsbuf_size = WSBUF_INC;

  wsbuf = (wchar_t *) malloc (wsbuf_size * sizeof(wchar_t));
  if (wsbuf == NULL)
    {
      *destp = NULL;
      return (size_t)-1;
    }

  indices = NULL;
  if (indicesp)
    {
      indices = (char **) malloc (wsbuf_size * sizeof(char *));
      if (indices == NULL)
	{
	  free (wsbuf);
	  *destp = NULL;
	  return (size_t)-1;
	}
    }

  p = src;
  wcnum = 0;
  do
    {
      size_t mblength;	/* Byte length of one multibyte character. */

      if (mbsinit (&state))
	{
	  if (*p == '\0')
	    {
	      wc = L'\0';
	      mblength = 1;
	    }
	  else if (*p == '\\')
	    {
	      wc = L'\\';
	      mblength = 1;
	    }
	  else
	    mblength = mbrtowc(&wc, p, MB_LEN_MAX, &state);
	}
      else
	mblength = mbrtowc(&wc, p, MB_LEN_MAX, &state);

      /* Conversion failed. */
      if (MB_INVALIDCH (mblength))
	{
	  free (wsbuf);
	  FREE (indices);
	  *destp = NULL;
	  return (size_t)-1;
	}

      ++wcnum;

      /* Resize buffers when they are not large enough. */
      if (wsbuf_size < wcnum)
	{
	  wchar_t *wstmp;
	  char **idxtmp;

	  wsbuf_size += WSBUF_INC;

	  wstmp = (wchar_t *) realloc (wsbuf, wsbuf_size * sizeof (wchar_t));
	  if (wstmp == NULL)
	    {
	      free (wsbuf);
	      FREE (indices);
	      *destp = NULL;
	      return (size_t)-1;
	    }
	  wsbuf = wstmp;

	  if (indicesp)
	    {
	      idxtmp = (char **) realloc (indices, wsbuf_size * sizeof (char **));
	      if (idxtmp == NULL)
		{
		  free (wsbuf);
		  free (indices);
		  *destp = NULL;
		  return (size_t)-1;
		}
	      indices = idxtmp;
	    }
	}

      wsbuf[wcnum - 1] = wc;
      if (indices)
        indices[wcnum - 1] = (char *)p;
      p += mblength;
    }
  while (MB_NULLWCH (wc) == 0);

  /* Return the length of the wide character string, not including `\0'. */
  *destp = wsbuf;
  if (indicesp != NULL)
    *indicesp = indices;

  return (wcnum - 1);
}
#endif /* HANDLE_MULTIBYTE */

/* Now we're finally ready to do some pattern matching
**
**	returns	0, if match
**		1, if no match
*/
int
match (mask, name)
     char *mask;
     char *name;
{
  int flags = FNM_CASEFOLD;
  char *patt = mask;
#ifdef HANDLE_MULTIBYTE
  int ret;
  size_t n;
  wchar_t *wpattern, *wstring;
#endif

  /* handle extended patterns */
  if (*mask == '&')
  {
    patt++;
    flags |= FNM_RNGMATCH;
  }

#ifdef HANDLE_MULTIBYTE
  if (mbsmbchar (name) == 0 && mbsmbchar (patt) == 0)
    return (internal_strmatch ((unsigned char *)patt, (unsigned char *)name, flags));

  n = xdupmbstowcs (&wpattern, NULL, patt);
  if (n == (size_t)-1 || n == (size_t)-2)
    return (internal_strmatch ((unsigned char *)patt, (unsigned char *)name, flags));

  n = xdupmbstowcs (&wstring, NULL, name);
  if (n == (size_t)-1 || n == (size_t)-2)
    {
      free (wpattern);
      return (internal_strmatch ((unsigned char *)patt, (unsigned char *)name, flags));
    }

  ret = internal_wstrmatch (wpattern, wstring, flags);

  free (wpattern);
  free (wstring);

  return ret;
#else
  return (internal_strmatch ((unsigned char *)patt, (unsigned char *)name, flags));
#endif /* !HANDLE_MULTIBYTE */
}

/* End of GNU bash code */

/*
** collapse a pattern string into minimal components.
** This particular version is "in place", so that it changes the pattern
** which is to be reduced to a "minimal" size.
*/
char	*collapse(pattern)
char	*pattern;
{
	Reg	char	*s = pattern, *s1, *t;

	if (BadPtr(pattern))
		return pattern;
	/*
	 * Collapse all \** into \*, \*[?]+\** into \*[?]+
	 */
	for (; *s; s++)
		if (*s == '\\')
			if (!*(s + 1))
				break;
			else
				s++;
		else if (*s == '*')
		    {
			if (*(t = s1 = s + 1) == '*')
				while (*t == '*')
					t++;
			else if (*t == '?')
				for (t++, s1++; *t == '*' || *t == '?'; t++)
					if (*t == '?')
						*s1++ = *t;
			while ((*s1++ = *t++))
				;
		    }
	return pattern;
}

/*
** Performs case-insensitive comparison for two null-terminated strings
**
**     returns  0, if s1 equal to s2
**             <0, if s1 lexicographically less than s2
**             >0, if s1 lexicographically greater than s2
*/

int mycmp(s1, s2)
const char    *s1;
const char    *s2;
    {
        Reg     char   *str1 = (char *)s1;
        Reg     char   *str2 = (char *)s2;
        Reg     int     res;

        while ((res = toupper(*str1) - toupper(*str2)) == 0)
            {
                if (*str1 == '\0')
                        return 0;
                str1++;
                str2++;
            }
        return (res);
}

int myncmp(str1, str2, n)
const char    *str1;
const char    *str2;
int     n;
    {
        Reg     char   *s1 = (char *)str1;
        Reg     char   *s2 = (char *)str2;
        Reg     int             res;

        while ((res = toupper(*s1) - toupper(*s2)) == 0)
            {
                s1++; s2++; n--;
                if (n == 0 || (*s1 == '\0' && *s2 == '\0'))
                        return 0;
            }
        return (res);
}
#if defined(USE_OLD8BIT) || defined(LOCALE_STRICT_NAMES)

void setup_validtab (void)
{
  int i;

  for (i = 0; i < 256; i++)
  {
    if ((i >= 'A' && i <= '~') || i == '-' ||
#ifndef USE_OLD8BIT
	isalnum(i))
#else
	i >= 0xc0 || i == 0xad || i == 0xbd ||
	(i >= 0xa3 && i <= 0xa7) || (i >= 0xb3 && i <= 0xb7) || isdigit(i))
#endif
      validtab[i] = i;
    else
      validtab[i] = 0;
  }
}
#endif
