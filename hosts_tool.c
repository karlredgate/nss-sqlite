
/** \file hosts.c
 * \brief Shell command to 
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <sqlite3.h>

#include "hosts.h"

static void usage() {
    fprintf( stderr, "Usage: hosts --add|--delete [--zone interface] hostname address\n" );
    exit( EINVAL );
}

static int debug = 0;
static int command = 0;

#define ADD_HOST 1
#define DEL_HOST 2

static struct option options[] = {
    { "add",     no_argument, &command, ADD_HOST },
    { "delete",  no_argument, &command, DEL_HOST },
    { "debug",   no_argument, &debug, 1 },
    { "zone",    required_argument, NULL, 'z' },
    { 0, 0, 0, 0 },
};

/** Locate interface for internal communications.
 * 
 * Glob the sysconfig dir to search each file for config.
 * Eventually may cache info if too slow locating
 */
int
main( int argc, char **argv ) {
    int result = 1;

    char *zone = NULL;
    char *hostname, *address;
    int family;
    struct in6_addr a6;
    struct in_addr a4;

    while ( 1 ) {
        int c = getopt_long(argc, argv, "", options, NULL);
	if ( c == -1 ) break;
	if ( c == 0 ) continue;
	switch ( c ) {
	case 'z':
	    zone = optarg;
	    break;
	}
    }

    if ( (argc - optind) < 2 )  usage();

    if ( debug ) Hosts_setdebug( debug );

    hostname = argv[optind];
    address = argv[optind+1];

    if ( inet_pton(AF_INET6, address, &a6) > 0 ) {
        family = AF_INET6;
    } else if ( inet_pton(AF_INET, address, &a4) > 0 ) {
        family = AF_INET;
    } else {
        fprintf( stderr, "Invalid address\n" );
	exit( 1 );
    }

    if ( debug ) printf( "add '%s' as '%s' in zone '%s'\n", hostname, address, zone );

    switch ( command ) {
    case ADD_HOST:
	if ( zone == NULL ) {
	} else {
            if ( Hosts_add_zoned_host(hostname, address, zone) < 0 ) {
	        printf( "failed to add host\n" );
	    } else {
	        result = 0;
	    }
	}
	break;
    case DEL_HOST:
	if ( zone == NULL ) {
	} else {
            if ( Hosts_del_zoned_host(hostname, address, zone) < 0 ) {
	        printf( "failed to del host\n" );
	    } else {
	        result = 0;
	    }
	}
	break;
    default: usage();
    }

    return result;
}

/*
 * vim:autoindent
 */
