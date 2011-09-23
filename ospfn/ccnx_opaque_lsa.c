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
#include "ccnx_opaque_lsa.h"
#include "utility.h"
#include "uthash.h"
#include "ospfn.h"
#include "lib/table.h"

#include "ospfd/ospf_api.h"
#include "ospfd/ospf_dump.h"
#include "ospfclient/ospf_apiclient.h"
#include "lib/stream.h"




void ccnx_opaque_lsa_print (struct ccnx_opaque_lsa  *col)
{
    struct lsa_header *lsah = &col->lsah;
    int opaque_type=ntohl(lsah->id.s_addr)>>24;

    switch( opaque_type)
    {
        case NAME_OPAQUE_LSA:
            ccnx_name_opaque_lsa_print((struct ccnx_opaque_lsa *)col);
            break;
        case ADJ_OPAQUE_LSA:
            ccnx_adjacent_opaque_lsa_print ((struct ccnx_adjacent_opaque_lsa *) col);
            break;
    }
}

void ccnx_name_opaque_lsa_print (struct ccnx_opaque_lsa  *ol)
{
        // printf ("  Name-Opaque-LSA\n");
        writeLogg(logFile,"  Name-Opaque_LSA\n");
        unsigned int size;
        char *name;
        sscanf(ol->name,"%d",&size);
        name=substring(ol->name,number_width((unsigned int)size),(unsigned int)size);
        //printf(" %s\n",name);
        strncat(name,"\n",1);
        writeLogg(logFile,name);
        free(name);
}


void ccnx_adjacent_opaque_lsa_print (struct ccnx_adjacent_opaque_lsa  *aol)
{

        //printf ("  Adjacent-Opaque-LSA\n");
        writeLogg(logFile,"  Adjacent-Opaque-LSA\n");
        char *nbr,*nbr_cost;
        const char *sep=" |";
        char *rem;
        char *no_adj;
        int num_adj;
        char *adj_list;
        adj_list=substring(aol->adj_list,0,strlen(aol->adj_list));
        //printf(" Adjacent List: %s\n",aol->adj_list);
        no_adj=strtok_r(adj_list,sep,&rem);
        num_adj=atoi(no_adj);

        while (num_adj>0)
                {
                        nbr=strtok_r(NULL,sep,&rem);
                        nbr_cost=strtok_r(NULL,sep,&rem);
                        //printf(" Neighbor: %s Path Cost: %s \n",nbr,nbr_cost);
                        char logMsg[50];
                        sprintf(logMsg," Neighbor: %s Path Cost: %s\n",nbr,nbr_cost);
                        writeLogg(logFile,logMsg);
                        num_adj--;

                }
        //printf(" Neihbors list : %s \n",aol->adj_list);
        free(adj_list);
}

void
 ccnx_lsa_header_dump (struct lsa_header *lsah)
 {
   const char *lsah_type = LOOKUP (ospf_lsa_type_msg, lsah->type);
   char logMsg[50];
   //printf ("  LSA Header\n");
   writeLogg(logFile,"  LSA Header\n");
   //printf ("    LS age %d\n", ntohs (lsah->ls_age));
   //printf ("    Options %d (%s)\n", lsah->options,
     //         ospf_options_dump (lsah->options));
   //printf ("    LS type %d (%s)\n", lsah->type,
   //          (lsah->type ? lsah_type : "unknown type"));
   //printf ("    Link State ID %s\n", inet_ntoa (lsah->id));
     sprintf (logMsg,"    LS age %d\n", ntohs (lsah->ls_age));
     writeLogg(logFile, logMsg);
     sprintf (logMsg,"    Options %d (%s)\n", lsah->options, ospf_options_dump (lsah->options));   
     writeLogg(logFile, logMsg);
     sprintf (logMsg,"    LS type %d (%s)\n", lsah->type, (lsah->type ? lsah_type : "unknown type"));
     writeLogg(logFile, logMsg);
     sprintf (logMsg,"    Link State ID %s\n", inet_ntoa (lsah->id)); 
     writeLogg(logFile, logMsg);
     if( lsah->type == 9 || lsah->type == 10 || lsah->type ==11)
        {
       // printf(" Opaque Type  %d\n" ,ntohl(lsah->id.s_addr)>>24);
        //printf(" Opaque Id  %d\n" ,ntohl(lsah->id.s_addr)& LSID_OPAQUE_ID_MASK);
        sprintf(logMsg," Opaque Type  %d\n" ,ntohl(lsah->id.s_addr)>>24);
        writeLogg(logFile, logMsg);
        sprintf(logMsg," Opaque Id  %d\n" ,ntohl(lsah->id.s_addr)& LSID_OPAQUE_ID_MASK);
        writeLogg(logFile, logMsg);
        }
   //printf ("    Advertising Router %s\n", inet_ntoa (lsah->adv_router));
   //printf ("    LS sequence number 0x%lx\n", (u_long)ntohl (lsah->ls_seqnum));
   //printf ("    LS checksum 0x%x\n", ntohs (lsah->checksum));
   //printf ("    length %d\n", ntohs (lsah->length));
   sprintf (logMsg,"    Advertising Router %s\n", inet_ntoa (lsah->adv_router));
   writeLogg(logFile, logMsg);
   sprintf (logMsg,"    LS sequence number 0x%lx\n", (u_long)ntohl (lsah->ls_seqnum));
   writeLogg(logFile, logMsg);
   sprintf (logMsg,"    LS checksum 0x%x\n", ntohs (lsah->checksum));
   writeLogg(logFile, logMsg);
   sprintf (logMsg,"    length %d\n", ntohs (lsah->length)); 
   writeLogg(logFile, logMsg);
}

void
 ospf_router_lsa_print (struct router_lsa  *rl, u_int16_t length)
 {
   //char buf[BUFSIZ];
   //struct router_lsa *rl;
   int i, len;

   //rl = (struct router_lsa *) STREAM_PNT (s);
   char logMsg[40];
   //printf ("  Router-LSA\n");
   writeLogg(logFile, "  Router-LSA\n");
//   zlog_debug ("    flags %s", 
 //             ospf_router_lsa_flags_dump (rl->flags, buf, BUFSIZ));
   //printf ("    # links %d\n", ntohs (rl->links));
   sprintf (logMsg,"    # links %d\n", ntohs (rl->links)); 
   writeLogg(logFile, logMsg);
   len = ntohs (rl->header.length) - OSPF_LSA_HEADER_SIZE - 4;
   for (i = 0; len > 0; i++)
     {
       //printf ("    Link ID %s\n", inet_ntoa (rl->link[i].link_id));
       //printf ("    Link Data %s\n", inet_ntoa (rl->link[i].link_data));
       //printf ("    Type %d\n", (u_char) rl->link[i].type);
       //printf ("    TOS %d\n", (u_char) rl->link[i].tos);
       //printf ("    metric %d\n", ntohs (rl->link[i].metric));

       sprintf (logMsg,"    Link ID %s\n", inet_ntoa (rl->link[i].link_id));
       writeLogg(logFile, logMsg);
       sprintf (logMsg,"    Link Data %s\n", inet_ntoa (rl->link[i].link_data));
       writeLogg(logFile, logMsg);
       sprintf (logMsg,"    Type %d\n", (u_char) rl->link[i].type);
       writeLogg(logFile, logMsg);
       sprintf (logMsg,"    TOS %d\n", (u_char) rl->link[i].tos);
       writeLogg(logFile, logMsg);
       sprintf (logMsg,"    metric %d\n", ntohs (rl->link[i].metric));
       writeLogg(logFile, logMsg);       

       len -= 12;
     }
 }

void update_opaque_lsa(struct ccnx_opaque_lsa *col)
{
        struct lsa_header *lsah = &col->lsah;
        int opaque_type=ntohl(lsah->id.s_addr)>>24;

        switch( opaque_type)
        {
                case NAME_OPAQUE_LSA:
                        update_name_opaque_lsa((struct ccnx_opaque_lsa *) col);
                        break;
                case ADJ_OPAQUE_LSA:
                        break;  
        }
}

void update_name_opaque_lsa(struct ccnx_opaque_lsa *col)
{

	//call yaoqing's function here

    //printf("\n Update _ name opaque lsa called \n");
    writeLogg(logFile, "  Update_name_lsa called \n");
    struct name_prefix *np=(struct name_prefix *)malloc(sizeof(struct name_prefix));
    unsigned int size;
    char *name;
    sscanf(col->name,"%d",&size);
    name=substring(col->name,number_width((unsigned int)size),(unsigned int)size);
    //printf(" %s\n",name);
    np->name=(u_char *)name;
    np->length=size;

    add_new_name_prefix(oclient, prefix_table, np, origin_table, &col->lsah.adv_router);
}


void delete_opaque_lsa(struct ccnx_opaque_lsa *col)
{

        struct lsa_header *lsah = &col->lsah;
        int opaque_type=ntohl(lsah->id.s_addr)>>24;

        switch( opaque_type)
        {
                case NAME_OPAQUE_LSA:
                        delete_name_opaque_lsa((struct ccnx_opaque_lsa *) col);
                        break;
                case ADJ_OPAQUE_LSA:
                        break;
        }
}

void delete_name_opaque_lsa(struct ccnx_opaque_lsa * col)
{

	//call yaoqing's function form here	
	//printf("\n Delete  _ name opaque lsa called \n");
	writeLogg(logFile, " Delete _name opaque lsa called \n");
        struct name_prefix *np=(struct name_prefix *)malloc(sizeof(struct name_prefix));
	unsigned int size;
        char *name;
        sscanf(col->name,"%d",&size);
        name=substring(col->name,number_width((unsigned int)size),(unsigned int)size);
        //printf(" %s\n",name);
        np->name=(u_char *)name;
	np->length=size; 
	//free(name);
    delete_name_prefix(prefix_table, np, origin_table, &col->lsah.adv_router);
}
