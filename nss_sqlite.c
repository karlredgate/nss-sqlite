
/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2013-2024 Karl Redgate
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// #pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <nss.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

#include <sqlite3.h>

#include "hosts.h"

static char *by_name = "SELECT address  FROM host WHERE hostname = ?";
static char *by_addr = "SELECT hostname FROM host WHERE address  = ?";

struct in6_data {
    struct in6_addr address;
    char *addresses[2];
    char *aliases[1];
    char hostname[80];
};

struct in4_data {
    struct in6_addr address;
    char *addresses[2];
    char *aliases[1];
    char hostname[80];
};

/**
 */
static size_t
strlcpy(char *dst, const char *src, size_t siz) {
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit */
    if (n != 0) {
        while (--n != 0) {
            if ((*d++ = *s++) == '\0') break;
        }
    }
    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (siz != 0) *d = '\0';                /* NUL-terminate dst */
        while (*s++) ;
    }
    return(s - src - 1);        /* count does not include NUL */
}

/**
 */
static enum nss_status
populate( sqlite3_stmt *stmt,
          struct hostent *result, char *buffer, size_t length,
          int *errnop )
{
    const char *hostname;
    const char *address;

    int delta;
    char *bufp;
    struct in6_addr *inet6_addr;
    struct in_addr  *inet_addr;

    if ( stmt == NULL ) {
        return NSS_STATUS_NOTFOUND;
    }

    memset( result, 0, sizeof(*result) );

    hostname = (const char *)sqlite3_column_text( stmt, 0 );
    address = (const char *)sqlite3_column_text( stmt, 1 );

    bufp = buffer;

    if ( inet_pton(AF_INET6, address, buffer) > 0 ) {
        inet6_addr = (struct in6_addr *)bufp;
        delta = sizeof(struct in6_addr);
        result->h_addrtype = AF_INET6;
        result->h_length = sizeof(struct in6_addr);
    }

    if ( inet_pton(AF_INET, address, buffer) > 0 ) {
        inet_addr = (struct in_addr *)bufp;
        delta = sizeof(struct in_addr);
        result->h_addrtype = AF_INET;
        result->h_length = sizeof(struct in_addr);
    }

    if ( length < delta ) goto range_error;
    bufp += delta; length -= delta;

    result->h_addr_list = (char **)bufp;
    delta = sizeof(char*) * 2;
    if ( length < delta ) goto range_error;
    bufp += delta; length -= delta;

    result->h_addr_list[0] = buffer;
    result->h_addr_list[1] = 0;

    result->h_name = bufp;
    delta = strlcpy( result->h_name, hostname, length );
    if ( length < delta ) goto range_error;
    bufp += delta; length -= delta;

    result->h_aliases = (char **)bufp;
    result->h_aliases[0] = 0;

    return NSS_STATUS_SUCCESS;

range_error:
    *errnop = ERANGE;
    return NSS_STATUS_TRYAGAIN;
}

/**
 */
enum nss_status
_nss_sqlite_gethostbyname2_r( const char *name, int family, 
                           struct hostent *result,
                           char *buffer, size_t buflen,
                           int *errnop, int *h_errnop )
{
    enum nss_status status = NSS_STATUS_NOTFOUND;

    struct in6_data *data6 = (struct in6_data *)buffer;
    struct in4_data *data4 = (struct in4_data *)buffer;

    sqlite3      *db = NULL;
    sqlite3_stmt *stmt = NULL;

    if ( sqlite3_open(HOSTSDB, &db) != SQLITE_OK ) goto close;
    if ( sqlite3_prepare(db, by_name, strlen(by_name), &stmt, NULL) != SQLITE_OK ) {
        goto close;
    }

    if ( sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK ) {
        goto finalize;
    }

    while (1) {
        size_t delta;

        const char *address;
        int lookup = sqlite3_step( stmt );

        if ( lookup == SQLITE_DONE ) goto finalize;
        if ( lookup == SQLITE_BUSY ) {
            status = NSS_STATUS_TRYAGAIN;
            goto finalize;
        }
        if ( lookup != SQLITE_ROW ) goto finalize;

        address = (const char *)sqlite3_column_text( stmt, 0 );

        switch ( family ) {
        case AF_INET6:
            if ( inet_pton(AF_INET6, address, buffer) < 1 ) continue;
            delta = sizeof(struct in6_addr);
            result->h_addrtype = AF_INET6;
            result->h_addr_list = data6->addresses;
            result->h_aliases = data6->aliases;
            result->h_name = data6->hostname;
            break;
        case AF_INET:
            if ( inet_pton(AF_INET, address, buffer) < 1 ) continue;
            delta = sizeof(struct in_addr);
            result->h_addrtype = AF_INET;
            result->h_addr_list = data4->addresses;
            result->h_aliases = data4->aliases;
            result->h_name = data4->hostname;
            break;
        }
        result->h_length = delta;
        result->h_addr_list[0] = buffer;
        result->h_addr_list[1] = NULL;
        result->h_aliases[0] = NULL;
        strcpy( result->h_name, name );

        status = NSS_STATUS_SUCCESS;
        break;
    }

finalize:
    sqlite3_finalize( stmt );
close:
    sqlite3_close( db );
    return status;
}

/**
 */
enum nss_status
_nss_sqlite_gethostbyname_r( const char *name, struct hostent *result,
                          char *buffer, size_t buflen,
                          int *errnop, int *h_errnop )
{
    return _nss_sqlite_gethostbyname2_r( name, AF_INET, result, buffer, buflen, errnop, h_errnop );
}

/** Lookups for getaddrinfo and related functions
 */
enum nss_status
_nss_sqlite_gethostbyname3_r( const char *name, int family, 
                           struct hostent *result,
                           char *buffer, size_t buflen,
                           int *errnop, int *h_errnop,
                           int32_t *ttlp, char **canonp )
{
    return _nss_sqlite_gethostbyname2_r( name, family, result, buffer, buflen, errnop, h_errnop );
}

/**
 */
enum nss_status
_nss_sqlite_gethostbyaddr_r( const char *address, socklen_t len, int family,
                          struct hostent *result,
                          char *buffer, size_t buflen,
                          int *errnop, int *h_errnop )
{
    enum nss_status status = NSS_STATUS_NOTFOUND;

    char addrbuf[128];
    const char *addr;

    struct in6_data *data6 = (struct in6_data *)buffer;
    struct in4_data *data4 = (struct in4_data *)buffer;

    sqlite3      *db = NULL;
    sqlite3_stmt *stmt = NULL;

    if ( sqlite3_open(HOSTSDB, &db) != SQLITE_OK ) goto close;
    if ( sqlite3_prepare(db, by_addr, strlen(by_addr), &stmt, NULL) != SQLITE_OK ) {
        goto close;
    }

    addr = inet_ntop( family, address, addrbuf, sizeof(addrbuf) );

    if ( addr == NULL ) {
        /* return error */
    }

    if ( sqlite3_bind_text(stmt, 1, addr, -1, SQLITE_STATIC) != SQLITE_OK ) {
        goto finalize;
    }

    while (1) {
        size_t delta;

        const char *hostname;
        int lookup = sqlite3_step( stmt );

        if ( lookup == SQLITE_DONE ) goto finalize;
        if ( lookup == SQLITE_BUSY ) {
            status = NSS_STATUS_TRYAGAIN;
            goto finalize;
        }
        if ( lookup != SQLITE_ROW ) goto finalize;

        hostname = (const char *)sqlite3_column_text( stmt, 0 );

        switch ( family ) {
        case AF_INET6:
            if ( inet_pton(AF_INET6, addr, buffer) < 1 ) continue;
            delta = sizeof(struct in6_addr);
            result->h_addrtype = AF_INET6;
            result->h_addr_list = data6->addresses;
            result->h_aliases = data6->aliases;
            result->h_name = data6->hostname;
            break;
        case AF_INET:
            if ( inet_pton(AF_INET, addr, buffer) < 1 ) continue;
            delta = sizeof(struct in_addr);
            result->h_addrtype = AF_INET;
            result->h_addr_list = data4->addresses;
            result->h_aliases = data4->aliases;
            result->h_name = data4->hostname;
            break;
        }
        result->h_length = delta;
        result->h_addr_list[0] = buffer;
        result->h_addr_list[1] = NULL;
        result->h_aliases[0] = NULL;
        strcpy( result->h_name, hostname );

        status = NSS_STATUS_SUCCESS;
        break;
    }

finalize:
    sqlite3_finalize( stmt );
close:
    sqlite3_close( db );
    return status;
}

static sqlite3      *gethostent_db = NULL;
static sqlite3_stmt *gethostent_stmt = NULL;
static char *gethostent_sql = "SELECT hostname,address FROM host";

static int
prepare_hostent() {
    if ( gethostent_stmt != NULL ) {
        sqlite3_finalize( gethostent_stmt );
    }
    return sqlite3_prepare(gethostent_db, gethostent_sql, strlen(gethostent_sql), &gethostent_stmt, NULL);
}

/** Prepare for gethostent
 *
 * mmap the current netmgr host table file and reset the current
 * index so gethostent will start at the beginning.
 */
enum nss_status
_nss_sqlite_sethostent( int persist ) {

    if ( gethostent_db == NULL ) {
        if ( sqlite3_open(HOSTSDB, &gethostent_db) != SQLITE_OK ) {
            sqlite3_close( gethostent_db );
            gethostent_db = NULL;
            /* not sure if this is a reasonable return here */
            return NSS_STATUS_TRYAGAIN;
        }
    }

    if ( prepare_hostent() != SQLITE_OK ) {
        sqlite3_close( gethostent_db );
        gethostent_db = NULL;
        /* not sure if this is a reasonable return here */
        return NSS_STATUS_TRYAGAIN;
    }

    return NSS_STATUS_SUCCESS;
}

/** Cleanup after gethostent
 */
enum nss_status
_nss_sqlite_endhostent() {
    if ( gethostent_stmt != NULL ) {
        sqlite3_finalize( gethostent_stmt );
        gethostent_stmt = NULL;
    }
    if ( gethostent_db != NULL ) {
        sqlite3_close( gethostent_db );
        gethostent_db = NULL;
    }
    return NSS_STATUS_SUCCESS;
}

/** Return next hostent in the current iterator
 *
 * This is normally called after a sethostent(), but this is written to
 * handle ill-behaved apps also by checking if it needs to be called and
 * calling before returning the first entry.
 */
enum nss_status
_nss_sqlite_gethostent_r( struct hostent *result, char *buffer, size_t buflen,
                       int *errnop, int *h_errnop )
{
    int lookup;

    if ( gethostent_stmt == NULL ) {
        _nss_sqlite_sethostent( 0 );
    }

    lookup = sqlite3_step( gethostent_stmt );

    switch ( lookup ) {
    case SQLITE_ROW:
        populate( gethostent_stmt, result, buffer, buflen, errnop );
        return NSS_STATUS_SUCCESS;
    case SQLITE_BUSY:
        return NSS_STATUS_TRYAGAIN;
    case SQLITE_DONE:
        /* do something else?? */
        /* for now fall thru and treat like failure */
        break;
    }

    _nss_sqlite_endhostent();
    return NSS_STATUS_NOTFOUND;
}

/*
 * vim:autoindent
 * vim:expandtab
 */
