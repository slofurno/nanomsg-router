#include <stdlib.h>
#include <stdio.h>
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
#include <string.h>
#include <unistd.h>
#include "addresses.h"

int main(const int argc, const char **argv)
{
    
    int request = nn_socket(AF_SP, NN_REQ);
    nn_connect(request, BACKROUTER);

    char msg [100];
    char *buf = "ready for work";
    sprintf(msg, "%s says ready for work", argv[1]);
    int i = 0;
    while(++i < 5){
        printf("%s\n",msg);
        nn_send(request,msg,strlen(msg),0);

        int n = nn_recv(request,&buf,NN_MSG,0);
        printf("%s got response %.*s\n", argv[1], n, buf);

        nn_freemsg(buf);
        sleep(1);
    }
    printf("%s is quitting\n", argv[1]);
    nn_shutdown(request,0);
    return 0;
}
