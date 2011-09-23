// LSA related Header 

#include "ospfd/ospfd.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_opaque.h"
#include "tables.h"

#define ASYNCPORT 4000
#define NAME_OPAQUE_LSA 235
#define ADJ_OPAQUE_LSA 234

struct ccnx_opaque_lsa
{
 struct lsa_header lsah;
 char name[1];
};

struct ccnx_adjacent_opaque_lsa{
        struct lsa_header lsah;
        char adj_list[1];
};

//extern struct ccn_neighbors *neighbors=NULL;

// function protoype

void ccnx_opaque_lsa_print (struct ccnx_opaque_lsa  *col);
void ccnx_name_opaque_lsa_print (struct ccnx_opaque_lsa  *ol);
void ccnx_adjacent_opaque_lsa_print (struct ccnx_adjacent_opaque_lsa  *aol);

void ospf_router_lsa_print (struct router_lsa  *rl, u_int16_t length);
void ccnx_lsa_header_dump (struct lsa_header *lsah);


void update_opaque_lsa(struct ccnx_opaque_lsa *col);
void update_name_opaque_lsa(struct ccnx_opaque_lsa *col);

void delete_opaque_lsa(struct ccnx_opaque_lsa * lsa);
void delete_name_opaque_lsa(struct ccnx_opaque_lsa * col);

