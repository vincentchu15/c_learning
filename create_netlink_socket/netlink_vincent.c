#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/module.h>
#include <linux/rtnetlink.h>
#include <linux/netlink.h>

#define NETLINK_VINCENT 31
#define VINCENT_HELLO 5566

#define VINCENT_HIHI 55
#define VINCENT_YAYA 66

char *cli_msg = "Hello! This is VINCENT from user space";

int main() {
    //Create socket using self define protocol (Need make sure kernel module was inserted first.)
    int sk = socket(AF_NETLINK, SOCK_RAW, NETLINK_VINCENT); //Self define NETLINK_VINCENT is protocol, not type
    if (sk < 0) { //Coressponding kernel module was not inserted
	    printf("Failed to create socket\n");
	    return -1;
    }

    //Step 1: Init client(Me) structure
    struct sockaddr_nl cli_addr; { //a.k.a my name
        memset(&cli_addr, 0, sizeof(cli_addr));
        cli_addr.nl_family = AF_NETLINK;
        cli_addr.nl_pid = getpid(); //We could define any number, if set zero means let kernel decide PID (default consider PID as our source port number)
        cli_addr.nl_groups = 0; //Zero means unicast
    }

    //Step 2: Bind this socket
    if (bind(sk, (struct sockaddr*)&cli_addr, sizeof(cli_addr)) < 0) {
        printf("Failed to bind netlink socket: %s\n", (char*)strerror(errno));
        close(sk);
        return 1;
    }

    //Step 3: Init Server(Kernel) structure
    struct sockaddr_nl server_addr; {
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.nl_family = AF_NETLINK;
        server_addr.nl_pid = 0; //Zero means kernel
        server_addr.nl_groups = 0; //Zero means unicast
    }

    //Step 4: Init message buffer
    char buf[1050];
    struct iovec iov; {//points to buf we define
        iov.iov_base = buf;
        iov.iov_len = sizeof(buf); //Same as buffer size
    }

    //Step 5: Init netlink message header
    struct nlmsghdr *nlh = (struct nlmsghdr*)buf; //nlh always points to buf
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_len = sizeof(buf);
    //*****The most important part. Determine what we want kernel netlink module to do.
    //*****Different type need passing diffrent structure. E.g RTM_XXX
    nlh->nlmsg_type = VINCENT_HELLO;

    //Step 6: Copy our message string into payload part
    strcpy(NLMSG_DATA(nlh), cli_msg);

    printf("[Before] Message to kernel is '%s' , message type is %d, length is %d\n", (char*)NLMSG_DATA(nlh), nlh->nlmsg_type, nlh->nlmsg_len);
    //Step 7: Init protocol header, this msghdr is common structure. Also used in sockaddr_in not only sockaddr_nl (to destination)
    struct msghdr msg; {
        memset(&msg, 0, sizeof(msg));
        msg.msg_name = &server_addr;
        msg.msg_namelen = sizeof(server_addr);
        msg.msg_iov = &iov; //io vector (put our header & payload into it)
        msg.msg_iovlen = 1; //How many number of buffer(In our case: only one buffer)
    }

    //Step 8: Send message to kernel
    for (int i=0;i<3;i++) {
        ssize_t snd_status = sendmsg(sk, &msg, 0);
        printf("Send message '%s' \n", (char*)NLMSG_DATA(nlh));
        if (snd_status < 0) {
            printf("Failed to send message to kernel. Error is %s", strerror(errno));
	    close(sk);
        }
    }

    //Step 9: Waiting for reponse messages from kernel    
    memset(nlh, 0, sizeof(nlh));
    //The return value of recvmsg() is actual return length. And nlh->nlmsg_len means actual message length.
    //So what NLMSG_OK() does is check if rcv_status == nlh->nlmsg_len
#if 0
    ssize_t rcv_status = recvmsg(sk, &msg, 0); 
    printf("\n[After] Message from kernel is '%s' \ntype is %d, payload length is %d, header length is %d, total length is %d\n",
                                        (char*)NLMSG_DATA(nlh),
                                        nlh->nlmsg_type, //type is not 24 as we define. 24 is protocol
                                        nlh->nlmsg_len,
                                        NLMSG_HDRLEN,
                                        NLMSG_LENGTH(nlh->nlmsg_len));
#endif
//    if (rcv_status < 0) {
//        printf("Failed to receive message from kernel. Error is %s", strerror(errno));
//	close(sk);
//	return -1;
//    }

    //Step 10: Check message length
    char *response_type = NULL;
    //printf("rcv_status=%ld, msg_len=%d\n", rcv_status, nlh->nlmsg_len);
    //Check response length, rcv_status should equals to length of response message
    //Unless buffer size we give is smaller than returned message length or something wrong
    while(1) {
        ssize_t rcv_status = recvmsg(sk, &msg, 0); //recvmsg is blocking, so it will keep waiting until next message
        if (NLMSG_OK(nlh, rcv_status)) {
            switch (nlh->nlmsg_type){
                case VINCENT_HIHI:
                    response_type = "HIHI";
                    break;
                case VINCENT_YAYA:
                    response_type = "YAYA";
                    break;
                default:
                    response_type = "Nothing";
            }
            printf("\nReceive message from kernel: '%s' \ntype is %d, payload length is %d, header length is %d, total length is %d\n",
                                        (char*)NLMSG_DATA(nlh),
                                        nlh->nlmsg_type, //type is not 24. 24 is protocol
                                        nlh->nlmsg_len,
                                        NLMSG_HDRLEN,
                                        NLMSG_LENGTH(nlh->nlmsg_len));
            //nlh = NLMSG_NEXT(nlh, nlh->nlmsg_len);
        } else {
            printf("Received message '%s' , type is '%s' (%d), length is %d\n", (char*)NLMSG_DATA(nlh), response_type, nlh->nlmsg_type, nlh->nlmsg_len); 
        };
        usleep(1000000);
    }

    //printf("Received message '%s' , type is '%s' (%d)\n", (char*)NLMSG_DATA(nlh), response_type, nlh->nlmsg_type);
    close(sk);
    return 0;
}
