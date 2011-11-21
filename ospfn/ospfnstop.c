#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>

int main(void)
{
	FILE *fp;
	fp=fopen("/var/run/quagga-state/ospfn.pid","r");
	if(fp!=NULL){
		int pid;
		fscanf(fp,"%d",&pid);
		kill(pid,SIGTERM);
	}
	else{
		perror("ospfnstop is not running or user does not have permission to pid file\n");	
	}
}

