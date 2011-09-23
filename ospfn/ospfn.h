// header file for opsfn.c containing definitions of functions and variables
// Author: Hoque

#ifndef _OSPFN_H_
#define _OSPFN_H_

#include "thread.h"
#include "ospfclient/ospf_apiclient.h"
#include "tables.h"

/*
struct name_prefix{
        unsigned int limit;
        unsigned int length;
        unsigned char *name;
        };
*/

struct ccn_neighbors{
        struct in_addr address;
        int path_cost;

        struct ccn_neighbors * next;
        };


struct my_opaque_lsa
{
  struct lsa_header hdr; /* include common LSA header */
  u_char data[4]; /* our own data format then follows here */
};

/* Master thread */
struct thread_master *master;

/* Global variables */
char **args;
struct ospf_apiclient *oclient;

/* hash tables */
struct hash *prefix_table;
struct hash *origin_table;

char  *logFile;

//----------Data Structure Added for ospfn -------
//data structures 

// proto type for function implemented in ospfn.c

//static void add_neighbor(char *neighbor, int cost);
//static void print_ccn_neighbors();
//static int usage(char *progname);
void process_command_ccnmaxnexthops(char *command);
void process_command_ccnneighbor(char *command);
void process_command_ccnname(char *command);
void process_conf_command(char *command);
int readConfigFile(char *filename);
//static void process_adjacent();


int inject_adjacency_opaque_lsa(struct thread *t);
void inject_name_opaque_lsa( struct name_prefix *np, unsigned int op_id);


#endif
