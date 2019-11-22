#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
//On linux need below two
#include <sys/types.h>
#include <sys/wait.h>

#define IFCONFIG_CMD_PATH "/sbin/ifconfig"

//Check if table is valid
int is_valid_interface(const char *iface_str) {
    pid_t pid = fork(); //Create a child process
    int status;

    if (pid < 0) {
	printf("fork failed (%s)", strerror(errno));
	return -1;
    } else if (pid == 0) { //child (return value from fork() is 0)
	if (execl(IFCONFIG_CMD_PATH, "ifconfig", iface_str, (char *) NULL)) {
	    printf("execl failed (%s)", strerror(errno));
	    exit(0);
	}
    } else { //parent (return value from fork() is parent’s pid)
        wait(&status);
	if (WIFEXITED(status) == 0 || WEXITSTATUS(status) != 0) { // WIFEXITED不為0為正常(正常退出), WEXITSTATUS為0為正常(正常回傳值)
		printf("Invalid interface, status=%d\n", status);
		return 0;
	}
    }
    return 1;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		printf("Invalid arg number\n");
		return 0;
	}
	const char *iface_str = argv[1]; //en0
	if (is_valid_interface(iface_str)) {
		printf("%s is valid\n", iface_str);
	}
	return 0;
}

