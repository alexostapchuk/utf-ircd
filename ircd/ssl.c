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

#ifdef DEBUGMODE
#define	_D_UNUSED_
#else
#define	_D_UNUSED_	_UNUSED_
#endif

#if SSLEAY_VERSION_NUMBER >= 0x0090581fL
#define	_B_UNUSED_	_UNUSED_
#else
#define	_B_UNUSED_
#endif

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
#ifdef WRITE_SEED_DATA
    int writebytes;
#endif

    if ((readbytes = RAND_load_file(filename, 255)))
    {
        Debug((DEBUG_DEBUG, "Got 255 random bytes from %s", filename));
    }
    else
    {
        Debug((DEBUG_NOTICE,
		"Unable to retrieve any random data from %s", filename));
    }
#ifdef WRITE_SEED_DATA
    writebytes = RAND_write_file(WRITE_SEED_DATA);
    if (writebytes == -1)
    {
        Debug((DEBUG_ERROR, "Failed to write strong random data to %s - "
            "may be a permissions or seeding problem", WRITE_SEED_DATA));
    }
    else
    {
        Debug((DEBUG_DEBUG, "Wrote %d new random bytes to %s",
				writebytes, WRITE_SEED_DATA));
    }
#endif /* WRITE_SEED_DATA */
    return readbytes;
}

static int prng_seeded(int bytes _B_UNUSED_)
{
#if SSLEAY_VERSION_NUMBER >= 0x0090581fL
    if (RAND_status())
    {
        Debug((DEBUG_DEBUG,
		"RAND_status claims sufficient entropy for the PRNG"));
        return 1;
    }
#else
    if (bytes >= 255)
    {
        Debug((DEBUG_NOTICE, "Sufficient entropy in PRNG assumed (>= 255)"));
        return 1;
    }
#endif
    return 0;        /* We don't have enough bytes */
}

static int prng_init(void)
{
    int totbytes = 0;
#if SSLEAY_VERSION_NUMBER >= 0x0090581fL
#ifdef EGD_SOCKET
    int bytes = 0;

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
    Debug((DEBUG_ERROR,
		"PRNG may not have been seeded with enough random bytes"));
    return -1; /* FAILED but we will deal with it*/
}

static DH* load_dh_params_2048(void)
{
    static unsigned char dh2048_p[]={
        0x88,0xE4,0x69,0x80,0x20,0x8E,0xE0,0xB4,0xE6,0x7B,0x0A,0x2A,
        0x99,0x53,0x06,0xBB,0x29,0x39,0x26,0x9E,0xF8,0xD0,0x63,0x02,
        0xF0,0xE8,0x19,0x3C,0x47,0x5F,0xE7,0x26,0xDD,0xB5,0x37,0xBC,
        0x05,0x6C,0x7E,0x52,0xDF,0x04,0xA1,0x30,0xAF,0xC8,0x0C,0x07,
        0xD6,0x39,0xBF,0xC2,0xAE,0xC8,0x7D,0x3F,0x9E,0x19,0xA8,0x42,
        0xC5,0x35,0x6F,0xF1,0x57,0x8F,0x2D,0x2F,0x5A,0x36,0xB4,0x23,
        0x03,0xAE,0xC7,0x85,0xEE,0x96,0xB5,0x7A,0x29,0x8C,0x67,0xB2,
        0x6F,0x03,0x00,0xA7,0x16,0x02,0xA7,0xDF,0x23,0x23,0x82,0xA5,
        0x91,0x0C,0x82,0x69,0x6C,0x44,0xC9,0x28,0x86,0xAC,0x36,0x8A,
        0xEA,0x12,0xE2,0x47,0x74,0xA8,0x1A,0x9E,0xB8,0x4C,0x00,0x68,
        0xCD,0xA3,0xF6,0x33,0xF1,0x88,0xDB,0x90,0xB2,0x22,0x29,0x63,
        0xD4,0x67,0x06,0x17,0x29,0x21,0x93,0x4F,0xB8,0x41,0xCC,0x40,
        0xE7,0x3D,0x46,0x90,0x4E,0x5A,0x5A,0x50,0xF7,0x88,0xE5,0xB7,
        0x79,0x42,0xD8,0xA0,0x94,0x58,0xC2,0x50,0xBB,0x9A,0xD0,0x31,
        0xDE,0xA1,0x2A,0xF5,0x8E,0x5F,0x5B,0x0C,0x2D,0xE1,0x1B,0x0E,
        0x8A,0xCA,0x43,0xE8,0x43,0x40,0x6B,0x45,0x45,0xBD,0x64,0xB3,
        0x16,0x60,0xE9,0xA8,0x03,0xED,0x7E,0xF2,0xE0,0x0B,0x47,0x8F,
        0x8D,0x73,0x44,0x25,0x17,0x81,0x10,0x7C,0x3D,0xF0,0x99,0x16,
        0x66,0x05,0x66,0x12,0x7B,0xD7,0xE4,0xF4,0x26,0x73,0xC0,0x37,
        0x25,0x61,0x3B,0xEE,0x12,0xD9,0xBC,0x72,0x34,0x8E,0xE5,0xDF,
        0xAF,0x6C,0x1D,0x37,0x8B,0x36,0x6C,0xD8,0xD1,0xA2,0x3D,0xD7,
        0xE0,0xE4,0xC7,0xA3,
        };
    static unsigned char dh2048_g[]={
        0x02,
        };

    DH *dh = DH_new();
    if (NULL == dh) return NULL;

    dh->p = BN_bin2bn(dh2048_p, sizeof(dh2048_p), NULL);
    dh->g = BN_bin2bn(dh2048_g, sizeof(dh2048_g), NULL);

    if (NULL == dh->p || NULL == dh->g) {
        DH_free(dh);
        return NULL;
    }

    return dh;
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

    EC_KEY  *ecdh;
    DH      *dh;
    
    ctx = SSL_CTX_new(SSLv23_server_method());

    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3);

    dh = load_dh_params_2048();

    if (dh == NULL) {
        (void)fprintf(stderr, "ssl_init(): Failed to get DH params\n");
        SSL_CTX_free(ctx);
        return NULL;
    }

    SSL_CTX_set_tmp_dh(ctx, dh);
    DH_free(dh);

    ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (ecdh == NULL) {
        (void)fprintf(stderr, "ssl_init(): Failed to get EC curve\n");
        SSL_CTX_free(ctx);
        return NULL;
    }

    SSL_CTX_set_tmp_ecdh(ctx, ecdh);
    EC_KEY_free(ecdh);

    SSL_CTX_set_options(ctx, SSL_OP_SINGLE_DH_USE|SSL_OP_SINGLE_ECDH_USE);

    SSL_CTX_set_cipher_list(ctx, "EECDH+AES128:EDH+AES128:!ECDSA:!DSS");

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



void ssl_rehash(void)
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

int ssl_accept(aClient *acptr)
{

    int ssl_err;

    if ((ssl_err = SSL_accept(acptr->ssl)) <= 0)
    {
	switch (ssl_err = SSL_get_error(acptr->ssl, ssl_err))
	{
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
    }
    return 1;
}

int ssl_shutdown(SSL *ssl) 
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

static int ssl_fatal(int ssl_error, int where, aClient *sptr _D_UNUSED_)
{
    int errtmp = errno;
#if defined(USE_SYSLOG) || defined(DEBUGMODE)
    char *errstr = strerror(errtmp);
#endif
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
