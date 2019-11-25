#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/netlink.h>
#include <net/sock.h>

#define NETLINK_VINCENT 31  //At most 32
#define MAX_MSGSIZE 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vincent_Chu <liataian@gmail.com>");


char *msg_from_kernel = "Hi This is the response from VINCENT module.";
struct sock *nl_sk = NULL;
int user_pid = -1;

static void send_msg_to_user(char *msg, int user_pid) {
	struct nlmsghdr *nlh = NULL;
	int len = NLMSG_SPACE(MAX_MSGSIZE);

	//Init a new skb
	struct sk_buff *skb;
	skb = alloc_skb(len, GFP_KERNEL);
	if (!skb) {
		printk("Failed to allocate skb...");
		return;
	}

	//Add a new netlink message into a skb (Construct header also, so return header address)
	nlh = nlmsg_put(skb, 0, 0, 0, MAX_MSGSIZE, 0); //Second arg is kernel pid
	NETLINK_CB(skb).portid = 0;
        NETLINK_CB(skb).dst_group = 0;
       
        //Copy response message into payload part of our netlink message
	strcpy(NLMSG_DATA(nlh), msg_from_kernel);
	printk("Send message: '%s' to user. \n", (char *)NLMSG_DATA(nlh));

	//Send response message to user space
        netlink_unicast(nl_sk, skb, user_pid, MSG_DONTWAIT);
}

//Receive message from user space(a skb)
static void recv_user_msg(struct sk_buff *skb) {
	struct nlmsghdr *nlh = NULL;
	nlh = nlmsg_hdr(skb); //Extract message header
	user_pid = nlh->nlmsg_pid; //Extract user pid

	//Check message length
	if (nlh->nlmsg_len < NLMSG_HDRLEN || skb->len < nlh->nlmsg_len) {
		return;
	}

	printk("Received messge: '%s' from user(pid:%d)\n", (char*)NLMSG_DATA(nlh), user_pid); //Extract payload in message
	
	//Response to user
	send_msg_to_user(msg_from_kernel, user_pid);
}

//Step1 Implement .input callback to receive message from user space
struct netlink_kernel_cfg test_cfg = {
	.input = recv_user_msg,
};

static int __init netlink_init(void) //init function would return int
{
    printk("Hellooooo\n");

    //Step2 Create kernel netlink socket with protocol NETLINK_VINCENT
    nl_sk = netlink_kernel_create(&init_net, NETLINK_VINCENT, &test_cfg);
    if (!nl_sk) {
        printk("Failed to create kernel netlink socket.\n");
	return -1;
    }
    return 0;
}

static void __exit netlink_exit(void) //exit function no need to return
{
    printk("Goodbyeeeee\n");
    netlink_kernel_release(nl_sk); //release kernel netlink socket
}

module_init(netlink_init);
module_exit(netlink_exit);
