#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

struct Pipe {
    int fd_send;
    int fd_recv;
};

/*
cut the buffer with the '\n'
*/
int recvline(int s, char *buf, int max_len){
    int len = recv(s, buf, max_len, MSG_PEEK);
    if(len == 0)
    	return 0;

    if(buf[0] == '\n'){
        recv(s, buf, 1, 0);
        return 1;
    }

    char* line = strtok(buf, "\n");
    int str_len = strlen(line);

    recv(s, buf, str_len + 1, 0);
    
    if(buf[str_len] != '\n'){
    	buf[str_len] = '\0';
    	return str_len;
    }
    
    return str_len + 1;
}

void *handle_chat(void *data) {
    struct Pipe *pipe = (struct Pipe *)data;
    char buffer[1048600] = "Message:";
    ssize_t len;
    while ((len = recvline(pipe->fd_send, buffer + 8, 1048576)) > 0) {
        send(pipe->fd_recv, buffer, len + 8, 0);
    }
    return NULL;
}

int main(int argc, char **argv) {
    int port = atoi(argv[1]);
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket");
        return 1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    socklen_t addr_len = sizeof(addr);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind");
        return 1;
    }
    if (listen(fd, 2)) {
        perror("listen");
        return 1;
    }
    int fd1 = accept(fd, NULL, NULL);
    int fd2 = accept(fd, NULL, NULL);
    if (fd1 == -1 || fd2 == -1) {
        perror("accept");
        return 1;
    }
    pthread_t thread1, thread2;
    struct Pipe pipe1;
    struct Pipe pipe2;
    pipe1.fd_send = fd1;
    pipe1.fd_recv = fd2;
    pipe2.fd_send = fd2;
    pipe2.fd_recv = fd1;
    pthread_create(&thread1, NULL, handle_chat, (void *)&pipe1);
    pthread_create(&thread2, NULL, handle_chat, (void *)&pipe2);
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    return 0;
}
