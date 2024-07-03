//
// Created by azazo1 on 2024/6/5.
//
#include <Arduino.h>
#include "MySocketClient.h"

/**
 * 检查 str 是否是满足 r"^\d{3}" 正则匹配的字符串, 需要保证 str 有至少 3 个字符的空间.
 */
bool checkVal(const char *str) {
    return isdigit(str[0]) && isdigit(str[1]) && isdigit(str[2]);
}

auto defaultErrorWhat = "Socket Error";

class SocketException {
    const char *whatPtr = nullptr;

public:
    explicit SocketException(const char *str = nullptr): whatPtr(str) {
    }

    const char *what() const noexcept {
        if (whatPtr == nullptr) {
            return defaultErrorWhat;
        }
        return whatPtr;
    }
};

MySocketClient::MySocketClient(): addr() {
}

void MySocketClient::connect(const char *ip, const uint16_t port) {
    if (sockfd != -1) {
        close(sockfd);
        delay(1000);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        Serial.println("Failed to create socket");
        exit(1);
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        Serial.println("IP address is invalid");
        exit(1);
    }
    if (::connect(sockfd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        Serial.println("Connection failed");
        exit(1);
    }
}

MySocketClient::~MySocketClient() {
    if (sockfd >= 0) {
        close(sockfd);
    }
}

int MySocketClient::recvVal() const {
    char buffer[1024] = {0};
    const int getCnt = recv(sockfd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
    if (getCnt <= 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -3;
        }
        Serial.println("Recv Error");
        return -1;
    }
    buffer[getCnt] = '\0';
    int start = 0;
    while (start < getCnt - 2 && !checkVal(buffer + start)) {
        start++;
    }
    if (start == getCnt - 2) {
        // 没有收到 "\d{3}" 这样的字符串
        Serial.println("Invalid Recv");
        return -2;
    }
    // 到达第一个数字的位置
    return strtol(buffer + start, nullptr, 10);
}

void MySocketClient::sendHeartBeat() const {
    if (sockfd != -1) {
        constexpr char buffer[] = "I'am alive.\n";
        send(sockfd, buffer, sizeof(buffer), 0);
    }
}
