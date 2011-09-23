#include <zebra.h>
#include "prefix.h" /* needed by ospf_asbr.h */
#include "privs.h"
#include "log.h"
//header added for ospfn implementation
#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include<stdlib.h>

#include "ndn_route.h"
#include "fib_man.h"
#include "ccnx_opaque_lsa.h"
#include "utility.h"
#include "uthash.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_opaque.h"
#include "ospfd/ospf_api.h"
#include "ospfclient/ospf_apiclient.h"
#include "lib/stream.h"

/*
struct zebra_privs_t ospfd_privs =
{
  .user = NULL,
  .group = NULL,
  .cap_num_p = 0,
  .cap_num_i = 0
};
*/
/* Master thread */
//struct thread_master *master;

unsigned int fib_hash_key_make(void *p){
        struct name_prefix *np = (struct name_prefix *)p;
        unsigned int key_size = np->length;
        unsigned int h;
        unsigned int i;
        for (h = key_size + 23, i = 0; i < key_size; i++)
                h = ((h << 6) ^ (h >> 27)) + np->name[i];
        return h;
}

//Note: the p1 is fibtable_entry, p2 is name_prefix  
int fib_hash_cmp(const void *p1, const void *p2){
        struct fibtable_entry *fe = (struct fibtable_entry *)p1;
        struct name_prefix *np1 = fe->nameprefix;
	struct name_prefix *np2 = (struct name_prefix *)p2;
        return (np1->length == np2->length && memcmp (np1->name, np2->name , np1->length) == 0);        
}                                                                                                             
                                                                                                              
void * fib_hash_alloc (void *p)                                                                          
{  	
	struct name_prefix *np = (struct name_prefix *)p; 
	struct fibtable_entry *newdata = (struct fibtable_entry *)mycalloc(sizeof(struct fibtable_entry), 1);
	newdata->nameprefix = (struct name_preifx *)copy_name_prefix(np);
	return newdata;
}


void *mycalloc(size_t nmemb, size_t size)
{
       void *p = calloc(nmemb, size);
       if (p == NULL) {
               printf("Memory allocation failed.  Exit.");
               exit(-1);
       }
       return p;
}

struct name_prefix * copy_name_prefix(struct name_prefix *nameprefix){

	struct name_prefix *np = (struct name_prefix *)mycalloc(sizeof(struct name_prefix), 1);
	//*np = *nameprefix;
	np->name = (u_char *)mycalloc(sizeof(u_char), nameprefix->length);
	np->length = nameprefix->length;
	np->limit = nameprefix->limit;
	memcpy(np->name, nameprefix->name, nameprefix->length);
	return np;
}

void free_nameprefix(struct name_prefix *np)
{
       free(np->name);
       free(np);
}

void delete_fib_entry(struct hash *hash, struct name_prefix *nameprefix){
	
	struct fibtable_entry *f1 = hash_release(hash, (void *) nameprefix);
	
	struct nexthoplist_entry *n1 = f1->nexthop_list; // get the pointer of next hop list
	struct originlist_entry *o1 = f1->origin_list; // get the pointer of origin list
	struct nexthoplist_entry *nexthop_temp = NULL;
	while(n1){
		nexthop_temp = n1;
		n1 = n1->next;                                                                     
		free(nexthop_temp);                                                                
	}                                                                                          
	struct originlist_entry *origin_temp = NULL;                                               
	while(o1){                                                                                 
		origin_temp = o1;                                                                  
		o1 = o1->next;                                                                     
		free(origin_temp);                                                                 
	}                                                                                          
	printf("name: %s\n", f1->nameprefix->name);                                                
	//free(f1->nameprefix->name);                                                              
	free_nameprefix(f1->nameprefix);                                                           
	free(f1); // need to think about the solution here

}
struct nexthoplist_entry * fib_add_nexthop(struct fibtable_entry *fibentry, struct in_addr nexthop, unsigned int cost, unsigned int flag){
	struct nexthoplist_entry *n = fibentry->nexthop_list;
	// Search by nexthop, if the search nexthop is NULL, then return the head 
	struct nexthoplist_entry *previous = fibentry->nexthop_list;                                              
	// search by order from small to big                                                                
	while(n){                                                                                          
		if(nexthop.s_addr < n->nexthop.s_addr ){                                               
			printf("break\n");                                                                  
			break;                                                                                              
		}                                                                                                       
		else if(nexthop.s_addr == n->nexthop.s_addr && cost == n->cost && flag == n->flag){
			return n; 
			printf("Already in list.\n");                                                                         
		}                                                                                           
		printf("n:%u,%s\tn:%u,%u\n", n->nexthop.s_addr, inet_ntoa(n->nexthop), n->cost, n->flag);
		previous = n;                                                                              
		n = n->next;
		                                                                              
	}
	printf("Cannot find next hop and cost.\n");
	// list head is NULL
	struct nexthoplist_entry *ne = (struct nexthoplist_entry *)mycalloc(sizeof(struct nexthoplist_entry),1);
	printf("Cost: %d,flag: %d\n", cost, flag);
	ne->nexthop = nexthop;
	ne->cost = cost;
	ne->flag = flag;
	if(fibentry->nexthop_list == NULL){                                                                       
		fibentry->nexthop_list = ne;                                                                      
		printf("Add next hop at beginning.\n");                                                     
	}                                                                                                   
	else if(n == NULL) {//  search to the end, and need to insert in the end                           
		previous->next = ne;                                                                        
		printf("Add next hop in end\n");                                                            
	}                                                                                                   
	else if(n == fibentry->nexthop_list){ //search the first one, and need to insert before the first one    
		ne->next = n;                                                                                                      
		fibentry->nexthop_list = ne;                                                                      
		//if(n != NULL)                                                                                                          
		printf("Add next hop after master node\n");                                                                                  
	}                                                                                                   
	else{ // insert in the middle                                                                       
		ne->next = n;                                                                              
		previous->next = ne;                                                                        
		printf("previous:%u,%s\n", previous->nexthop.s_addr, inet_ntoa(previous->nexthop));         
		printf("n:%u,%s\n", n->nexthop.s_addr, inet_ntoa(n->nexthop));                           
		printf("Add next hop in middle\n");                                                         
	}                                                                                                   
	printf("Add next hop %s.\n",inet_ntoa(ne->nexthop));
	return ne;
}                    

struct originlist_entry * fib_add_origin(struct fibtable_entry *fibentry, struct in_addr origin){
        struct originlist_entry *n = fibentry->origin_list;
        // Search by nexthop, if the search nexthop is NULL, then return the head 
        struct originlist_entry *previous = fibentry->origin_list;
        // search by order from small to big                                                                
        while(n){
                if(origin.s_addr < n->origin.s_addr ){
                        printf("break\n");
                        break;
                }
                else if(origin.s_addr == n->origin.s_addr){
                        printf("%s already in list.\n", inet_ntoa(n->origin));
                        return n;
                }
                printf("n:%u,%s\n", n->origin.s_addr, inet_ntoa(n->origin));
                previous = n;
                n = n->next;

        }
        printf("Cannot find origin.\n");
        // list head is NULL
        struct originlist_entry *ne = (struct originlist_entry *)mycalloc(sizeof(struct originlist_entry),1);
        printf("Origin: %s\n", inet_ntoa(origin));
        ne->origin = origin;
	if(fibentry->origin_list == NULL){
                fibentry->origin_list = ne;                                                                       
                printf("Add origin at beginning.\n");                                                            
        }
        else if(n == NULL) {//  search to the end, and need to insert in the end                                   
                previous->next = ne;                                                                               
                printf("Add origin in end\n");                                                                   
        }                                                                                                          
        else if(n == fibentry->origin_list){ //search the first one, and need to insert before the first one      
                ne->next = n;                                                                                                      
                fibentry->origin_list = ne;                                                                       
                //if(n != NULL)                                                                                                      
    
                printf("Add origin after master node\n");                                                                         

        }                                                                                                          
        else{ // insert in the middle                                                                              
                ne->next = n;                                                                                      
                previous->next = ne;                                                                               
                printf("previous:%u,%s\n", previous->origin.s_addr, inet_ntoa(previous->origin));                
                printf("n:%u,%s\n", n->origin.s_addr, inet_ntoa(n->origin));                                     
                printf("Add origin in middle\n");                                                                
        }                                                                                                          
        printf("Add origin %s.\n",inet_ntoa(ne->origin));                                                       
        return ne;                                                                                                 
}

// Delete all items with the same next hops, even if the costs are different
void fib_delete_nexthop(struct fibtable_entry *fibentry, struct in_addr nexthop){
	struct nexthoplist_entry *n = fibentry->nexthop_list;
        // Search by nexthop, if the search nexthop is NULL, then return the head 
        struct nexthoplist_entry *previous = fibentry->nexthop_list;
	struct nexthoplist_entry *temp = NULL;
        // search by order from small to big                                                                
        while(n){
                if(nexthop.s_addr < n->nexthop.s_addr ){
                        printf("break, no such next hop found.\n");
                        break;
                }
                else if(nexthop.s_addr == n->nexthop.s_addr){
                        //return n; 
			if(n == fibentry->nexthop_list){
				fibentry->nexthop_list = n->next;
			}
			else{
				previous->next = n->next;
			}
                        temp = n;
			n = n->next;
			free(temp);
			printf("delete from list.\n");
                }
		else{
                	previous = n;
                	n = n->next;
		}
        }

}


void fib_delete_origin(struct fibtable_entry *fibentry, struct in_addr origin){
	struct originlist_entry *n = fibentry->origin_list;
        // Search by nexthop, if the search nexthop is NULL, then return the head 
        struct originlist_entry *previous = fibentry->origin_list;
        // search by order from small to big                                                                
	while(n){
		if(origin.s_addr < n->origin.s_addr ){
                        printf("break, no such origin found.\n");
                        break;
                }
                else if(origin.s_addr == n->origin.s_addr){
                        //return n; 
                        printf("Deleting origin: %s\n", inet_ntoa(n->origin));
			if(n == fibentry->origin_list){
                                fibentry->origin_list = n->next;
                        }
                        else{
                                previous->next = n->next;
                        }
                        free(n);
                        printf("delete from list.\n");
			break;
                }
		else{
                        previous = n;
                        n = n->next;
                }
        }
	
}


struct fibtable_entry *lookup_insert_fibentry(struct hash *hash, struct name_prefix *nameprefix, void * (*alloc_func) (void *)){
	return (struct fibtable_entry *) hash_get(hash, (void *) nameprefix, *alloc_func);	
}
struct fibtable_entry *lookup_fibentry(struct hash *hash, struct name_prefix *nameprefix){
	return (struct fibtable_entry *) hash_get(hash, (void *) nameprefix, NULL);
}

// type == 1 output nexthop info
// type == 2 output origin info
// type == 3 output both
void iterate_fib_entry(struct hash *hash, struct name_prefix *nameprefix, int type){
	unsigned int key;
        unsigned int index;
        struct hash_backet *backet;
	printf("ok\n");
        key = (*hash->hash_key) (nameprefix);
        index = key % hash->size;
        for (backet = hash->index[index]; backet != NULL; backet = backet->next){
                struct fibtable_entry *f1 = (struct fibtable_entry *)backet->data;
                if (backet->key == key && (*hash->hash_cmp) (f1, nameprefix)){
			printf("Iterate name prefix: %s\n", f1->nameprefix->name);
                        struct nexthoplist_entry *n1 = f1->nexthop_list; // get the pointer of next hop list
                        struct originlist_entry *o1 = f1->origin_list; // get the pointer of origin list
                        while(n1 && (type == 1 || type == 3)){
				printf("Iterate nexthop, name: %s, np: %s, cost: %d, flag: %d\n", 
					f1->nameprefix->name, inet_ntoa(n1->nexthop),n1->cost, n1->flag);
                                //nexthop_temp = n1;
                                n1 = n1->next;
                        }
                        //struct originlist_entry *origin_temp = NULL;
                        while(o1 && (type == 2 || type == 3)){
				printf("Iterate origin, name: %s, origin: %s\n", f1->nameprefix->name, inet_ntoa(o1->origin));
                                //origin_temp = o1;
                                o1 = o1->next;
                        }
                }
        }

}

unsigned int origin_hash_key_make(void *p){
	struct in_addr *o = (struct in_addr *)p;
        unsigned int key_size = strlen(inet_ntoa(*o));
        unsigned int h;
        unsigned int i;
        for (h = key_size + 23, i = 0; i < key_size; i++)
                h = ((h << 6) ^ (h >> 27)) + (inet_ntoa(*o))[i];
        return h;
}

int origin_hash_cmp(const void *p1, const void *p2){
	struct origintable_entry *ore = (struct origintable_entry *)p1;	
	struct in_addr *o1 = &(ore->origin);
	struct in_addr *o2 = (struct in_addr *)p2;
        return (o1->s_addr == o2->s_addr);
}                                                         

void * origin_hash_alloc (void *p){
	struct in_addr *o = (struct in_addr *)p;
        struct origintable_entry *newdata = (struct origintable_entry *)mycalloc(sizeof(struct origintable_entry), 1);
	struct in_addr origin;
	origin.s_addr = o->s_addr;
	newdata->origin = origin;
        return newdata;
}


struct nexthoplist_entry * origintable_add_nexthop(struct origintable_entry *originentry, struct in_addr nexthop, unsigned int cost, unsigned int flag){
	struct nexthoplist_entry *n = originentry->nexthop_list;
	// Search by nexthop, if the search nexthop is NULL, then return the head 
	struct nexthoplist_entry *previous = originentry->nexthop_list;                                              
	// search by order from small to big                                                                
	while(n){                                                                                          
		if(nexthop.s_addr < n->nexthop.s_addr ){                                               
			printf("break\n");                                                                  
			break;                                                                                              
		}                                                                                                       
		else if(nexthop.s_addr == n->nexthop.s_addr && cost == n->cost && flag == n->flag){
			return n; 
			printf("Already in list.\n");                                                                         
		}                                                                                           
		printf("n:%u,%s\tn:%u,%u\n", n->nexthop.s_addr, inet_ntoa(n->nexthop), n->cost, n->flag);
		previous = n;                                                                              
		n = n->next;
		                                                                              
	}
	printf("Cannot find next hop and cost.\n");
	// list head is NULL
	struct nexthoplist_entry *ne = (struct nexthoplist_entry *)mycalloc(sizeof(struct nexthoplist_entry),1);
	printf("Cost: %d,flag: %d\n", cost, flag);
	ne->nexthop = nexthop;
	ne->cost = cost;
	ne->flag = flag;
	if(originentry->nexthop_list == NULL){                                                                       
		originentry->nexthop_list = ne;                                                                      
		printf("Add next hop at beginning.\n");                                                     
	}                                                                                                   
	else if(n == NULL) {//  search to the end, and need to insert in the end                           
		previous->next = ne;                                                                        
		printf("Add next hop in end\n");                                                            
	}                                                                                                   
	else if(n == originentry->nexthop_list){ //search the first one, and need to insert before the first one    
		ne->next = n;                                                                                                      
		originentry->nexthop_list = ne;                                                                      
		//if(n != NULL)                                                                                                          
		printf("Add next hop after master node\n");                                                                                  
	}                                                                                                   
	else{ // insert in the middle                                                                       
		ne->next = n;                                                                              
		previous->next = ne;                                                                        
		printf("previous:%u,%s\n", previous->nexthop.s_addr, inet_ntoa(previous->nexthop));         
		printf("n:%u,%s\n", n->nexthop.s_addr, inet_ntoa(n->nexthop));                           
		printf("Add next hop in middle\n");                                                         
	}                                                                                                   
	printf("Add next hop %s.\n",inet_ntoa(ne->nexthop));
	return ne;
}                    
	
struct nameprefixlist_entry * origintable_add_nameprefix(struct origintable_entry *originentry, struct name_prefix *nameprefix){

        struct nameprefixlist_entry *n = originentry->nameprefix_list;
        // Search by nexthop, if the search nexthop is NULL, then return the head 
        struct nameprefixlist_entry *previous = originentry->nameprefix_list;
        // search by order from small to big                                                                
        while(n){
                if(strcmp(nameprefix->name, n->nameprefix->name) < 0){
                        printf("break\n");
                        break;
                }
                else if(strcmp(nameprefix->name, n->nameprefix->name) == 0){
                        printf("%s already in list.\n", n->nameprefix->name);
                        return n;
                }
                printf("n:%s\n", n->nameprefix->name);
                previous = n;
                n = n->next;

        }
        printf("Cannot find name prefix.\n");
        // list head is NULL
        struct nameprefixlist_entry *ne = (struct nameprefixlist_entry *)mycalloc(sizeof(struct nameprefixlist_entry),1);
	struct name_prefix *np = copy_name_prefix(nameprefix); 
	ne->nameprefix = np;
        printf("Name prefix: %s\n", np->name);
	if(originentry->nameprefix_list == NULL){
                originentry->nameprefix_list = ne;                                                                       
                printf("Add name prefix at beginning.\n");                                                            
        }
        else if(n == NULL) {//  search to the end, and need to insert in the end                                   
                previous->next = ne;                                                                               
                printf("Add name prefix in end\n");                                                                   
        }                                                                                                          
        else if(n == originentry->nameprefix_list){ //search the first one, and need to insert before the first one      
                ne->next = n;                                                                                                      
                originentry->nameprefix_list = ne;                                                                       
                //if(n != NULL)                                                                                                      
    
                printf("Add name prefix after master node\n");                                                                         

        }                                                                                                          
        else{ // insert in the middle                                                                              
                ne->next = n;                                                                                      
                previous->next = ne;                                                                               
                printf("previous:%s\n", previous->nameprefix->name);                
                printf("n:%s\n", n->nameprefix->name);                                     
                printf("Add nameprefix in middle\n");                                                                
        }                                                                                                          
        printf("Add name prefix %s.\n",ne->nameprefix->name);                                                       
        return ne;                                                                                                 
}

// Delete all items with the same next hops, even if the costs are different
void origintable_delete_nexthop(struct origintable_entry *originentry, struct in_addr nexthop){
	struct nexthoplist_entry *n = originentry->nexthop_list;
        // Search by nexthop, if the search nexthop is NULL, then return the head 
        struct nexthoplist_entry *previous = originentry->nexthop_list;
	struct nexthoplist_entry *temp = NULL;
        // search by order from small to big                                                                
        while(n){
                if(nexthop.s_addr < n->nexthop.s_addr ){
                        printf("break, no such next hop found.\n");
                        break;
                }
                else if(nexthop.s_addr == n->nexthop.s_addr){
                        //return n; 
			if(n == originentry->nexthop_list){
				originentry->nexthop_list = n->next;
			}
			else{
				previous->next = n->next;
			}
                        temp = n;
			n = n->next;
			free(temp);
			printf("delete from list.\n");
                }
		else{
                	previous = n;
                	n = n->next;
		}
        }

}

// Delete the item with the specified name prefix
void origintable_delete_nameprefix(struct origintable_entry *originentry, u_char *name){
	struct nameprefixlist_entry *n = originentry->nameprefix_list;
        // Search by nexthop, if the search nexthop is NULL, then return the head 
        struct nameprefixlist_entry *previous = originentry->nameprefix_list;
        // search by order from small to big                                                                
        while(n){
                if(strcmp(name, n->nameprefix->name) < 0){
                        printf("break, no name prefix found.\n");
                        break;
                }
                else if(strcmp(name, n->nameprefix->name)==0){
                        //return n; 
			if(n == originentry->nameprefix_list){
				originentry->nameprefix_list = n->next;
			}
			else{
				previous->next = n->next;
			}
			free(n->nameprefix->name);
			free(n->nameprefix);
			free(n);
			printf("Name:%s, deleted from list.\n", name);
			break;
                }
		else{
                	previous = n;
                	n = n->next;
		}
        }

}
struct origintable_entry * lookup_insert_originentry(struct hash *hash, struct in_addr origin, void * (*alloc_func) (void *)){
	return (struct origintable_entry *) hash_get(hash, (void *)&origin, *alloc_func);
}

struct origintable_entry * lookup_originentry(struct hash *hash, struct in_addr origin){
	return (struct origintable_entry *) hash_get(hash, (void *)&origin, NULL);
}

void delete_origintable_entry(struct hash *hash, struct in_addr origin){
	
	struct origintable_entry *f1 = (struct origintable_entry *)hash_release(hash, (void *) &origin);
	
	struct nexthoplist_entry *n1 = f1->nexthop_list; // get the pointer of next hop list
	struct nameprefixlist_entry *o1 = f1->nameprefix_list; // get the pointer of origin list
	struct nexthoplist_entry *nexthop_temp = NULL;
	while(n1){
		nexthop_temp = n1;
		n1 = n1->next;                                                                     
		free(nexthop_temp);                                                                
	}                                                                                          
	struct nameprefixlist_entry *nameprefix_temp = NULL;                                               
	while(o1){                                                                                 
		nameprefix_temp = o1;                                                                  
		o1 = o1->next;                                                                     
		free(nameprefix_temp->nameprefix->name);
		free(nameprefix_temp->nameprefix);
		free(nameprefix_temp);                                                                 
	}                                                                                          
	printf("Origin: %s\n", inet_ntoa(f1->origin));                                                
	//free(f1->nameprefix->name);                                                              
	free(f1); // need to think about the solution here

}
// type == 1 output nexthop info
// type == 2 output name prefix info
// type == 3 output both
void iterate_origintable_entry(struct hash *hash, struct in_addr origin, int type){
	unsigned int key;
        unsigned int index;
        struct hash_backet *backet;
	printf("ok\n");
        key = (*hash->hash_key) (&origin);
        index = key % hash->size;
        for (backet = hash->index[index]; backet != NULL; backet = backet->next){
                struct origintable_entry *f1 = (struct origintable_entry *)backet->data;
                if (backet->key == key && (*hash->hash_cmp) (&(f1->origin), &origin)){
			printf("Iterate origin: %s\n", inet_ntoa(f1->origin));
                        struct nexthoplist_entry *n1 = f1->nexthop_list; // get the pointer of next hop list
                        struct nameprefixlist_entry *o1 = f1->nameprefix_list; // get the pointer of origin list
                        while(n1 && (type == 1 || type == 3)){
				printf("Iterate nexthop, origin: %s, ", inet_ntoa(f1->origin)); 
				printf("np: %s, cost: %d, flag: %d\n", inet_ntoa(n1->nexthop),n1->cost, n1->flag);
                                //nexthop_temp = n1;
                                n1 = n1->next;
                        }
                        //struct originlist_entry *origin_temp = NULL;
                        while(o1 && (type == 2 || type == 3)){
				printf("Iterate origin, origin: %s, name: %s\n", inet_ntoa(f1->origin), o1->nameprefix->name);
                                //origin_temp = o1;
                                o1 = o1->next;
                        }
                }
        }

}

/*int main(){
	
	// Test fib hash table operations
	struct hash *fib_hash = hash_create (fib_hash_key_make, fib_hash_cmp);
	struct fibtable_entry *fe = (struct fibtable_entry *)mycalloc(sizeof(struct fibtable_entry), 1);		
	struct name_prefix *np = (struct name_prefix *)mycalloc(sizeof(struct name_prefix), 1);
	np->name = "Hello_world";
        np->length = strlen("Hello_world");

	// test copy_name_prefix(struct name_prefix *name)	
	struct name_prefix *np0 = copy_name_prefix(np);
	free(np);
	fe->nameprefix = np0;
	printf("%s\t%d\n",np0->name, np0->length);

	struct nexthoplist_entry *ne = (struct nexthoplist_entry *)mycalloc(sizeof(struct nexthoplist_entry), 1);
	ne->nexthop.s_addr = inet_addr("200.200.200.200");
	struct fibtable_entry *find = lookup_insert_fibentry(fib_hash, np0, fib_hash_alloc);
	fib_add_nexthop(find, ne->nexthop, 40, 1);
	struct originlist_entry *oe = (struct originlist_entry *)mycalloc(sizeof(struct originlist_entry), 1);
        oe->origin.s_addr = inet_addr("201.201.201.201");
	fib_add_origin(find, oe->origin);
	
	struct nexthoplist_entry *ne1 = (struct nexthoplist_entry *)mycalloc(sizeof(struct nexthoplist_entry), 1);
        ne1->nexthop.s_addr = inet_addr("100.100.100.100");
	find = lookup_insert_fibentry(fib_hash, np0, fib_hash_alloc);
	fib_add_nexthop(find, ne1->nexthop, 20, 1);
	struct originlist_entry *oe1 = (struct originlist_entry *)mycalloc(sizeof(struct originlist_entry), 1);
        oe1->origin.s_addr = inet_addr("101.101.101.101");
        fib_add_origin(find, oe1->origin);
	
	struct nexthoplist_entry *ne2 = (struct nexthoplist_entry *)mycalloc(sizeof(struct nexthoplist_entry),1); 
        ne2->nexthop.s_addr = inet_addr("209.85.229.106");                                                   
	find = lookup_insert_fibentry(fib_hash, np0, fib_hash_alloc);
	fib_add_nexthop(find, ne2->nexthop, 30, 1);
	struct originlist_entry *oe2 = (struct originlist_entry *)mycalloc(sizeof(struct originlist_entry), 1);
        oe2->origin.s_addr = inet_addr("210.100.101.101");
        fib_add_origin(find, oe2->origin);
	
	struct originlist_entry *oe3 = (struct originlist_entry *)mycalloc(sizeof(struct originlist_entry), 1);
        oe3->origin.s_addr = inet_addr("102.102.102.102");
        fib_add_origin(find, oe3->origin);

	find = lookup_fibentry(fib_hash, np0);                                                                     
        printf("Lookup successfully! %s\n", find->nameprefix->name);
	iterate_fib_entry(fib_hash,np0,3);	

	struct in_addr o1;                                                                                        
        o1.s_addr = inet_addr("102.102.102.102");
	fib_delete_origin(find, o1);
	struct in_addr o2;                                                                                        
        o2.s_addr = inet_addr("209.85.229.106");	
	fib_delete_origin(find, o2);
	iterate_fib_entry(fib_hash,np0,3);	

	struct name_prefix *np3 = (struct name_prefix *)mycalloc(sizeof(struct name_prefix),1);
        np3->name = (u_char *)mycalloc(sizeof(u_char), strlen("Helloworld"));
	memcpy(np3->name, "Helloworld", strlen("Helloworld"));
	np3->name = "Helloworld";
        np3->length = strlen("Helloworld");
	find = lookup_insert_fibentry(fib_hash, np3, fib_hash_alloc);
	
	printf("Lookup and insert successfully! %s\n", find->nameprefix->name);
	struct in_addr nexthop;
	nexthop.s_addr = inet_addr("2.2.2.2");
	fib_add_nexthop(find, nexthop, 100, 1);
	struct in_addr nh1;
        nh1.s_addr = inet_addr("7.7.7.7");
        fib_add_nexthop(find, nh1, 200, 1);
	struct in_addr nh2;
        nh2.s_addr = inet_addr("3.3.3.3");
        fib_add_nexthop(find, nh2, 300, 1);
	
	find = lookup_fibentry(fib_hash, np3);
	printf("Lookup successfully! %s\n", find->nameprefix->name);
		
	struct in_addr nh3;
	nh3.s_addr = inet_addr("3.3.3.3");
        fib_add_nexthop(find, nh3, 400, 1);
        iterate_fib_entry(fib_hash,np3,3);
	fib_delete_nexthop(find,nh3);
	iterate_fib_entry(fib_hash,np3,3);	
	delete_fib_entry(fib_hash,np3);
	printf("Delete successfully! \n");
	
	// Test origin hash table operations
	
	struct hash *origintable_hash = hash_create (origin_hash_key_make, origin_hash_cmp);	
	printf("Create origin hash successfully! \n");
	struct origintable_entry *ore = (struct origintable_entry *)mycalloc(sizeof(struct origintable_entry), 1);	
	ore->origin.s_addr = inet_addr("1.1.1.1");
	
	struct origintable_entry *f = lookup_insert_originentry(origintable_hash, ore->origin, origin_hash_alloc);
	printf("Lookup and insert origin hash entry successfully! origin: %s \n", inet_ntoa(f->origin));
	f = lookup_originentry(origintable_hash, ore->origin);		
	printf("Lookup origin hash entry successfully! origin: %s \n", inet_ntoa(f->origin));
	origintable_add_nexthop(f, nexthop, 100, 1);
        origintable_add_nexthop(f, nh1, 200, 1);
        origintable_add_nexthop(f, nh2, 300, 1);
        origintable_add_nexthop(f, nh3, 300, 1);
	iterate_origintable_entry(origintable_hash, ore->origin, 3);	
	//struct in_addr origin;
	//origin.s_addr = inet_addr("1.1.1.1");
	//struct in_addr *orp = &origin;
	origintable_delete_nexthop(f, nh2);
	printf("Delete origin successfully!\n");
	iterate_origintable_entry(origintable_hash, ore->origin, 3);	
	//u_char *name = (u_char *)mycalloc(sizeof(u_char), "HelloWorld");
	//strcpy(name, "HelloWorld");	
	//printf("name: %s, length: %d\n", name, strlen(name));
	struct name_prefix *npx = (struct name_prefix *)mycalloc(sizeof(struct name_prefix),1);
        //npx->name = (u_char *)mycalloc(sizeof(u_char), strlen("Helloworld1"));
        //strcpy(npx->name, "Helloworld1");
        npx->name = "Helloworld1";
        npx->length = strlen("Helloworld1");
	origintable_add_nameprefix(f, npx);
	
	npx->name = "Helloworld4";
        npx->length = strlen("Helloworld4");
	origintable_add_nameprefix(f, npx);

	npx->name = "Helloworld3";
        npx->length = strlen("Helloworld3");
        origintable_add_nameprefix(f, npx);

	npx->name = "Helloworld2";
        npx->length = strlen("Helloworld2");
        origintable_add_nameprefix(f, npx);
	origintable_add_nameprefix(f, npx);
	printf("Add name prefix successfully\n");
	iterate_origintable_entry(origintable_hash, ore->origin, 2);
	origintable_delete_nameprefix(f, "Helloworld2");
	origintable_delete_nameprefix(f, "Helloworld1");
	iterate_origintable_entry(origintable_hash, ore->origin, 3);
	
	struct in_addr addr;
	addr.s_addr = inet_addr("1.1.1.1");
	delete_origintable_entry(origintable_hash, addr);
	printf("Delete origin entry successfully!\n");
	return 0;
}*/

