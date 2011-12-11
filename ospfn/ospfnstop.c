/* 
 *  ospfnstop -- stop ospfn 
 */

#include <zebra.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int main(int argc, char *argv[]){

	//printf("%s\n",PATH_OSPFN_PID);
	FILE *fp;
        int pid;	
	fp=fopen(PATH_OSPFN_PID,"r");
	if(fp!=NULL){
		fscanf(fp,"%d",&pid);
		//printf("%d\n",pid);	
		//printf("Sending signal to kill ospfn\n");
		kill(pid, SIGTERM);	
	}
	else 
	  fprintf(stderr, "ospfnstop: No process running or user does not have permission to open the pid file\n");
	return 0;
}



