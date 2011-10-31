/* 
 * OSPFN program designed to manipulate CCNX fib 
 */

/* The following includes are needed in all OSPF API client
   applications. */
#include <zebra.h>
#include "prefix.h" /* needed by ospf_asbr.h */
#include "privs.h"
#include "log.h"
//header added for ospfn implementation
#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include<stdlib.h>

// header for making daemon process
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

//header for ospfnstop response
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/un.h>

#include "getopt.h"
#include "ospfd/ospfd.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_lsa.h"
#include "lib/table.h"

#include "ccnx_opaque_lsa.h"
#include "utility.h"
#include "ospfn.h"

#include "ospfd/ospf_opaque.h"
#include "ospfd/ospf_api.h"
#include "ospfclient/ospf_apiclient.h"
#include "lib/stream.h"

#include <ccn/ccn.h>

static u_int32_t counter = 1;
static u_int32_t scount = 1;
static int opaque_id=1;

static int CCN_MAX_NEXT_HOPS=2;
static char OSPFN_DEFAULT_CONFIG_FILE[] = "ospfn.conf";
static char OSPFN_LOCAL_HOST[] = "127.0.0.1";
char *loggingDir;
/* privileges struct. 
 * set cap_num_* and uid/gid to nothing to use NULL privs
 * as ospfapiclient links in libospf.a which uses privs.
 */
struct zebra_privs_t ospfd_privs =
{
    .user = NULL,
    .group = NULL,
    .cap_num_p = 0,
    .cap_num_i = 0
};

struct option longopts[] =
{
    { "daemon",      no_argument,       NULL, 'd'},
    { "config_file", required_argument, NULL, 'f'},
    { "help",        no_argument,       NULL, 'h'},
    { "log",         no_argument,       NULL, 'n'},
    { 0 }
};    

struct ccn_neighbors *neighbors=NULL;

/* The following includes are specific to this application. For
   example it uses threads from libzebra, however your application is
   free to use any thread library (like pthreads). */

#include "ospfd/ospf_dump.h" /* for ospf_lsa_header_dump */
#include "thread.h"
#include "log.h"




/*------------------------------------------*/
/*    FUNCTION DEFINITON                    */
/*------------------------------------------*/

//functions for processing ccn_neighbors list

static void add_neighbor(char *neighbor, int cost)
{
    struct ccn_neighbors *temp;
    temp=(struct ccn_neighbors *) malloc(sizeof(struct ccn_neighbors));

    inet_aton(neighbor,&temp->address);
    temp->path_cost=cost;

    //checking of empty list
    if(neighbors == NULL)
    {
        neighbors=temp;
        neighbors->next=NULL;
    }
    else
    {
        temp->next=neighbors;
        neighbors=temp;
    }
}

static void print_ccn_neighbors()
{
    struct ccn_neighbors *current_neighbor;
    current_neighbor=neighbors;

    if(current_neighbor == NULL)
    {
       // printf("\n CCN Neighbors List is Empty\n");
	writeLogg(logFile,"CCN Neighbor List is Empty\n");   	
    }
    else 
    {
        //printf("\n CCN Neighbors in the List :\n");
        writeLogg(logFile,"CCN Neighbors in the List: \n"); 
	while(current_neighbor!= NULL)
        {		
           	char logMsg[50];
		sprintf(logMsg,"Neighbor: %s Path Cost: %d\n",inet_ntoa(current_neighbor->address), current_neighbor->path_cost);
	   	writeLogg(logFile,logMsg);
		current_neighbor=current_neighbor->next;		
        }		
        //printf("\n");	
    }
}




//------------- Function Added for Ospfn Implementation ------------------------
static int usage(char *progname)
{

    printf("Usage: %s [OPTIONS...]\n\
Announces name prefix LSAs and modifies ccnd FIB.\n\n\
-d, --daemon        Run in daemon mode\n\
-f, --config_file   Specify configuration file name\n\
-h, --help          Display this help message\n", progname);

    exit(1);
}



void process_command_ccnmaxnexthops(char *command)
{
    if(command==NULL)
    {
        //printf(" Wrong Command Format ( ccnmaxnexthops num)\n");
        writeLogg(logFile,"Wrong Command Format [ccnmaxnexthops num]\n");
	return;
    }
    char *rem;
    const char *sep=" \t\n";
    int ccn_max_next_hops;
    char *hops;

    hops=strtok_r(command,sep,&rem);
    if(hops==NULL)
    {
        //printf(" Wrong Command Format ( ccnmaxnexthops num)\n");
        writeLogg(logFile,"Wrong Command Format [ccnmaxnexthops num]\n");
	return;
    }
    ccn_max_next_hops=atoi(hops);	

    CCN_MAX_NEXT_HOPS=ccn_max_next_hops;

}

void process_command_ccnneighbor(char *command)
{
    if(command==NULL)
    {
        //printf(" Wrong Command Format ( ccnneighbor address cost)\n");
        writeLogg(logFile," Wrong Command Format [ ccnneighbor address cost]\n");
	return;
    }
    char *rem;
    const char *sep=" \t\n";

    char *address;
    char *pcost;
    int path_cost;
    struct in_addr neighbor;
    int check;
    //processing of ccn`neighbors here
    address=strtok_r(command,sep,&rem);
    if(address==NULL)
    {
       // printf(" Wrong Command Format ( ccnneighbor address cost)\n");
        writeLogg(logFile,"Wrong Command Format [ ccnneighbor address cost]\n");
	return;
    }
    check=inet_aton(address,&neighbor);
    if(check==0){
        //printf(" Wrong address format\n");
        writeLogg(logFile,"Invalid Address format\n");
	return;
    }
    pcost=strtok_r(NULL,sep,&rem);
    if(pcost==NULL)
    {
        //printf(" Wrong Command Format ( ccnneighbor address cost)\n");
        writeLogg(logFile,"Wrong Command Format [ ccnneighbor address cost]\n");
	return;
    }
    path_cost=atoi(pcost);

    add_neighbor(address, path_cost);
    print_ccn_neighbors();

}

void process_command_ccnname(char *command)
{
    if(command==NULL)
    {
        //printf(" Wrong Command Format ( ccnname /name/prefix opaque_id)\n");
        writeLogg(logFile,"Wrong Command Format [ ccnname /name/prefix opaque_id]\n");
	return;
    }
    char *rem;
    const char *sep=" \t\n";
    char *name;	
    char *opId;
    unsigned int op_id;
    struct name_prefix *np=(struct name_prefix *)malloc(sizeof(struct name_prefix));
    // procesing of ccn name commands here 
    name=strtok_r(command,sep,&rem);
    if(name==NULL)
    {
        //printf(" Wrong Command Format ( ccnname /name/prefix opaque_id)\n");
        writeLogg(logFile,"Wrong Command Format [ ccnname /name/prefix opaque_id\n");
	return;
    }
    opId=strtok_r(NULL,sep,&rem);
    if(opId==NULL)
    {
        //printf(" Wrong Command Format ( ccnname /name/prefix opaque_id)\n");
        writeLogg(logFile,"Wrong Command Format [ ccnname /name/prefix opaque_id\n");
	return;
    }
    op_id=atoi(opId);
    np->length=strlen(name);
    np->name=(u_char *)name;	

    inject_name_opaque_lsa(np,op_id);	

}

void process_command_logdir(char *command)
{
        if(command==NULL)
        {
        printf(" Wrong Command Format ( logdir /path/to/logdir )\n");
        //writeLogg(logFile,"Wrong Command Format [logdir /path/to/logdir]\n");
        return;
        }
        char *rem;
        const char *sep=" \t\n";
        char *dir;

        dir=strtok_r(command,sep,&rem);

        if(dir==NULL)
        {
        printf(" Wrong Command Format ( logdir /path/to/logdir/ )\n");
        //writeLogg(logFile,"Wrong Command Format [logdir  /path/to/logdir]\n");
        return;
        }
        loggingDir=strdup(dir);
        //printf(" %s \n",loggingDir);
}



void process_conf_command(char *command,int isLogOnlyProcessing)
{
    const char *separators=" \t\n";
    char *remainder=NULL;
    char *cmd_type=NULL;

    if(command==NULL || strlen(command)==0)
        return;	

    cmd_type=strtok_r(command,separators,&remainder);
    //if(cmd_type!=NULL)
      //  printf("\n%s %s",cmd_type,remainder);

    if(!strcmp(cmd_type, "ccnmaxnexthops") )
      {
       if (isLogOnlyProcessing ==0)
        process_command_ccnmaxnexthops(remainder);
      }
    else if(!strcmp(cmd_type,"ccnneighbor")) 
      { 
      if (isLogOnlyProcessing == 0)
        process_command_ccnneighbor(remainder);
      }
    else if(!strcmp(cmd_type,"ccnname") )
       {
       if (isLogOnlyProcessing == 0)
        process_command_ccnname(remainder);
       }
    else if(!strcmp(cmd_type,"logdir") )
       {
       if (isLogOnlyProcessing == 1)
        process_command_logdir(remainder);
       }
    else 
	{
	writeLogg(logFile,"Wrong configuration command\n");
	printf("Wrong configuration Command %s \n",cmd_type);
	}
}

int readConfigFile(char *filename, int isLogOnlyProcessing)
{
    FILE *cfg;
    char buf[1024];
    int len;

    //printf("\n %s ", filename);	

    cfg=fopen(filename, "r");

    if(cfg == NULL){
        printf("\nConfiguration File does not exists\n");
        exit(1);	
    }

    while(fgets((char *)buf, sizeof(buf), cfg)){
        len=strlen(buf);
        if(buf[len-1] == '\n')
            buf[len-1]='\0';		
        //printf("\n%s",buf);
       // printf("\n");
       writeLogg(logFile,"%s\n",buf);
       //writeLogg(logFile,"\n"); 
       process_conf_command(buf,isLogOnlyProcessing);	
    }

    fclose(cfg);

    return 0;
}

//------------------------------------------------------------------------------


/* ---------------------------------------------------------
 * Threads for asynchronous messages and LSA update/delete 
 * ---------------------------------------------------------
 */

/*
static int lsa_delete (struct thread *t)
{
    struct ospf_apiclient *oclient;
    struct in_addr area_id;
    int rc;

    oclient = THREAD_ARG (t);

    inet_aton (args[6], &area_id);

    printf ("Deleting LSA... ");
    rc = ospf_apiclient_lsa_delete (oclient, 
            area_id, 
            atoi (args[2]),
            atoi (args[3]),
            atoi (args[4]));
    printf ("done, return code is = %d\n", rc);
    return rc;
}

static int lsa_inject(struct thread *t)
{
    struct ospf_apiclient *cl;
    struct in_addr ifaddr;
    struct in_addr area_id;
    u_char lsa_type;
    u_char opaque_type;
    u_int32_t opaque_id;
    void *opaquedata;
    int opaquelen;

    int rc;

    cl = THREAD_ARG (t);

    inet_aton ("127.0.0.1", &ifaddr);
    inet_aton ("0", &area_id);
    lsa_type = 10;
    opaque_type = 234;
    opaque_id =1234; 
    opaquedata = &counter;
    opaquelen = sizeof (u_int32_t);


    printf ("Originating/updating LSA with counter=%d... ", counter);
    rc = ospf_apiclient_lsa_originate(cl, ifaddr, area_id,
            lsa_type,
            opaque_type, opaque_id,
            opaquedata, opaquelen);

    printf ("done, return code is %d\n", rc);

    counter++;

    return 0;
}
*/

/* This thread handles asynchronous messages coming in from the OSPF
   API server */
static int lsa_read (struct thread *thread)
{
    struct ospf_apiclient *oclient;
    int fd;
    int ret;

    //printf ("lsa_read called\n");
    writeLogg(logFile,"lsa_read called\n");
    oclient = THREAD_ARG (thread);
    fd = THREAD_FD (thread);

    /* Handle asynchronous message */
    ret = ospf_apiclient_handle_async (oclient);
    if (ret < 0) {
        //printf ("Connection closed, exiting...");
        writeLogg(logFile," OSPFD Connection closed, exiting...\n");
	exit(0);
    }

    /* Reschedule read thread */
    thread_add_read (master, lsa_read, oclient, fd);
    return 0;
}

/* ---------------------------------------------------------
 * Callback functions for asynchronous events 
 * ---------------------------------------------------------
 */

static void lsa_update_callback (struct in_addr ifaddr, struct in_addr area_id,
        u_char is_self_originated,
        struct lsa_header *lsa)
{
    //printf ("lsa_update_callback: ");
    //printf ("ifaddr: %s ", inet_ntoa (ifaddr));
    //printf ("area: %s\n", inet_ntoa (area_id));
    //printf ("is_self_origin: %u\n", is_self_originated);

    char logMsg[25];
    writeLogg(logFile,"lsa_update_callback: \n");
    sprintf (logMsg,"ifaddr: %s ", inet_ntoa (ifaddr));
    writeLogg(logFile,"%s\n",logMsg);
    sprintf (logMsg,"area: %s", inet_ntoa (area_id));
    writeLogg(logFile,"%s\n",logMsg);
    sprintf (logMsg,"is_self_origin: %u\n", is_self_originated);
    writeLogg(logFile, logMsg); 
	/* It is important to note that lsa_header does indeed include the
       header and the LSA payload. To access the payload, first check
       the LSA type and then typecast lsa into the corresponding type,
       e.g.:

       if (lsa->type == OSPF_ROUTER_LSA) {
       struct router_lsa *rl = (struct router_lsa) lsa;
       ...
       u_int16_t links = rl->links;
       ...
       }
       */

    ccnx_lsa_header_dump (lsa);

    if(lsa->type == OSPF_ROUTER_LSA)
    {    
        ospf_router_lsa_print((struct router_lsa *) lsa, lsa->length);
    } 
    if(lsa->type == 9 || lsa->type == 10 || lsa->type == 11)
    {
        update_opaque_lsa((struct ccnx_opaque_lsa *) lsa);
        ccnx_opaque_lsa_print((struct ccnx_opaque_lsa *) lsa);
    }
}

static void lsa_delete_callback (struct in_addr ifaddr, struct in_addr area_id,
        u_char is_self_originated,
        struct lsa_header *lsa)
{
    
    //printf ("lsa_delete_callback: ");
    //printf ("ifaddr: %s ", inet_ntoa (ifaddr));
    //printf ("area: %s\n", inet_ntoa (area_id));
    //printf ("is_self_origin: %u\n", is_self_originated);
    
    char logMsg[25];
    writeLogg(logFile,"lsa_delete_callback: ");
    sprintf (logMsg,"ifaddr: %s ", inet_ntoa (ifaddr));
    writeLogg(logFile,logMsg);
    sprintf (logMsg,"area: %s\n", inet_ntoa (area_id));
    writeLogg(logFile,logMsg);
    sprintf (logMsg,"is_self_origin: %u\n", is_self_originated);
    writeLogg(logFile, logMsg);

	
    ccnx_lsa_header_dump (lsa);

    if(lsa->type == 9 || lsa->type == 10 || lsa->type == 11)
    {
        delete_opaque_lsa((struct ccnx_opaque_lsa *) lsa);
    }
}

static void ready_callback (u_char lsa_type, u_char opaque_type, struct in_addr addr)
{
   // printf ("ready_callback: lsa_type: %d opaque_type: %d addr=%s\n",
     //       lsa_type, opaque_type, inet_ntoa (addr));
    char logMsg[75];
    sprintf (logMsg,"ready_callback: lsa_type: %d opaque_type: %d addr=%s\n",lsa_type, opaque_type, inet_ntoa (addr));
    writeLogg(logFile,logMsg);
    /* Schedule opaque LSA originate in 5 secs */
    //thread_add_timer (master, lsa_inject, oclient, 5);

    /* Schedule opaque LSA update with new value */
    //thread_add_timer (master, lsa_inject, oclient, 10);

    /* Schedule delete */
    //thread_add_timer (master, lsa_delete, oclient, 30);
}

static void new_if_callback (struct in_addr ifaddr, struct in_addr area_id)
{
    //printf ("new_if_callback: ifaddr: %s ", inet_ntoa (ifaddr));
    //printf ("area_id: %s\n", inet_ntoa (area_id));
    char logMsg[40];
    sprintf (logMsg,"new_if_callback: ifaddr: %s ", inet_ntoa (ifaddr));
    writeLogg(logFile,logMsg);
    sprintf (logMsg,"area_id: %s\n", inet_ntoa (area_id));
    writeLogg(logFile,logMsg);
}


static void del_if_callback (struct in_addr ifaddr)
{
    //printf ("new_if_callback: ifaddr: %s\n ", inet_ntoa (ifaddr));
    char logMsg[40];
    sprintf (logMsg,"new_if_callback: ifaddr: %s\n ", inet_ntoa (ifaddr));
    writeLogg(logFile,logMsg);
}

static void ism_change_callback (struct in_addr ifaddr, struct in_addr area_id,
        u_char state)
{
    //printf ("ism_change: ifaddr: %s ", inet_ntoa (ifaddr));
    //printf ("area_id: %s\n", inet_ntoa (area_id));
    //printf ("state: %d [%s]\n", state, LOOKUP (ospf_ism_state_msg, state));
    char logMsg[40]; 
    sprintf (logMsg,"ism_change: ifaddr: %s ", inet_ntoa (ifaddr));
    writeLogg(logFile,logMsg);
    sprintf (logMsg,"area_id: %s\n", inet_ntoa (area_id));
    writeLogg(logFile, logMsg);
    sprintf (logMsg,"state: %d [%s]\n", state, LOOKUP (ospf_ism_state_msg, state));
    writeLogg(logFile, logMsg);
}


static void nsm_change_callback (struct in_addr ifaddr, struct in_addr nbraddr,
        struct in_addr router_id, u_char state)
{
    //printf ("nsm_change: ifaddr: %s ", inet_ntoa (ifaddr));
    //printf ("nbraddr: %s\n", inet_ntoa (nbraddr));
    //printf ("router_id: %s\n", inet_ntoa (router_id));
    //printf ("state: %d [%s]\n", state, LOOKUP (ospf_nsm_state_msg, state));
    char logMsg[40];
    sprintf (logMsg,"nsm_change: ifaddr: %s ", inet_ntoa (ifaddr));
    writeLogg(logFile,logMsg);
    sprintf (logMsg,"nbraddr: %s\n", inet_ntoa (nbraddr));
    writeLogg(logFile,logMsg);
    sprintf (logMsg,"router_id: %s\n", inet_ntoa (router_id));
    writeLogg(logFile,logMsg);
    sprintf (logMsg,"state: %d [%s]\n", state, LOOKUP (ospf_nsm_state_msg, state));
    writeLogg(logFile,logMsg);
}

/* 08/29/2011 yic+ callback function for router routing table change */
static void nexthop_change_callback(struct in_addr router_id, int nexthop_count, struct in_addr *nexthops)
{
    struct origintable_entry *oe;
    struct nameprefix_entry *ne;
    struct prefixtable_entry *fe;

    oe = origin_hash_get(origin_table, &router_id);

    update_origin_nexthop_list(oe, nexthop_count, nexthops);

    ne = oe->nameprefix_list;
    while (ne)
    {
        fe = prefix_hash_get(prefix_table, ne->nameprefix);
        update_name_prefix_nexthop_list(fe, origin_table);
        ne = ne->next;
    }
}

static void process_adjacent()
{

    thread_add_timer (master,inject_adjacency_opaque_lsa, oclient, 5);
}


int inject_adjacency_opaque_lsa (struct thread *t)
{
    struct ospf_apiclient *cl;
    struct in_addr ifaddr;
    struct in_addr area_id;
    u_char lsa_type;
    u_char opaque_type;
    void *opaquedata;
    int opaquelen;
    int rc;
    char buff[30];
    char data[4048];
    data[0]='\0';
    char tmp_data[4058]; 
    cl = THREAD_ARG (t);

    //printf(" Write Logg from Inject Adj OLSA : %s \n",logFile);
    inet_aton ("127.0.0.1", &ifaddr);
    inet_aton ("0", &area_id);
    lsa_type = 10;
    opaque_type = 234;

    struct ccn_neighbors *current_neighbor;
    current_neighbor=neighbors;
    int count=0;

    if(current_neighbor == NULL)
    {
        //printf("\n CCN Neighbors List is Empty\n");
        writeLogg(logFile,"CCN Neighbors List is Empty\n");
    }
    else
    {
        //printf("\n CCN Neighbors in the List :\n");
        writeLogg(logFile,"CCN Neighborrs List is: \n");
        while(current_neighbor!= NULL)
        {
            sprintf(buff,"%s|%d|",inet_ntoa(current_neighbor->address),current_neighbor->path_cost);
            strcat(data,buff);
            strcpy(buff,"");
            count++;
            current_neighbor=current_neighbor->next;
        }
        //printf("\n");
    }
    sprintf(tmp_data,"%d|%s",count,data);
    strcpy(tmp_data,align_data(tmp_data));
    writeLogg(logFile,"%s\n",tmp_data);
    //printf("%s\n",tmp_data);
    opaquelen=strlen(tmp_data);
    opaquedata=&tmp_data;

    //printf ("Originating/updating  Adj LSA with counter=%d... ", counter);
    char logMsg[60];
    sprintf (logMsg,"Originating/updating  Adj LSA with counter=%d... ", counter);
    writeLogg(logFile,logMsg);
    rc = ospf_apiclient_lsa_originate(cl, ifaddr, area_id,
            lsa_type,
            opaque_type, opaque_id,
            opaquedata, opaquelen);

    //printf ("done, return code is %d\n", rc);
    sprintf (logMsg,"done, return code is %d\n", rc);
    writeLogg(logFile,logMsg);
    opaque_id++;
    counter++;

    return 0;
}


void inject_name_opaque_lsa(struct name_prefix *np, unsigned int op_id )
{
    struct ospf_apiclient *cl;
    struct in_addr ifaddr;
    struct in_addr area_id;
    u_char lsa_type;
    u_char opaque_type;
    void *opaquedata;
    int opaquelen;
//    unsigned int name_size;
    char *tname;

    char temp[np->length+number_width(np->length)+4];
    char lsa_names[np->length+number_width(np->length)+4];
    int rc;

    //cl = THREAD_ARG (t);
    cl=oclient;
    inet_aton ("127.0.0.1", &ifaddr);
    inet_aton ("0", &area_id);
    lsa_type = 10;
    opaque_type = 235;

    sprintf(temp,"%d%s",(int)strlen((char *)np->name),(char *)np->name);
    //printf("\n %s ", temp);
    strcpy(lsa_names,align_data(temp));
    //printf("\n %s ",lsa_names);
    strcpy(temp,"");
    tname=lsa_names;
    opaquedata=&lsa_names;
    char logMsg[15+strlen(lsa_names)];
    //printf("\n OpaqueData:%s \n",(char *) opaquedata);
    sprintf(logMsg," OpaqueData:%s \n",(char *) opaquedata);
    writeLogg(logFile,logMsg);
    opaquelen=strlen(lsa_names);
    //printf ("Originating/updating LSA with counter=%d... ", counter);
    sprintf (logMsg,"Originating/updating LSA with counter=%d... \n", counter);
    writeLogg(logFile,logMsg);
    rc = ospf_apiclient_lsa_originate(cl, ifaddr, area_id,
            lsa_type,
            opaque_type, op_id,
            opaquedata, opaquelen);

    //printf ("done, return code is %d\n", rc);
    sprintf (logMsg,"done, return code is %d\n", rc);
    writeLogg(logFile,logMsg);
    counter++;




}
/*----------------------------------------------------------*/
/*                     OSPFNSTOP RESPONSE                   */
/*----------------------------------------------------------*/

void setnonblocking(int sock)
{
	int opts;

	opts = fcntl(sock,F_GETFL);
	if (opts < 0) {
		perror("fcntl(F_GETFL)");
		exit(1);
	}
	opts = (opts | O_NONBLOCK);
	if (fcntl(sock,F_SETFL,opts) < 0) {
		perror("fcntl(F_SETFL)");
		exit(1);
	}
	return;
}

int get_ospfnstop_sock(void){

	struct sockaddr_un server_address; /* bind info structure */
	int reuse_addr = 1;  /* Used so we can re-bind to our port
				while a previous connection is still
				in TIME_WAIT state. */
	int sock;            /* The socket file descriptor for our "listening"
                   	socket */
	/* Obtain a file descriptor for our "listening" socket */
	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
	 	return 0;	
		//exit(EXIT_FAILURE);
	}
	/* So that we can re-bind to it without TIME_WAIT problems */
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,
		sizeof(reuse_addr));

	/* Set socket to non-blocking with our setnonblocking routine */
	setnonblocking(sock);

	memset((char *) &server_address, 0, sizeof(server_address));
	server_address.sun_family = AF_UNIX;
	strcpy(server_address.sun_path,"/tmp/ospfn.sock");
	unlink(server_address.sun_path);
	if (bind(sock, (struct sockaddr *) &server_address,
	  sizeof(server_address)) < 0 ) {
		close(sock);
	  	return 0;	
	}

	/* Set up queue for incoming connections. */
	listen(sock,1);
	return sock;
}

int ospfnstop(struct thread *t){

	printf("ospfnstop called: %4d\n",scount);
	writeLogg(logFile,"ospfnstop called: %4d\n",scount++);	
	fd_set socks;      
	struct timeval timeout;  
	int readsocks;	     

	FD_ZERO(&socks);
	FD_SET(ospfnstop_sock,&socks);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
		
	readsocks=select(ospfnstop_sock+1, &socks, (fd_set *) 0, (fd_set *) 0, &timeout);
	if (readsocks > 0){
		//printf("x\n");
	 	writeLogg(logFile,"Signal from ospfnstop\n");
		hash_iterate_delete_npt (prefix_table);	
		close(ospfnstop_sock);
		remove("/tmp/ospfn.sock");	
		exit(0);
	}

	thread_add_timer (master, ospfnstop, oclient,5);	
	return 0;
}


/* ---------------------------------------------------------
 * Main program 
 * ---------------------------------------------------------
 */

int main(int argc, char *argv[])
{
    //struct for connecting to Quagga OSPF	
    struct thread main_thread;
    int res;
    int daemon_mode = 0;
    int isLoggingEnabled = 1;
    char *config_file = OSPFN_DEFAULT_CONFIG_FILE;
    

    while ((res = getopt_long(argc, argv, "df:hn", longopts, 0)) != -1) {
        switch (res) {
            case 'd':
                daemon_mode = 1;
                break;
            case 'f':
                config_file = optarg;
                break;
            case 'n':
		isLoggingEnabled = 0;
		break;
            case 'h':
            default:
                usage(argv[0]);
        }
    }

    /* Initialization */
    zprivs_init (&ospfd_privs);
    master = thread_master_create ();

    origin_table = origin_hash_create();
    prefix_table = prefix_hash_create();
    ospfnstop_sock=get_ospfnstop_sock();
    readConfigFile(config_file,1);

  // printf("%s from main\n",loggingDir);
 
    if(isLoggingEnabled)
     logFile=startLogging( loggingDir );

    //printf("main:%s\n",logFile);	
    
    ccn_handle = ccn_create();
    res = ccn_connect(ccn_handle, NULL); 
    if (res < 0) {
        ccn_perror(ccn_handle, "Cannot connect to ccnd.");
        exit(1);
    }

    /* Open connection to OSPF daemon */
    //printf("\nConnecting to OSPF daemon........\n");
    writeLogg(logFile,"Connecting to OSPF daemon ............\n");
    oclient = ospf_apiclient_connect (OSPFN_LOCAL_HOST, ASYNCPORT);
    if (!oclient)
    {
        printf ("Connecting to OSPF daemon on 127.0.0.1 failed!\n");
        writeLogg(logFile, "Connecting to OSPF daemon on host: 127.0.0.1 failed!!\n");
        exit (1);
    }
    else writeLogg(logFile,"Connection to OSPF established. \n");

    /* Register callback functions. */
    ospf_apiclient_register_callback (oclient,
            ready_callback,
            new_if_callback,
            del_if_callback,
            ism_change_callback,
            nsm_change_callback,
            lsa_update_callback,
            lsa_delete_callback,
            nexthop_change_callback);

    /* Register LSA type and opaque type. */
    ospf_apiclient_register_opaque_type (oclient, 10, ADJ_OPAQUE_LSA); // registering adjacency opaque lsa
    ospf_apiclient_register_opaque_type (oclient, 10, NAME_OPAQUE_LSA); // registering name opaque lsa

    /* Synchronize database with OSPF daemon. */
    ospf_apiclient_sync_lsdb (oclient);

    readConfigFile(config_file,0);	

    //insert code here for processing neighbors and ccnnames and call them 
    process_adjacent();

    thread_add_timer (master, ospfnstop, oclient,5);
    thread_add_read (master, lsa_read, oclient, oclient->fd_async);
    //thread_add_timer (master, ospfnstop, oclient,5);

    if (daemon_mode)
        daemon(0, 0);

    while (1)
    {
        thread_fetch (master, &main_thread);
        thread_call (&main_thread);
    }


    return 0;		
}

