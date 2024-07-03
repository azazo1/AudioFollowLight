#include <Arduino.h>
#include <WiFi.h>
#include "MySocketClient.h"
#include "WlanConfig.h"
#include "IPConfig.h"

#define PWM_PIN GPIO_NUM_33
// #define LINEAR_CLOSING
#define DIVIDE_CLOSING

MySocketClient socket_client;

int targetPWMValue = 0;
int currentPWMValue = 0;

void routine() {
    // 中断函数内不能直接调用 analogWrite, 否则调用频率太高了, analogWrite 输出不稳定.
#ifdef LINEAR_CLOSING
    if (currentPWMValue < targetPWMValue) {
        currentPWMValue++;
    } else if (currentPWMValue > targetPWMValue) {
        currentPWMValue--;
    }
#endif
#ifdef DIVIDE_CLOSING
    int diff = targetPWMValue - currentPWMValue;
    constexpr int divide = 20;
    if (diff > 0) {
        diff = max(diff / divide, 1);
    } else if (diff < 0) {
        diff = min(diff / divide, -1);
    }
    currentPWMValue += diff;
#endif
}

void setup() {
    Serial.begin(9600);
    WiFi.begin(SSID, PWD);
    while (WiFiClass::status() != WL_CONNECTED) {
        Serial.print("Connecting: ");
        Serial.println(WiFiClass::status());
        delay(500);
    }
    Serial.println("Connected.");
    socket_client.connect(IP, PORT);
    Serial.println("Socket Connected.");
    pinMode(PWM_PIN, OUTPUT);
    analogWrite(PWM_PIN, 0);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    hw_timer_t *timer = timerBegin(0, 80, true); // 80MHz / 80 = 1MHz (也就是 1us)
    timerAttachInterrupt(timer, routine, true);
    timerAlarmWrite(timer, 7600, true); // 7600 us 触发 routine 一次, 然后自动重装, 使 routine 反复执行.
    timerAlarmEnable(timer);
}

void loop() {
    // 非阻塞, -1 为连接断开, -2 为数据错误, -3 为暂无数据, >= 0 为有效数据(计算机上监听到的音量大小).
    const int val = socket_client.recvVal();
    delay(1); // 让 analogWrite 能够稳定输出, 如果不添加此行, LED 的亮度会不符合 currentPWMValue, 而且有明显卡顿感.
    analogWrite(PWM_PIN, currentPWMValue); // 应用在 routine 函数中修改的 currentPWMValue.
    if (val != -3) {
        // 收到有效数据, 发送心跳, 输出调试信息.
        socket_client.sendHeartBeat(); // 发现发送心跳内容能够使接收响应速度更快
        Serial.print("Get Val: ");
        Serial.print(val);
        Serial.print(" Cur: ");
        Serial.print(currentPWMValue);
        Serial.println();
    }
    if (val == -1) {
        // 重连
        socket_client.connect(IP, PORT);
        return;
    }
    if (val < 0) {
        return;
    }
    // analogWrite(PWM_PIN, val * 2 / 3); // 控制不要太亮
    targetPWMValue = val / 3; // 除三为了控制亮度
    // delay(10);
}
