#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>

int client[32] = {};
int client_num = 0;
int client_fd_max = -1;

/*
cut the buffer with the '\n'
*/
int recvline(int s, char *buf, int max_len){
    int len = recv(s, buf, max_len, MSG_PEEK);
    if(len <= 0)
    	return len;

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

/*
*send broadcast
*/
void sendBroadcast(int fd, char *line,int length){
	int i;
	for(i=0;i<client_num;i++){
		if(client[i] != fd){
			if(send(client[i],line,length,0)<=0){
				break;
			}
		}
	}
}

int main(int argc, char **argv) {
    int port = atoi(argv[1]);
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket");
        return 1;
    }
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);// 将服务端的套接字设置成非阻塞

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    socklen_t addr_len = sizeof(addr);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind");
        return 1;
    }
    if (listen(fd, 32)) {
        perror("listen");
        return 1;
    }

    fd_set clients;
    char buffer[1048600] = "Message:";
    FD_ZERO(&clients);
    while (1) {
        //接受连接
    	int client_fd = accept(fd, NULL, NULL);
    	if (!(client_fd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
            client[client_num] = client_fd;
            client_num++;
            fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL, 0) | O_NONBLOCK); // 将客户端的套接字设置成非阻塞
            if(client_fd_max < client_fd)
        	client_fd_max = client_fd;
        }

        //循环设置集合
        FD_ZERO(&clients);

        int i = 0;
        for(i = 0;i < client_num;i++){
            FD_SET(client[i], &clients);
        }

        struct timeval tv;
        tv.tv_sec = tv.tv_usec = 0;

        if (select(client_fd_max + 1, &clients, NULL, NULL, &tv) > 0) { // 找出可以读的套接字
            for(i = 0;i < client_num;i++){
                if (FD_ISSET(client[i], &clients)) {
                    int len = 0;
                    if ((len = recvline(client[i], buffer + 8, 1048576)) <= 0) {
                        int j = 0;
                        for(j = 0; j < client_num; j++){
                            if(client[j] == client[i]){
                                close(client[j]);
                                client[j] = client[client_num - 1];
                                client_num--;
                            }
                        }
                    }
                    else{
                        sendBroadcast(client[i], buffer, len + 8);
                    }
                }
           }
        }//end if

    }
    return 0;
}