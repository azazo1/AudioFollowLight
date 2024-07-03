//
// Created by azazo1 on 2024/6/5.
//

#ifndef MYSOCKETCLIENT_H
#define MYSOCKETCLIENT_H
#include <sys/socket.h>

class MySocketClient {
    int sockfd = -1;
    sockaddr_in addr;

public:
    MySocketClient();

    ~MySocketClient();

    void connect(const char *ip, uint16_t port);

    int recvVal() const;

    void sendHeartBeat() const;
};


#endif //MYSOCKETCLIENT_H
