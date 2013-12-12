
/** \file hosts.h
 * \brief 
 *
 */

#ifndef _HOSTS_H_
#define _HOSTS_H_

#include <stdint.h>

#define HOSTSDB "/var/db/hosts.db"

#ifdef __cplusplus
extern "C" {
#endif

void Hosts_setdebug( int value );
int Hosts_add_zoned_host( char *hostname, char *address, char *zone );
int Hosts_del_zoned_host( char *hostname, char *address, char *zone );

#ifdef __cplusplus
}
#endif

#endif

/*
 * vim:autoindent
 * vim:expandtab
 */
