#include <Arduino.h>
#include <ios>

#include "res/myfont.h"

#include "TFT_eSPI.h"
#include <WiFiManager.h>
#include "NTPClient.h"
#include "PubSubClient.h"
#include "res/pic_wifi.h"
#include <ArduinoJson.h> // 引入ArduinoJson库用于解析JSON
#include "page/home.h"
#include "page/image.h"
#include "res/song10.h"


class PageHome;
// tft
TFT_eSPI tft = TFT_eSPI();
// TFT_eSprite sprite = TFT_eSprite(&tft); //处理闪屏问题

// wifi
const char* ssid = "ESP32_AP";
const char* password = "12345678";

// MQTT Broker Settings
const char* mqtt_server = "192.168.101.50"; // 例如本地部署填局域网IP（如192.168.1.xxx）或公共服务器地址
const int mqtt_port = 1883; // 默认TCP端口，若用SSL/TLS通常为8883
const char* clientId = "esp32"; // 客户端ID，确保唯一性
const char* mqttUser = ""; // 如果EMQX设置了认证
const char* mqttPassword = "lk123456"; // 如果EMQX设置了认证

const char* topic_operate = "topic/operate";
const char* topic_state = "topic/state";
const char* topic_publish = "topic/to/publish"; // ESP32向此主题发布消息


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp.aliyun.com"); //NTP服务器地址

WiFiClient espClient;
PubSubClient client(espClient);

// 全局状态
int pageIndex = 0;

// 按键
#define PS2_X     35
#define PS2_Y     34
#define SW        32
const int centerX = 1900; // 摇杆X轴居中的原始ADC值
const int centerY = 1900; // 摇杆Y轴居中的原始ADC值
// 死区阈值：消除居中位置附近的微小波动
const int deadZone = 100;

void printLog(const String& info)
{
    tft.fillRect(10, 10, 240, 15, TFT_BLACK);
    tft.setCursor(10, 10, 2);
    tft.setTextColor(TFT_RED);
    tft.println(info);
    delay(1000);
}


void setWiFiConfig()
{
    tft.setCursor(40, 75, 2);
    tft.setTextColor(TFT_RED);
    tft.println("waiting wifi ...");
    int i = 0;
    tft.pushImage(70, 100, 100, 20, wifiallArray[i]);

    WiFiManager manager;
    manager.autoConnect(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        i++;
        tft.pushImage(70, 100, 100, 20, wifiallArray[i]);
        if (wifiallArray_LEN - 1 == i)
        {
            i = 0;
        }
        delay(10);
        Serial.print("waiting for connect wifi ...");
    }
    tft.pushImage(70, 100, 100, 20, wifiallArray[wifiallArray_LEN - 1]);
    Serial.println("connected success!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    delay(1000);
}

void executeCommand(String command, String param1, String param2)
{
    // delay(1000);
    // printLog("executeCommand:"+command+" param1: "+param1);
    // 简单的字符串匹配处理指令
    // 根据不同的命令执行操作
    if (command == "ss")
    {
        //slideScreen
        if (param1 == "l") //left
        {
            tft.fillScreen(TFT_BLACK);
            pageIndex -= 1;
        }
        else if (param1 == "r") //right
        {
            tft.fillScreen(TFT_BLACK);
            pageIndex += 1;
        }
    }
}

void updateState(const DynamicJsonDocument& state)
{
    if (state.containsKey("pageIndex"))
    {
        tft.fillScreen(TFT_BLACK);
        pageIndex = state["pageIndex"].as<int>();
    }
    //todo
}

// ClientId:命令:参数1,参数2，...
void callback(char* topic, byte* payload, unsigned int length)
{
    String message;
    for (int i = 0; i < length; i++)
    {
        message += static_cast<char>(payload[i]);
    }
    // printLog("Message arrived [" + String(topic)+ "]:" + message);

    if (String(topic).equals(topic_operate))
    {
        // printLog("equals(topic_operate):" + message);

        // clientId:commend:param1,param2
        // 分割消息各部分
        int firstColon = message.indexOf(':');
        int secondColon = message.indexOf(':', firstColon + 1);
        int commaPos = message.indexOf(',', secondColon + 1);

        // 检查格式有效性
        if (firstColon == -1 || secondColon == -1)
        {
            Serial.println("Error: Invalid message format. Expected 'clientId:command:param1,param2'");
            return;
        }

        // 提取各部分
        String targetClientId = message.substring(0, firstColon);
        String command = message.substring(firstColon + 1, secondColon);
        String param1 = "";
        String param2 = "";

        // 解析参数（处理可能缺少参数的情况）
        if (commaPos != -1)
        {
            param1 = message.substring(secondColon + 1, commaPos);
            param2 = message.substring(commaPos + 1);
        }
        else
        {
            // 如果没有逗号，尝试将第二个冒号后的所有内容作为第一个参数
            param1 = message.substring(secondColon + 1);
        }

        // 检查此消息是否目标为本设备
        if (targetClientId.equals(clientId))
        {
            Serial.println("Message not for this device. Ignoring.");
            return;
        }
        executeCommand(command, param1, param2);
    }
    else if (String(topic).equals(topic_state))
    {
        // 解析JSON消息
        DynamicJsonDocument state(1024); // 根据消息大小调整缓冲区大小
        DeserializationError error = deserializeJson(state, message);
        if (error)
        {
            Serial.print("JSON解析失败: ");
            Serial.println(error.c_str());
            return;
        }
        updateState(state);
    }
}

void reconnectMQTT()
{
    while (!client.connected())
    {
        if (client.connect(clientId, mqttUser, mqttPassword))
        {
            Serial.println("Connected to MQTT broker");
            client.subscribe(topic_state); // 重新连接后订阅主题
            client.subscribe(topic_operate); // 重新连接后订阅主题
        }
        else
        {
            Serial.println("Failed, rc=" + String(client.state()) + " try again in 5 seconds");
            delay(5000);
        }
    }
}

Home pageHome; // 调用MyClass的默认构造函数
Image pageImage; // 调用MyClass的默认构造函数


void setup()
{
    Serial.begin(115200);
    Serial.println("setup start");

    //tft 模块
    tft.begin(); //初始化
    tft.loadFont(song10);
    tft.setRotation(0);
    tft.setSwapBytes(true);
    // sprite.setColorDepth(8);
    // sprite.setSwapBytes(true);
    // if (!sprite.createSprite(128, 128))
    // {
    //     // 假设创建100x100的缓冲区
    //     Serial.println("Sprite buffer allocation failed!");
    //     return; // 创建失败，直接返回。切勿继续执行绘制操作！
    // }

    // sprite.createSprite(tft.width(), tft.height());
    tft.fillScreen(TFT_BLACK); //屏幕颜色
    // showHanziS(40, 50, "周日晴", TFT_YELLOW);//显示汉字
    tft.println("tft success!");


    // wifi
    Serial.println("init wifi");
    setWiFiConfig();
    Serial.println("init wifi success");

    tft.fillScreen(TFT_BLACK); //屏幕颜色

    //MQTT
    Serial.println("init MQTT");
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback); // 设置收到消息后的回调函数
    reconnectMQTT(); // 连接MQTT服务器
    Serial.println("init MQTT success!");

    tft.fillScreen(TFT_BLACK); //屏幕颜色

    // ntp
    Serial.println("init ntp");
    timeClient.begin();
    timeClient.setTimeOffset(28800);
    Serial.println("init ntp success!");

    tft.fillScreen(TFT_BLACK); //屏幕颜色

    pageHome.setTft(&tft);
    pageHome.setTimeClient(&timeClient);
    pageImage.setTft(&tft);

    // 配置输入模式
    pinMode(PS2_X, INPUT);
    pinMode(PS2_Y, INPUT);
    pinMode(SW, INPUT_PULLUP);

    Serial.printf("x: %d, y: %d, z: %d\n",
                  analogRead(PS2_X), //读模拟信号
                  analogRead(PS2_Y),
                  digitalRead(SW)); //读数字信号


    Serial.println("setup success!!!");
}


void loop()
{
    // 确保MQTT客户端保持连接
    if (!client.connected())
    {
        reconnectMQTT();
    }
    client.loop(); // 允许MQTT客户端处理接收的消息并维持连接

    if (pageIndex == 0)
    {
        pageHome.show();
    }
    else
    {
        pageImage.show();
    }

    if (digitalRead(SW) == 0)
    {
        tft.fillScreen(TFT_BLACK);
        pageIndex = 0;
    }

    // 1. 读取原始模拟值
    int rawX = analogRead(PS2_X);
    int rawY = analogRead(PS2_Y);
    // 2. 转换为以(0,0)为中心的坐标
    int mappedX = map(rawX, 0, 4095, -100, 100); // 将原始范围映射到-100到100
    int mappedY = map(rawY, 0, 4095, -100, 100); // ESP32 ADC分辨率通常为12位(0-4095)

    // 3. 应用死区，消除居中附近的抖动
    if (abs(mappedX) < deadZone) mappedX = 0;
    if (abs(mappedY) < deadZone) mappedY = 0;

    // 4. 打印结果用于调试
    Serial.print("X: ");
    Serial.print(mappedX);
    Serial.print(" | Y: ");
    Serial.println(mappedY);

    // 5. 根据坐标判断方向（示例）
    if (mappedX == 0 && mappedY == 0)
    {
        Serial.println("Stop");
        // 执行停止动作
    }
    else
    {
        tft.fillScreen(TFT_BLACK);
        // 判断主要方向
        if (mappedY > 50)
        {
            pageIndex++;
            Serial.println("Forward");
            // 执行向前动作
        }
        else if (mappedY < -50)
        {
            pageIndex--;
            Serial.println("Backward");
            // 执行向后动作
        }

        if (mappedX > 50)
        {
            pageIndex++;
            Serial.println("Right");
            // 执行向右动作
        }
        else if (mappedX < -50)
        {
            pageIndex--;
            Serial.println("Left");
            // 执行向左动作
        }
    }
}
