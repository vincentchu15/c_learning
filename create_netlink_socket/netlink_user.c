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

int main() {
    int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (fd < 0) {
	    printf("Failed to create socket\n");
	    return 1;
    }

    //Step 1: Init client(this user space program) structure
    struct sockaddr_nl cli_addr; //a.k.a my name
    memset(&cli_addr, 0, sizeof(cli_addr));
    cli_addr.nl_family = AF_NETLINK;
    cli_addr.nl_pid = getpid();
    cli_addr.nl_groups = RTMGRP_IPV4_ROUTE;

    //Step 2: Init message buffer
    char buf[8192];
    struct iovec iov;
    iov.iov_base = buf;
    iov.iov_len = sizeof(buf);

    //Step 3: Init protocol header
    struct msghdr msg;
    {
        msg.msg_name = &cli_addr; //local address
        msg.msg_namelen = sizeof(cli_addr); //address size
        msg.msg_iov = &iov; //io vector
        msg.msg_iovlen = 1; //io size
    }

    //Step 4: Bind socket
    if (bind(fd, (struct sockaddr*)&cli_addr, sizeof(cli_addr)) < 0) {
        printf("Failed to bind netlink socket: %s\n", (char*)strerror(errno));
        close(fd);
        return 1;
    }


    //Step 5: Read all routing related messages from kernel
    printf("Wait for message from kernel:\n");
    while (1) {
        ssize_t status = recvmsg(fd, &msg, MSG_DONTWAIT); //status means message length

	//Failed to receive message or No message yet
	if (status < 0) {
	    if (errno == EINTR || errno == EAGAIN) {
#if 0
		    printf("No message yet. status=%ld\n", status);
#endif
		    usleep(100000); //100ms
		    continue;
	    }
	    printf("Failed to read netlink: %s", (char*)strerror(errno));
	    continue;
	}

	//Check if message length is correct
        if (msg.msg_namelen != sizeof(cli_addr)) {
            printf("Invalid length of the sender address struct\n");
            continue;
        }

	//Step 6: Parse message
	struct nlmsghdr *nl_hdr;
       	nl_hdr = (struct nlmsghdr*)buf;
	int pid = nl_hdr->nlmsg_pid;
	int my_pid = getpid();
	int len = nl_hdr->nlmsg_len;
	int type = nl_hdr->nlmsg_type;
	printf("Got message from kernel. my_pid=%d, pid=%d, status=%ld, length=%d, sizeof(nl_hdr)=%ld, type=%d\n",
		       my_pid, pid, status, len, sizeof(*nl_hdr), type);

	if (type == RTM_NEWROUTE) { //ip route add ...
	    printf("Got route add. message is: %s\n", (char*)msg.msg_iov);
	} else if (type == RTM_DELROUTE) { //ip route del ...
	    printf("Got route delete.\n");
	}
	usleep(1000000); //1sec
    }

    close(fd);
    return 0;
}
