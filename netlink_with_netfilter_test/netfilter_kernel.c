#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/icmp.h>
#include <linux/ip.h>
#include <linux/inet.h>

#define NETFILTER_VINCENT 30  //At most 32
#define MAX_MSGSIZE 1024
#define RESPONSE_MSG_TYPE 55
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vincent_Chu <liataian@gmail.com>");


char *msg_from_kernel = "Hi This is the icmp packet from NETFILTER module.";
struct sock *nl_sk = NULL;
int user_pid = -1;
int count = 0;
int res = 0;

#if 0
static void send_msg_to_user(char *msg, int user_pid) {
	struct nlmsghdr *nlh = NULL;
	int len = NLMSG_SPACE(MAX_MSGSIZE);

	//Init a new skb
	struct sk_buff *skb;
	skb = alloc_skb(len, GFP_KERNEL); //Allocate memory for a new akb
	if (!skb) {
		printk("Failed to allocate memory for skb...");
		return;
	}

	//Fulfill a new netlink message into a skb (Construct header also, so return header address)
	nlh = nlmsg_put(skb, 0, 0, 0, MAX_MSGSIZE, 0); //Second arg is kernel pid, total return message length is MAX_MSGSIZE + NLMSG_HDRLEN(e.g 1024+16)
        nlh->nlmsg_type = RESPONSE_MSG_TYPE;
	NETLINK_CB(skb).portid = 0;
        NETLINK_CB(skb).dst_group = 0;

        //Copy response message into payload part of our netlink message
	strcpy(NLMSG_DATA(nlh), msg_from_kernel);
	printk("Send message: '%s' to user. \n", (char *)NLMSG_DATA(nlh));

        //Send response message to user space (Non-blocking)
        netlink_unicast(nl_sk, skb, user_pid, MSG_DONTWAIT); //function would return when there is no data if last arg is 1 (otherwise sleep)
}
#endif
#if 1
//Receive message from user space(a skb)
static void recv_user_msg(struct sk_buff *skb) {
	struct nlmsghdr *nlh = NULL;
	nlh = nlmsg_hdr(skb); //Extract message header
	user_pid = nlh->nlmsg_pid; //Extract user pid

	//Check message length
	if (nlh->nlmsg_len < NLMSG_HDRLEN || skb->len < nlh->nlmsg_len) {
		return;
	}
        count++;
	printk("Received messge: '%s' from user pid:%d, len=%d (count=%d)\n", (char*)NLMSG_DATA(nlh), user_pid, nlh->nlmsg_len, count); //Extract payload in message
	
	//Response to user
//	send_msg_to_user(msg_from_kernel, user_pid);
}
#endif
unsigned int get_icmp(void *priv,
                      struct sk_buff *skb,
                      const struct nf_hook_state *state) {
    printk("Just for check");
    return 0;
}

static struct nf_hook_ops icmp_ops = {
    .hook = get_icmp, //hook function
    .pf = AF_INET,
    .hooknum = NF_INET_PRE_ROUTING,
    .priority = NF_IP_PRI_FILTER -1,
};

struct netlink_kernel_cfg test_cfg = {
	.input = recv_user_msg,
};

static int __init netfilter_test_init(void) //init function would return int
{
    printk("Hellooooo...\n");
    nl_sk = netlink_kernel_create(&init_net, NETFILTER_VINCENT, &test_cfg);
    if (!nl_sk) {
        printk("Failed to create kernel netlink socket.\n");
	return -1;
    }
    //Register our hook function
    res = nf_register_net_hook(&init_net, &icmp_ops);
    if (res) {
        printk(KERN_INFO "module is NOT loaded into the kernel\n");    
        return -1;
    }
    return 0;
}

static void __exit netfilter_test_exit(void) //exit function no need to return
{
    printk("Goodbyeeeee...\n");
    netlink_kernel_release(nl_sk); //release kernel netlink socket
    //Unregister our hook function
    nf_unregister_net_hook(&init_net, &icmp_ops);
}

module_init(netfilter_test_init);
module_exit(netfilter_test_exit);
