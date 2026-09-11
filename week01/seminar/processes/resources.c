#include <err.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 12345
#define MSG  "hello from child process\n"

static int make_listener(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        err(EXIT_FAILURE, "socket");

    /* allow quick restarts without "address already in use" */
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
            .sin_family      = AF_INET,
            .sin_port        = htons(PORT),
            .sin_addr.s_addr = INADDR_ANY,
    };
    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
        err(EXIT_FAILURE, "bind");
    if (listen(fd, 1) == -1)
        err(EXIT_FAILURE, "listen");

    return fd;
}


/*
 * parent: keeps the listening socket, accepts the next connection
 * child: inherits the accepted connection fd, handles it, then exits
 */
int main(void) {
    int listen_fd = make_listener();
    printf("[parent pid=%d] listening socket fd=%d on port %d\n",
           getpid(), listen_fd, PORT);

    printf("[parent] waiting for a connection (run: nc 127.0.0.1 %d)\n", PORT);

    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    int conn_fd = accept(listen_fd, (struct sockaddr *) &client_addr, &addrlen);
    if (conn_fd == -1)
        err(EXIT_FAILURE, "accept");

    printf("[parent] accepted connection, conn_fd=%d — about to fork\n", conn_fd);

    // both listen_fd and conn_fd are duplicated into child
    pid_t pid = fork();
    if (pid == -1)
        err(EXIT_FAILURE, "fork");

    if (pid == 0) {
        // close copy, otherwise port stays open until the child also exits
        close(listen_fd);

        printf("[child  pid=%d] inherited conn_fd=%d, listen_fd closed\n",
               getpid(), conn_fd);

        /* Send a reply to the connected peer */
        if (write(conn_fd, MSG, strlen(MSG)) == -1)
            err(EXIT_FAILURE, "write");

        printf("[child] wrote \"%.*s\" to peer, exiting\n",
               (int) strlen(MSG) - 1, MSG);

        close(conn_fd);
        exit(EXIT_SUCCESS);

    } else {
        close(conn_fd);

        printf("[parent pid=%d] closed its copy of conn_fd, child owns it now\n",
               getpid());

        int status;
        waitpid(pid, &status, 0);
        printf("[parent] child exited with status %d\n",
               WEXITSTATUS(status));

        close(listen_fd);
        exit(EXIT_SUCCESS);
    }
}
