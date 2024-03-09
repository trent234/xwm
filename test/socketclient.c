#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// Replace this with your socket path
#define SOCKET_PATH "/tmp/xwm"

enum {GetClient, SetFocus}; /* socket commands. use these instead of hardcode below */

int main() {
    struct sockaddr_un addr;
    char buf[256];
    int fd, rc;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect error");
        exit(-1);
    }

    // Send a message to the server
    char* message = "0 \n junk "; // example message
    if (write(fd, message, strlen(message)) == -1) {
        perror("write error");
        exit(-1);
    }

    // Read the server's response
    while ((rc = read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[rc] = '\0'; // Null-terminate the buffer
        printf("Received: %s\n", buf);
    }
    if (rc == -1) {
        perror("read error");
        exit(-1);
    } else if (rc == 0) {
        printf("Server closed connection\n");
    }

    close(fd);
    return 0;
}