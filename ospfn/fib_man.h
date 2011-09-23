#include <zebra.h>
#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include<stdlib.h>

#include "hash.h"
#include "memory.h"
#include "memtypes.h"

struct name_prefix { //  name prefix
       unsigned int limit;
       unsigned int length;
       u_char *name;
};

struct nameprefixlist_entry { // name prefix linked list
       struct name_prefix *nameprefix;
       struct nameprefixlist_entry *next;
};

struct nexthoplist_entry{ // next hop linked list
       struct in_addr nexthop;
       unsigned int cost;
       unsigned int flag;
       struct nexthoplist_entry *next;
};

struct originlist_entry { // origin linked list

       struct in_addr origin; // address
       struct originlist_entry *next;
};

struct fibtable_entry{ // fib hash table entry
       struct name_prefix *nameprefix; // name prefix
       struct nexthoplist_entry *nexthop_list; // next hop list
       struct originlist_entry *origin_list; // origin list
};

struct origintable_entry{ // origin hash table entry
       struct in_addr origin;
       struct nameprefixlist_entry *nameprefix_list; // name prefix list
       struct nexthoplist_entry *nexthop_list; // nexthop prefix list
};


void *mycalloc(size_t nmemb, size_t size);

// fib hash table opearations
unsigned int fib_hash_key_make(void *p);
int fib_hash_cmp(const void *p1, const void *p2);
void * fib_hash_alloc (void *p);
struct name_prefix * copy_name_prefix(struct name_prefix *nameprefix);
void free_nameprefix(struct name_prefix *np);
struct nexthoplist_entry * fib_add_nexthop(struct fibtable_entry *fibentry, struct in_addr nexthop, unsigned int cost, unsigned int flag);
struct originlist_entry * fib_add_origin(struct fibtable_entry *fibentry, struct in_addr origin);
void fib_delete_nexthop(struct fibtable_entry *fibentry, struct in_addr nexthop);
void fib_delete_origin(struct fibtable_entry *fibentry, struct in_addr origin);

void iterate_fib_entry(struct hash *hash, struct name_prefix *nameprefix, int type);
struct fibtable_entry *lookup_insert_fibentry(struct hash *hash, struct name_prefix *nameprefix, void * (*alloc_func) (void *));
struct fibtable_entry *lookup_fibentry(struct hash *hash, struct name_prefix *nameprefix);
void delete_fib_entry(struct hash *hash, struct name_prefix *nameprefix);

// origin hash table operations
unsigned int origin_hash_key_make(void *p);                                                                     
void *origin_hash_alloc (void *p);                                                                             
int origin_hash_cmp(const void *p1, const void *p2);                                                            
struct origintable_entry * lookup_insert_originentry(struct hash *hash, struct in_addr origin, void * (*alloc_func) (void *));
struct origintable_entry * lookup_originentry(struct hash *hash, struct in_addr origin);
struct nexthoplist_entry * origintable_add_nexthop(struct origintable_entry *fibentry, struct in_addr nexthop, unsigned int cost, unsigned int flag);
struct nameprefixlist_entry * origintable_add_nameprefix(struct origintable_entry *originentry, struct name_prefix *nameprefix);                  
void origintable_delete_nexthop(struct origintable_entry *fibentry, struct in_addr nexthop);                                  
void origintable_delete_nameprefix(struct origintable_entry *fibentry, u_char *name);
void delete_origintable_entry(struct hash *hash, struct in_addr origin);
void iterate_origintable_entry(struct hash *hash, struct in_addr origin, int type);

