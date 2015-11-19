#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
#include <libmill.h>
#include <string.h>
#include <unistd.h>
#include "../sus-stack/slice.c"
#include "addresses.h"

typedef struct header header;
struct header{
    int len;
    char *header;
};

coroutine void collect(chan workers, int back_router)
{
    //int back_router = nn_socket(AF_SP_RAW, NN_REP);
    //nn_bind(back_router, BACKROUTER);

    int ready;
    size_t ready_sz = sizeof (ready);

    nn_getsockopt(back_router, NN_SOL_SOCKET, NN_RCVFD, &ready, &ready_sz);

    struct nn_msghdr hdr;
    char *body = malloc(sizeof(char)*128);

    while(1){

        char *ctrl = malloc(sizeof(char)*64);
       
        memset(&hdr, 0, sizeof(hdr)); 

        struct nn_iovec iovec;
        iovec.iov_base = body;
        iovec.iov_len = 128;

        hdr.msg_iov = &iovec;
        hdr.msg_iovlen = 1;
        hdr.msg_control = ctrl;
        hdr.msg_controllen = 64;

        printf("waiting for worker to report in..\n");
        fdwait(ready, FDW_IN, -1);
        int rc = nn_recvmsg(back_router, &hdr, NN_DONTWAIT);
        printf("got msg: %.*s\n", hdr.msg_iov->iov_len, (char*) hdr.msg_iov->iov_base);
        chs(workers, char*, ctrl); 
    }

}

coroutine void generate_work(chan jobs)
{

    int c = 0;
    while(1){

        char *buf = malloc(sizeof(char)*64);
        sprintf(buf, "work item number %d", ++c);
        printf("enqueuing %s\n",buf);
        chs(jobs, char*, buf);

        msleep(now() + 4000);

    }

}
coroutine void route_work(chan workers, int back_router, chan jobs)
{
    while(1){

        msleep(now() + 4000);
        printf("i've got work to do\n");

        char *buf = chr(jobs, char*);
        char *worker_header = chr(workers, char*);

        printf("i've got a worker available\n");
        struct nn_msghdr hdr; 
        hdr.msg_control = worker_header;
        hdr.msg_controllen = 64;

        printf("sending msg: %s w/ len %d\n", buf, strlen(buf));
        struct nn_iovec iovec;
        iovec.iov_base = buf;
        iovec.iov_len = strlen(buf);

        hdr.msg_iov = &iovec;
        hdr.msg_iovlen = 1;

        nn_sendmsg(back_router, &hdr, NN_DONTWAIT);
        printf("work sent\n");
    }
}

int main()
{

    int back_router = nn_socket(AF_SP_RAW, NN_REP);
    nn_bind(back_router, BACKROUTER);

    chan jobs = chmake(char*, 128);
    chan workers = chmake(char*, 64);

    go(collect(workers, back_router));
    go(generate_work(jobs));
    route_work(workers, back_router, jobs); 
    
    int front_router = nn_socket(AF_SP_RAW, NN_REP);
    nn_bind(front_router, FRONTROUTER);
    
    char ctrl [256];
    char body [256];
    char ctrl2 [256];
    char body2 [256];

    struct nn_msghdr hdr;
    struct nn_msghdr hdr2;

    struct nn_iovec iovec;
    iovec.iov_base = body;
    iovec.iov_len = sizeof(body);

    memset(&hdr, 0, sizeof (hdr));
    hdr.msg_iov = &iovec;
    hdr.msg_iovlen = 1;

    hdr.msg_control = ctrl;
    hdr.msg_controllen = sizeof (ctrl);

    int rc = nn_recvmsg(back_router, &hdr, 0);
    printf("got rc: %d, got ctrl: %d\n", rc, hdr.msg_controllen);

    struct nn_iovec iovec2;
    iovec2.iov_base = body2;
    iovec2.iov_len = sizeof(body2);

    memset(&hdr2, 0, sizeof (hdr));

    hdr2.msg_iov = &iovec2;
    hdr2.msg_iovlen = 1;
    hdr2.msg_control = ctrl2;
    hdr2.msg_controllen = sizeof (ctrl2);

    rc = nn_recvmsg(back_router, &hdr2, 0);
    /* 
    struct nn_cmsghdr *cmsg;

    cmsg = NN_CMSG_FIRSTHDR (&hdr);
    while (cmsg != NULL) {
        size_t len = cmsg->cmsg_len - sizeof (struct nn_cmsghdr);
        printf ("level: %d property: %d length: %dB data: ",
            (int) cmsg->cmsg_level,
            (int) cmsg->cmsg_type,
            (int) len);
        unsigned char *data = NN_CMSG_DATA(cmsg);
        while (len) {
            printf ("%02X", *data);
            ++data;
            --len;
        }
        printf ("\n");

        cmsg = NN_CMSG_NXTHDR (&hdr.msg_control, cmsg);
    }
*/
    hdr.msg_iov = &iovec2;
    hdr.msg_iovlen = 1;
    nn_sendmsg(back_router, &hdr, 0);

    /*
    while(1){
        sleep(10);
        buf = NULL;
        printf("waiting for worker\n");

        int n = nn_recv(response, &buf, NN_MSG, 0);

        printf("server got msg: %.*s\n", n, buf);
        sprintf(tevs, "you are number %d served", ++c);
        printf("responding to %d\n",response);
        
        nn_send(response, tevs, strlen(tevs), 0);
        nn_freemsg(buf);
    }
   
    nn_shutdown(response, 0); 
    */
    return 0;
}
