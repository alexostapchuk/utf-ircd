/************************************************************************
 *   IRC - Internet Relay Chat, src/ssl.c
 *   Copyright (C) RusNet development
 *
 *   Some pieces of code were adopted from:
 *   stunnel:  Michal Trojnara  <Michal.Trojnara@mirt.net>
 *   azzurranet patch for bahamut: Barnaba Marcello <vjt@azzurra.org>
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
 *   $Id: ssl.c,v 1.7 2005/12/20 13:20:02 cj Exp $  
 */

#include "os.h"
#include "s_defines.h"
#define SSL_C
#include "s_externs.h"
#undef SSL_C

#ifdef USE_SSL

#define SSL_READ	1
#define SSL_WRITE	2
#define SSL_ACCEPT	3

static int ssl_fatal(int, int, aClient *);
extern int errno;
SSL_CTX *ctx;
int sslable = 0;

static void disable_ssl(int do_errors)
{
    if(do_errors)
    {
	char buf[256];
	unsigned long e;
	while((e = ERR_get_error()))
	{
	    ERR_error_string_n(e, buf, sizeof(buf) - 1);
	    Debug((DEBUG_DEBUG, "%s", buf));
	    if (serverbooting) {
		(void)fprintf(stderr, "%s\n", buf);
	    }
	}
    }
   if (sslable) {
	if ((bootopt & BOOT_TTY) && (bootopt & BOOT_DEBUG))
	    (void)fprintf(stderr, "disable_ssl(): EVP_cleanup()\n");
	EVP_cleanup();
    }

    if(ctx) {
	if ((bootopt & BOOT_TTY) && (bootopt & BOOT_DEBUG))
	    (void)fprintf(stderr, "disable_ssl(): SSL_CTX_free()\n");    
	SSL_CTX_free(ctx);
    }
    Debug((DEBUG_DEBUG, "SSL support is disabled."));
    sslable = 0;
    return;
}

static int add_rand_file(char *filename)
{
    int readbytes;
    int writebytes;

    if(readbytes = RAND_load_file(filename, 255))
    {
        Debug((DEBUG_DEBUG, "Got 255 random bytes from %s", filename));
    }
    else
    {
        Debug((DEBUG_NOTICE, "Unable to retrieve any random data from %s", filename));
    }
#ifdef WRITE_SEED_DATA
    writebytes = RAND_write_file(WRITE_SEED_DATA);
    if(writebytes == -1)
        Debug((DEBUG_ERROR, "Failed to write strong random data to %s - "
            "may be a permissions or seeding problem", WRITE_SEED_DATA));
    else
        Debug((DEBUG_DEBUG, "Wrote %d new random bytes to %s", writebytes, WRITE_SEED_DATA));
#endif /* WRITE_SEED_DATA */
    return readbytes;
}

static int prng_seeded(int bytes)
{
#if SSLEAY_VERSION_NUMBER >= 0x0090581fL
    if(RAND_status()){
        Debug((DEBUG_DEBUG, "RAND_status claims sufficient entropy for the PRNG"));
        return 1;
    }
#else
    if(bytes >= 255) {
        Debug((DEBUG_NOTICE, "Sufficient entropy in PRNG assumed (>= 255)"));
        return 1;
    }
#endif
    return 0;        /* We don't have enough bytes */
}

static int prng_init(void)
{
    int totbytes = 0;
    int bytes = 0;

#if SSLEAY_VERSION_NUMBER >= 0x0090581fL
#ifdef EGD_SOCKET
    if((bytes = RAND_egd(EGD_SOCKET)) == -1) {
        Debug((DEBUG_ERROR, "EGD Socket %s failed", EGD_SOCKET));
	bytes = 0;
    } else {
        totbytes += bytes;
        Debug((DEBUG_DEBUG, "Got %d random bytes from EGD Socket %s",
            bytes, EGD_SOCKET));
        return 0;
    }
#endif /* EGD_SOCKET */
#endif /* OpenSSL-0.9.5a */
#ifdef RANDOM_FILE
    /* Try RANDOM_FILE if available */
    totbytes += add_rand_file(RANDOM_FILE);
    if(prng_seeded(totbytes))
        return 0;
#endif
    Debug((DEBUG_NOTICE, "PRNG seeded with %d bytes total", totbytes));
    Debug((DEBUG_ERROR, "PRNG may not have been seeded with enough random bytes"));
    return -1; /* FAILED but we will deal with it*/
}


int ssl_init(void)
{
    if (!prng_init()) {
	Debug((DEBUG_NOTICE, "PRNG seeded"));
    }
    if ((bootopt & BOOT_TTY) && (bootopt & BOOT_DEBUG))
	    (void)fprintf(stderr, "ssl_init(): SSL_load_error_strings()\n" );
    SSL_load_error_strings();
    if ((bootopt & BOOT_TTY) && (bootopt & BOOT_DEBUG))
	    (void)fprintf(stderr, "ssl_init(): OpenSSL_add_ssl_algorythms()\n");
    OpenSSL_add_ssl_algorithms();
    if ((bootopt & BOOT_TTY) && (bootopt & BOOT_DEBUG))
	    (void)fprintf(stderr, "ssl_init(): SSL_CTX_new()\n");
    ctx = SSL_CTX_new(SSLv23_server_method());
    if (!ctx) {
	Debug((DEBUG_ERROR, "CTX_new: %s", ERR_error_string(ERR_get_error(), NULL)));
	return 0;
    }
    if ((bootopt & BOOT_TTY) && (bootopt & BOOT_DEBUG))
	    (void)fprintf(stderr, "ssl_init(): SSL_CTX_use_certificate_file()\n");
    if (SSL_CTX_use_certificate_file(ctx,
		IRCD_CRT, SSL_FILETYPE_PEM) <= 0) {
	(void)disable_ssl(1);
	return 0;
    }
    if ((bootopt & BOOT_TTY) && (bootopt & BOOT_DEBUG))
	    (void)fprintf(stderr, "ssl_init(): SSL_CTX_use_PrivateKey_file()\n");
    if (SSL_CTX_use_PrivateKey_file(ctx,
		IRCD_KEY, SSL_FILETYPE_PEM) <= 0) {
	(void)disable_ssl(1);
	return 0;
    }
    if ((bootopt & BOOT_TTY) && (bootopt & BOOT_DEBUG))
	    (void)fprintf(stderr, "ssl_init(): SSL_CTX_check_private_key()\n");    
    if (!SSL_CTX_check_private_key(ctx)) {
	(void)disable_ssl(1);
	return 0;
    }
    return 1;
}



int ssl_rehash(void)
{
    if (sslable)
	disable_ssl(0);
    sslable = ssl_init();
}



int ssl_read(aClient *acptr, void *buf, int sz)
{
    int len, ssl_err;

    len = SSL_read(acptr->ssl, buf, sz);
    if (len <= 0)
    {
	switch(ssl_err = SSL_get_error(acptr->ssl, len)) {
	    case SSL_ERROR_SYSCALL:
		if (errno == EWOULDBLOCK || errno == EAGAIN ||
			errno == EINTR) {
	    case SSL_ERROR_WANT_WRITE:
	    case SSL_ERROR_WANT_READ:
		    errno = EWOULDBLOCK;
		    return 0;
		}
	    case SSL_ERROR_SSL:
		if(errno == EAGAIN)
		    return 0;
	    default:
		return ssl_fatal(ssl_err, SSL_READ, acptr);
	}
    }
    return len;
}

int ssl_write(aClient *acptr, const void *buf, int sz)
{
    int len, ssl_err;

    len = SSL_write(acptr->ssl, buf, sz);
    if (len <= 0)
    {
	switch(ssl_err = SSL_get_error(acptr->ssl, len)) {
	    case SSL_ERROR_SYSCALL:
		if (errno == EWOULDBLOCK || errno == EAGAIN ||
			errno == EINTR) {
	    case SSL_ERROR_WANT_WRITE:
	    case SSL_ERROR_WANT_READ:
		    errno = EWOULDBLOCK;
		    return 0;
		}
	    case SSL_ERROR_SSL:
		if(errno == EAGAIN)
		    return 0;
	    default:
		return ssl_fatal(ssl_err, SSL_WRITE, acptr);
	}
    }
    return len;
}

int ssl_accept(aClient *acptr, int fd)
{

    int ssl_err;
    if((ssl_err = SSL_accept(acptr->ssl)) <= 0) {
	switch(ssl_err = SSL_get_error(acptr->ssl, ssl_err)) {
	    case SSL_ERROR_SYSCALL:
		if (errno == EINTR || errno == EWOULDBLOCK
			|| errno == EAGAIN)
	    case SSL_ERROR_WANT_READ:
	    case SSL_ERROR_WANT_WRITE:
		    /* handshake is postponed? */
		    return 1;
	    default:
		return ssl_fatal(ssl_err, SSL_ACCEPT, acptr);
	}
	/* Never get here */
	return -1;
    }
    return 1;
}

int ssl_shutdown(struct SSL *ssl) 
{
    int i;
    int retval = 0;

    for(i = 0; i < 4; i++) { /* I don't know if this enough for bi-directional shutdown,
			      * but it should be ;) --skold
			      */
	if((retval = SSL_shutdown(ssl)))
	    break;
    }
    return retval;
}

static int ssl_fatal(int ssl_error, int where, aClient *sptr)
{
    int errtmp = errno;
    char *errstr = strerror(errtmp);
    char *ssl_errstr, *ssl_func;
#ifdef DEBUGMODE
    char buf[256];
    unsigned long ssl_err_queue = 0;
#endif
    switch(where) {
	case SSL_READ:
	    ssl_func = "SSL_read()";
	    break;
	case SSL_WRITE:
	    ssl_func = "SSL_write()";
	    break;
	case SSL_ACCEPT:
	    ssl_func = "SSL_accept()";
	    break;
	default:
	    ssl_func = "SSL_UNKNOWN()";
    }

    switch(ssl_error) {
    	case SSL_ERROR_NONE:
	    ssl_errstr = "SSL operation completed";
	    break;
	case SSL_ERROR_SSL:
	    ssl_errstr = "SSL internal error";
	    break;
	case SSL_ERROR_WANT_READ:
	    ssl_errstr = "SSL read failed";
	    break;
	case SSL_ERROR_WANT_WRITE:
	    ssl_errstr = "SSL write failed";
	    break;
	case SSL_ERROR_WANT_X509_LOOKUP:
	    ssl_errstr = "SSL X509 lookup failed";
	    break;
	case SSL_ERROR_SYSCALL:
	    ssl_errstr = "SSL I/O error";
	    break;
	case SSL_ERROR_ZERO_RETURN:
	    ssl_errstr = "SSL session closed";
	    break;
	case SSL_ERROR_WANT_CONNECT:
	    ssl_errstr = "SSL connect failed";
	    break;
	default:
	    ssl_errstr = "SSL protocol error";
    }

    Debug((DEBUG_DEBUG, "%s to "
		"%s!%s@%s aborted with%serror (%s). [%s]", 
		ssl_func, *sptr->name ? sptr->name : "<unknown>",
		(sptr->user && sptr->user->username) ? sptr->user->
		username : "<unregistered>", sptr->sockhost,
		(errno > 0) ? " " : " no ", errstr, ssl_errstr));
#ifdef DEBUGMODE
	while((ssl_err_queue = ERR_get_error()))
	{
	    ERR_error_string_n(ssl_err_queue, buf, sizeof(buf) - 1);
	    Debug((DEBUG_DEBUG, "SSL error stack: %s", buf));
	}
#endif
    
#ifdef USE_SYSLOG
    syslog(LOG_ERR, "SSL error in %s: %s [%s]", ssl_func, errstr,
	    ssl_errstr);
#endif /* USE_SYSLOG */
    /*
     * Keep silence here. No error messages or we fall into the loop here
     */
    if (ssl_error) {
	strerror_ssl = ssl_errstr;
    } else {
	strerror_ssl = NULL;
    }
    errno = errtmp ? errtmp : EIO;
    //sptr->flags |= FLAGS_DEADSOCKET;
    ERR_clear_error();
    return -1;
}

/*
 * We should use this fine recursive function in the future
 */
static void sslerror_stack(void) {
    unsigned long err;
    char string[120];

    err=ERR_get_error();
    if(!err)
        return;
    sslerror_stack();
    ERR_error_string(err, string);
    Debug((DEBUG_DEBUG, "error stack: %lX : %s", err, string));
}

#endif /* USE_SSL */
