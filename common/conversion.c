/************************************************************************
 *   IRC - Internet Relay Chat, common/common_def.h
 *   Copyright (C) 2005 Rusnet
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
#include "s_externs.h"
 
#if defined(__GNUC__) && !defined(SOLARIS_10UP)
#define _ICONV_CHAR_	(char ** __restrict__)
#else	/* __GNUC__ */
#define _ICONV_CHAR_	(const char **)
#endif	/* __GNUC__ */

static conversion_t *Conversions = NULL;
static conversion_t *internal = NULL;

/*
 * find or allocate conversion structure for given charset
 * returns pointer to found structure or NULL if charset isn't handled
*/
conversion_t *conv_get_conversion(const char *charset)
{
  conversion_t *conv;
  iconv_t cd;
  char name[40]; /* assume charset's name is at most 21 char long */

  if (charset == NULL) /* for me assume it's internal charset */
    charset = internal->charset;
  else /* scan Conversions tree */
    for (conv = Conversions; conv; conv = conv->next)
      if (!strcasecmp(conv->charset, charset))
      {
	conv->inuse++;
	return conv;
      }
  /* try charset */
  if (internal && !strcasecmp(charset, internal->charset))
    cd = NULL; /* if it's internal charset then don't open descriptor */
  else if (internal)
  {
#ifdef DO_TEXT_REPLACE
# ifdef HAVE_ICONV_TRANSLIT
	snprintf(name, sizeof(name), "%s//TRANSLIT", internal->charset);
# else
	strncpy(name, internal->charset, sizeof(name));
# endif
#else
# ifdef HAVE_ICONV_TRANSLIT
	snprintf(name, sizeof(name), "%s" TRANSLIT_IGNORE, internal->charset);
# else
	snprintf(name, sizeof(name), "%s//IGNORE", internal->charset);
# endif
#endif

	cd = iconv_open(name, charset);
  }
  else /* internal isn't set yet so check only */
    cd = iconv_open(charset, "ascii");
  if (cd == (iconv_t)(-1))
    return NULL;
  if (!internal) /* checked ok */
  {
    iconv_close(cd);
    cd = NULL;
  }
  conv = (void *)MyMalloc(sizeof(conversion_t));
  conv->next = Conversions;
  conv->prev = NULL;
  if (conv->next)
    conv->next->prev = conv;
  Conversions = conv;
  conv->inuse = 1;
  conv->charset = mystrdup(charset);
  conv->cdin = cd;
  if (cd == NULL) /* if no cdin then don't open descriptor cdout too */
    conv->cdout = NULL;
  else
  {
#ifdef DO_TEXT_REPLACE
    snprintf(name, sizeof(name), "%s//TRANSLIT", charset);
#else
    snprintf(name, sizeof(name), "%s" TRANSLIT_IGNORE, charset);
#endif
    conv->cdout = iconv_open(name, internal->charset);
  }
  return conv;
}

/*
 * frees conversion structure if it's used nowhere else
 * returns nothing
*/
void conv_free_conversion(conversion_t *conv)
{
  if (conv == NULL) return;
  conv->inuse--;
  if (conv->inuse == 0 && strcasecmp(conv->charset, CHARSET_8BIT))
  {
    MyFree(conv->charset);
    if (conv->cdin) iconv_close(conv->cdin);
    if (conv->cdout) iconv_close(conv->cdout);
    if (conv->prev)
      conv->prev->next = conv->next;
    if (conv->next)
      conv->next->prev = conv->prev;
    if (conv == Conversions)
      Conversions = conv->next;
    MyFree(conv);
  }
}

/*
 * sets internal charset for conversions
 * returns conversion_t* if success, NULL if no charset with that name exist
*/
conversion_t *conv_set_internal(conversion_t **old, const char *charset)
{
  iconv_t cd;
  conversion_t *conv, *ic;
  char name[40]; /* assume charset's name is at most 21 char long */
  char iname[40];

  /* test if charset name is ok */
  if (!charset || !*charset ||
	(cd = iconv_open(charset, "ascii")) == (iconv_t)(-1))
    return NULL; /* charset doesn't exist! */
  iconv_close(cd);
  if (internal && !strcasecmp(charset, internal->charset))
    return internal; /* wasn't changed so nothing to do */
  /* reset all already opened descriptors */
#ifdef DO_TEXT_REPLACE
  snprintf(iname, sizeof(iname), "%s//TRANSLIT", charset);
#else
  snprintf(iname, sizeof(iname), "%s" TRANSLIT_IGNORE, charset);
#endif
  ic = NULL;
  if (old)
    *old = internal; /* it's about to be changed so keep it */
  for (conv = Conversions; conv; conv = conv->next)
  {
    if (conv->cdin) iconv_close(conv->cdin); /* close current descriptors */
    if (conv->cdout) iconv_close(conv->cdout);
    if (conv == internal)
      conv->inuse--; /* don't free it - it may be reopened just after that */
    if (ic == NULL && !strcasecmp(conv->charset, charset)) /* new internal */
    {
      ic = conv;
      conv->inuse++;
      conv->cdin = conv->cdout = NULL;
    }
    else
    {
#ifdef DO_TEXT_REPLACE
      snprintf(name, sizeof(name), "%s//TRANSLIT", conv->charset);
#else
      snprintf(name, sizeof(name), "%s" TRANSLIT_IGNORE, conv->charset);
#endif
      conv->cdin = iconv_open(iname, conv->charset);
      conv->cdout = iconv_open(name, charset);
    }
  }
  if (ic == NULL)
  {
    conv = conv_get_conversion(charset);
    internal = conv;
  }
  else
    internal = ic;
  return internal;
}

/*
 * do conversion from some charset to another one via iconv descriptor cd
 * given string line with size sl will be converted to buffer buf with
 * max size sz
 * returns size of converted string
*/
size_t conv_do_conv(iconv_t cd, char *line, size_t sl,
		    char **buf, size_t sz)
{
  char *sbuf;
#ifdef DO_TEXT_REPLACE
  int seq = 0;
#endif

  /* conversion unavailable, return source line intact */
  if (cd == NULL)
  {
    *buf = line;
    return (sl > sz) ? sz : sl; /* input line is too long? */
  }
  sbuf = *buf;
#ifdef DO_TEXT_REPLACE
  while (sl && sz)
  {
    size_t last = sz;

    if (iconv(cd, _ICONV_CHAR_ &line, &sl, &sbuf, &sz) != (size_t)(-1)
	|| errno != EILSEQ) /* success or unrecoverable error */
      break;
    /* replace char */
    if (seq == 0 || last != sz) /* some text was converted */
    {
      *sbuf++ = TEXT_REPLACE_CHAR;
      sz--;
      seq = 1;
    }
    else if (seq == 3) /* at most 4 byte sequence with one TEXT_REPLACE_CHAR */
      seq = 0;
    else
      seq++;
    line++;
    sl--;
  }
#else
  if (iconv(cd, _ICONV_CHAR_ &line, &sl, &sbuf, &sz) == (size_t)(-1))
  {
    Debug((DEBUG_NOTICE, "conversion error: %lu chars left unconverted", sz));
  }
#endif
  return (sbuf - *buf);
}

/*
 * sends list of all charsets and use of it, including listen ports
 * it's answer for /stats e command
*/
void conv_report(aClient *sptr, char *to)
{
  conversion_t *conv;

  if (!internal)
    return; /* it's impossible, I think! --LoSt */
  sendto_one(sptr, ":%s %d %s :Internal charset: %s",
	     ME, RPL_STATSDEBUG, to, internal->charset);
  for (conv = Conversions; conv; conv = conv->next)
    sendto_one(sptr, ":%s %d %s :Charset %s: %d",
	       ME, RPL_STATSDEBUG, to, conv->charset, conv->inuse);
}

/*
 * converts null-terminated string src to upper case string
 * output buffer dst with size ds must be enough for null-terminated string
 * else string will be truncated
 * returns length of output null-terminated string (with null char)
 * if dst == NULL or ds == 0 then returns 0
*/
size_t rfcstrtoupper(char *dst, char *src, size_t ds)
{
    size_t sout = 0, ss;

    if (dst == NULL || ds == 0)
	return 0;
    ds--;
    if (src && *src)
    {
	if (Force8bit) /* if string needs conversion to 8bit charset */
	{
	    conversion_t *conv;
	    size_t processed, rest;
	    char *ch;
	    char buf[100]; /* hope it's enough for one pass ;) */

	    ss = strlen(src);
	    conv = conv_get_conversion(CHARSET_8BIT); /* it must be already created --LoSt */
	    while (ss && ds)
	    {
		ch = buf;
		rest = sizeof(buf);
		/* use iconv() since conv_* doesn't do that we want */
		iconv(conv->cdout, _ICONV_CHAR_ &src, &ss, &ch, &rest);
		processed = ch - buf;
		for (ch = buf; ch < &buf[processed]; ch++)
		    *ch = toupper(*ch);
		rest = ds;
		ch = buf;
		iconv(conv->cdin, _ICONV_CHAR_ &ch, &processed, &dst, &ds);
		sout += (rest - ds);
		/* there still may be unconvertable chars in buffer! */
		if (rest == ds)
		    break;
	    }
	    conv_free_conversion(conv);
	}
	else
#ifdef IRCD_CHANGE_LOCALE
	if (UseUnicode && MB_CUR_MAX > 1) /* if multibyte encoding */
	{
	    wchar_t wc;
	    int len;
	    register char *ch;
	    char c[MB_LEN_MAX];

	    for (ch = src, ss = strlen(ch); *ch; )
	    {
		len = mbtowc(&wc, ch, ss);
		if (len < 1)
		{
		    ss--;
		    if ((--ds) == 0)
			break; /* no room for it? */
		    *dst++ = *ch++;
		    sout++;
		    mbtowc(&wc, NULL, 0); /* reset shift state after error */
		    continue;
		}
		ss -= len;
		ch += len;
		wc = towupper(wc);
		len = wctomb(c, wc); /* first get the size of lowercase mbchar */
		if (len < 1)
		    continue; /* tolower() returned unknown char? ignore it */
		if ((size_t)len >= ds)
		    break; /* oops, out of output size! */
		memcpy(dst, c, len); /* really convert it */
		ds -= len; /* it's at least 1 now */
		dst += len;
		sout += len;
	    }
	}
	else
#endif
	{ /* string in internal single-byte encoding the same as locale */
	    register char ch;

	    for (ch = *src++; ch && ds; ch = *src++, sout++, ds--)
		*dst++ = toupper(ch);
	}
    }
    *dst = 0;
    return (sout + 1);
}
