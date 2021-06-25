# 实验报告 实验三 PB19000071彭怡腾

## 1、简要介绍

本实验代码全部用C语言编写，使用如下方式编译本目录中的.C文件即可。

```c
gcc 1.c -o 1 -lpthread
gcc 2.c -o 2 -lpthread
gcc 3.c -o 3
```

然后使用如下命令即可运行服务器。

```c
./1 6666
./2 6666
./3 6666
```

本次实验完成了1，2，3三项实验内容。

## 2、完善双人聊天室

```c
创建一个略大于1M的字符数组用以接收每一次的消息
char buffer[1048600] = "Message:";
```

使用如下函数进行按换行符为分隔符的接收和分割数据，每一次读入缓冲区的数据（不超过max_len），但是通过使用strtok函数只取出第一个换行符前的部分，从而实现避免每次只接收单个字符。

```c
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
```

将助教给的原始代码接收部分改为此函数，其他部分与原始代码基本一致，详见1.C

## 3、利用多线程技术实现多人聊天室

总体思想为，持续等待用户进入，如果有用户进入，就在存储用户信息的数组中添加此用户的信息，如果退出，则删除其信息，值得注意的是每一次添加和删除信息的时候都需要使用互斥锁保证数组一次只被一个线程修改。

同时，由于socket并发写入可能造成的问题，在广播某用户的发送的信息的时候，需要给当前被广播的用户添加锁，防止其同时被别的线程也发送信息。

本部分的关键代码解释如下：

建立一个用户信息数组，client_num里面存储当前用户总数。

```c
int client[32] = {};
int client_num = 0;
```

广播函数，向其他所有用户广播当前用户发送的信息，发送的时候给对应用户上锁。

```c
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
```

下面两个部分值得注意的地方均为：在修改用户信息的时候需要使用互斥锁。

```c
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
```

```c
        pthread_t pthread_id;
        client[client_num] = client_fd;
        pthread_create(&pthread_id,NULL,handle_chat,(void*)(&client[client_num]));
        pthread_mutex_lock(&mutex);
        client_num++;
        pthread_mutex_unlock(&mutex);
```

## 4、利用 IO 复用 / 异步 IO 技术实现多人聊天室

主要分为两个板块，接受连接和信息交互，关键代码如下：

循环接受连接，并且将文件描述符置入文件描述符集合中，同时更新当前的用户信息数组和最大文件描述符。（之后有函数需要用到）

```c
    fd_set clients;
    FD_ZERO(&clients);
```

```c
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
```

循环查找当前可读的套接字，对于每一个用户，如果他的套接字文件描述符在可读集合中，则接受它的数据，并广播给其他所有用户，因为这里不再是多线程，所以不需要加锁。如果读出的信息小于等于0，则说明当前用户已退出连接，则将其从用户信息数组中删去。

```c
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
```

