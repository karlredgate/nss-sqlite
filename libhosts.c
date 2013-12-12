
/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2013 Karl Redgate
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

/** \file libhosts.c
 * \brief Library functions to 
 *
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glob.h>

#include <sqlite3.h>

#include "hosts.h"

static int debug = 0;

/**
 * This is just to allow for some debugging of the library.
 */
void
Hosts_setdebug( int value ) {
    /* do some error checks */
    debug = value;
}

/**
 */
static int
Hosts_opendb( sqlite3 **db ) {
    char *dbfile = getenv("HOSTSDB");
    if ( dbfile == NULL ) {
        dbfile = HOSTSDB;
    }
    if ( debug ) fprintf( stderr, "opening db file %s\n", dbfile );
    return sqlite3_open(dbfile, db);
}

/*
 * SQL statement to be used for adding to the hosts db.
 */
static char *zoned_insert = "INSERT OR REPLACE INTO host (hostname,zone,address) VALUES (?,?,?)";

/**
 */
int
Hosts_add_zoned_host( char *hostname, char *address, char *zone ) {
    int result = -1;
    sqlite3      *db = NULL;
    sqlite3_stmt *stmt = NULL;
    int status;

    /* if ( sqlite3_open(HOSTSDB, &db) != SQLITE_OK ) goto close; */
    if ( Hosts_opendb(&db) != SQLITE_OK ) goto close;
    if ( sqlite3_prepare(db, zoned_insert, strlen(zoned_insert), &stmt, NULL) != SQLITE_OK ) {
	if ( debug ) fprintf( stderr, "could not prepare\n" );
        goto close;
    }

    if ( sqlite3_bind_text(stmt, 1, hostname, -1, SQLITE_STATIC) != SQLITE_OK ) {
	if ( debug ) fprintf( stderr, "could not bind hostname\n" );
        goto finalize;
    }
    if ( sqlite3_bind_text(stmt, 2, zone, -1, SQLITE_STATIC) != SQLITE_OK ) {
	if ( debug ) fprintf( stderr, "could not bind zone\n" );
        goto finalize;
    }
    if ( sqlite3_bind_text(stmt, 3, address, -1, SQLITE_STATIC) != SQLITE_OK ) {
	if ( debug ) fprintf( stderr, "could not bind address\n" );
        goto finalize;
    }

    status = sqlite3_step( stmt );
    if ( status == SQLITE_DONE ) {
        result = 0;
	if ( debug ) fprintf( stderr, "host added\n" );
    } else {
        if ( debug ) fprintf( stderr, "failed to add %s\n", hostname );
	if ( debug ) fprintf( stderr, "step = %d\n", status );
    }

finalize:
    sqlite3_finalize( stmt );
close:
    sqlite3_close( db );
    return result;
}

/*
 * SQL statement to be used for removing from the hosts db.
 */
static char *zoned_delete = "DELETE FROM host WHERE hostname=? and zone=? and address=?";

/**
 */
int
Hosts_del_zoned_host( char *hostname, char *address, char *zone ) {
    int result = -1;
    sqlite3      *db = NULL;
    sqlite3_stmt *stmt = NULL;
    int status;

    /* if ( sqlite3_open(HOSTSDB, &db) != SQLITE_OK ) goto close; */
    if ( Hosts_opendb(&db) != SQLITE_OK ) goto close;
    if ( sqlite3_prepare(db, zoned_delete, strlen(zoned_delete), &stmt, NULL) != SQLITE_OK ) {
	if ( debug ) fprintf( stderr, "could not prepare\n" );
        goto close;
    }

    if ( sqlite3_bind_text(stmt, 1, hostname, -1, SQLITE_STATIC) != SQLITE_OK ) {
	if ( debug ) fprintf( stderr, "could not bind hostname\n" );
        goto finalize;
    }
    if ( sqlite3_bind_text(stmt, 2, zone, -1, SQLITE_STATIC) != SQLITE_OK ) {
	if ( debug ) fprintf( stderr, "could not bind zone\n" );
        goto finalize;
    }
    if ( sqlite3_bind_text(stmt, 3, address, -1, SQLITE_STATIC) != SQLITE_OK ) {
	if ( debug ) fprintf( stderr, "could not bind address\n" );
        goto finalize;
    }

    status = sqlite3_step( stmt );
    if ( status == SQLITE_DONE ) {
        result = 0;
	if ( debug ) fprintf( stderr, "host deleted\n" );
    } else {
        if ( debug ) fprintf( stderr, "failed to delete %s\n", hostname );
	if ( debug ) fprintf( stderr, "step = %d\n", status );
    }

finalize:
    sqlite3_finalize( stmt );
close:
    sqlite3_close( db );
    return result;
}

static char *insertion = "INSERT INTO host (hostname,address) VALUES (?,?)";

/**
 */
int
Hosts_add_host( char *hostname, char *address ) {
    int result = 0;
    sqlite3      *db = NULL;
    sqlite3_stmt *stmt = NULL;

    if ( sqlite3_open(HOSTSDB, &db) != SQLITE_OK ) goto close;
    if ( sqlite3_prepare(db, insertion, strlen(insertion), &stmt, NULL) != SQLITE_OK ) {
        goto close;
    }

    if ( sqlite3_bind_text(stmt, 1, hostname, -1, SQLITE_STATIC) != SQLITE_OK ) {
        goto finalize;
    }
    if ( sqlite3_bind_text(stmt, 2, address, -1, SQLITE_STATIC) != SQLITE_OK ) {
        goto finalize;
    }

finalize:
    sqlite3_finalize( stmt );
close:
    sqlite3_close( db );
    return result;
}

/*
 * vim:autoindent
 */
