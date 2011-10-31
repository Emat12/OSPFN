#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCK_PATH "/tmp/ospfn.sock"

int main(void)
{
    int s, len;
    struct sockaddr_un remote;

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        //perror("socket");
        exit(1);
    }

    //printf("Trying to connect...\n");

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, SOCK_PATH);
    //len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    len=sizeof(remote);
    if (connect(s, (struct sockaddr *)&remote, len) == -1) {
        //perror("connect");
        printf("Either no ospfn process running or connection refused\n"); 
	exit(1);
    }

    //printf("Connected.\n");
    len=write(s," ",1);
    //printf("WRITE %d\n",wlen);
    close(s);
    return 0;
}

