#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

int client[32] = {};
int client_num = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
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
			pthread_mutex_lock(&mutex);
			if(send(client[i],line,length,0)<=0){
				break;
			}
			pthread_mutex_unlock(&mutex);
		}
	}
}

void *handle_chat(void *data) {
    int user = *(int*)(data);
    while (1) {
        char buffer[1048600] = "Message:";
    	int len;
    	if((len = recvline(user, buffer + 8, 1048576)) > 0){
            sendBroadcast(user, buffer, len + 8);
        }
        else if(len <= 0){//out
            int i = 0;
            for(i = 0; i < client_num; i++){
                if(client[i] == user){
                    pthread_mutex_lock(&mutex);
                    client[i] = client[client_num - 1];
                    client_num--;
                    close(user);
                    pthread_mutex_unlock(&mutex);
                    return NULL;
                }
            }
        }
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
    if (listen(fd, 32)) {
        perror("listen");
        return 1;
    }
    while(1){
        int client_fd = accept(fd, NULL, NULL);
        if (client_fd == -1) {//last change
            perror("accept");
            return 1;
        }
        pthread_t pthread_id;
        client[client_num] = client_fd;
        pthread_create(&pthread_id,NULL,handle_chat,(void*)(&client[client_num]));
        pthread_mutex_lock(&mutex);
        client_num++;
        pthread_mutex_unlock(&mutex);
    }
    return 0;
}
