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


#define	MAX_ITERATIONS	512
/*
**  Compare if a given string (name) matches the given
**  mask (which can contain wild cards: '*' - match any
**  number of chars, '?' - match any single character.
**
**	return	0, if match
**		1, if no match
*/

/*
** match()
** Iterative matching function, rather than recursive.
** Written by Douglas A Lewis (dalewis@acsu.buffalo.edu)
*/

int	match(mask, name)
char	*mask, *name;
{
	Reg	u_char	*m = (u_char *)mask, *n = (u_char *)name;
	char	*ma = mask, *na = name;
	int	wild = 0, q = 0, calls = 0;

	if (!*mask)
		return 1;

	while (1)
	    {
#ifdef	MAX_ITERATIONS
	        if (calls++ > MAX_ITERATIONS)
			break;               
#endif
		if ((m > (u_char *)mask) && (*m == '=') && (m[-1] == '@'))
		    m++;

		if (*m == '*')
		   {
			while (*m == '*')
				m++;
			wild = 1;
			ma = (char *)m;
			na = (char *)n;
		    }

		if (!*m)
		    {
	  		if (!*n)
				return 0;
	  		for (m--; (m > (u_char *)mask) && (*m == '?'); m--)
				;
			if ((m > (u_char *)mask) && (*m == '*') &&
			    (m[-1] != '\\'))
				return 0;
			if (!wild) 
				return 1;
			m = (u_char *)ma;
			n = (u_char *)++na;
		    }
		else if (!*n)
			return 1;
		if ((*m == '\\') && (m[1] == '\\'))	/* handle backslash */
		    {
			m++;
			q = 0;
		    }
		else if ((*m == '\\') && ((m == (u_char *)mask) ||
			 (m[-1] != '\\')) && ((m[1] == '*') || (m[1] == '?')))
		    {
			m++;
			q = 1;
		    }
		else
			q = 0;

		if ((tolower(*m) != tolower(*n)) && ((*m != '?') || q))
		    {
			if (!wild)
				return 1;
			m = (u_char *)ma;
			n = (u_char *)++na;
		    }
		else
		    {
			if (*n)
			{
#if !defined(CLIENT_COMPILE) && !defined(USE_OLD8BIT)
			    /* works for utf* only!!! */
			    if (UseUnicode > 0 && *m == '?')
			    {
				if ((*n++ & 0xc0) == 0xc0) /* first multibyte */
				    while ((*n & 0xc0) == 0x80)
					n++;	/* skip rest of octets */
			    }
			    else
#endif
				n++;
			}
			if (*m)
				m++;
		    }
	    }

	return 1;
}


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
char    *s1;
char    *s2;
    {
        Reg     unsigned char   *str1 = (unsigned char *)s1;
        Reg     unsigned char   *str2 = (unsigned char *)s2;
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
char    *str1;
char    *str2;
int     n;
    {
        Reg     unsigned char   *s1 = (unsigned char *)str1;
        Reg     unsigned char   *s2 = (unsigned char *)str2;
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
