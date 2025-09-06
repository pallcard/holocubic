//
// Created by 刘科 on 2025/9/4.

#include "home.h"

#include <WiFiClient.h>

#include "NTPClient.h"
#include "TFT_eSPI.h"
#include "../res/pic_as.h"
#include "../res/Weather.h"
#include "ArduinoJson.h"
#include "res/myfont.h"

const char* host = "api.seniverse.com";

Home::Home()
{
}

void Home::setTft(TFT_eSPI* tft)
{
    this->tft = tft;
}

void Home::setTimeClient(NTPClient* timeClient)
{
    this->timeClient = timeClient;
    this->updateWeather();
}


// 成员函数定义
void Home::show()
{
    astronautIndex++;
    if (astronautIndex >= astronautallArray_LEN)
    {
        astronautIndex = 0;
    }
    tft->pushImage(10, 55, 96, 128, astronautallArray[astronautIndex]);

    //    delay(100);
    // weatherIndex++;
    // if (weatherIndex >= 8)
    // {
    //     weatherIndex = 0;
    // }
    // tft->pushImage(135, 55, 64, 64, weather_pic[weatherIndex]);

    tft->fillRect(10, 10, 230, 20, TFT_BLACK);
    tft->setCursor(10, 10);
    tft->setTextColor(TFT_WHITE);
    tft->println(this->getTimeStr());

    // showHanziS(10, 25, String("武汉").c_str(), TFT_YELLOW);
    tft->setCursor(10, 30);
    tft->setTextColor(TFT_GREEN);
    tft->printf("武汉  %s  %d°C", weather.c_str(), temperature);
    // showHanziS(10, 25, String("雨").c_str(), TFT_YELLOW);
    // showHanziS(50, 25, weather.c_str(), TFT_YELLOW);
    //
    // tft->setCursor(100, 25, 2);
    // tft->setTextColor(TFT_WHITE);
    // tft->printf("%d°C", temperature);

    // showHanziS(115, 25, "度", TFT_WHITE);
    // Serial.println(location);
    // Serial.println(weather);
    // Serial.println(temperature);

    delay(500);
}

/*单一汉字显示*/
void Home::showHanzi(int32_t x, int32_t y, const char c[3], uint32_t color)
{
    for (int k = 0; k < hanzi_LEN; k++) // 根据字库的字数调节循环的次数
        if (hanzi[k].Index[0] == c[0] && hanzi[k].Index[1] == c[1] && hanzi[k].Index[2] == c[2])
        {
            tft->drawBitmap(x, y, hanzi[k].hz_Id, hanzi[k].hz_width, 16, color);
        }
}

/*整句汉字显示*/
void Home::showHanziS(int32_t x, int32_t y, const char str[], uint32_t color)
{
    //显示整句汉字，字库比较简单，上下、左右输出是在函数内实现
    int x0 = x;
    for (int i = 0; i < strlen(str); i += 3)
    {
        showHanzi(x0, y, str + i, color);
        x0 += 17;
    }
}

String Home::getTimeStr()
{
    timeClient->update();
    unsigned long epochTime = timeClient->getEpochTime();
    //将epochTime换算成年月日
    tm* ptm = gmtime((time_t*)&epochTime);
    int currentYear = ptm->tm_year + 1900; // tm_year是从1900开始的年数
    int monthDay = ptm->tm_mday;
    int currentMonth = ptm->tm_mon + 1;
    // 从NTPClient获取各个时间成分
    int currentHour = timeClient->getHours();
    int currentMinute = timeClient->getMinutes();
    int currentSecond = timeClient->getSeconds();

    // 格式化输出：YYYY-MM-DD HH:MM:SS
    char formattedTime[20]; // 缓冲区足够大 "YYYY-MM-DD HH:MM:SS\0"
    snprintf(formattedTime, sizeof(formattedTime), "%04d-%02d-%02d %02d:%02d:%02d",
             currentYear, currentMonth, monthDay,
             currentHour, currentMinute, currentSecond);
    // Serial.println(formattedTime);
    return formattedTime;
}

char determineqing[] = "晴";
char determineduoyun[] = "多云";
char determineyin[] = "阴";
char determineyu[] = "雨";
char determinexue[] = "雪";

void Home::updateWeather()
{
    //创建TCP连接
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort))
    {
        Serial.println("connection failed"); //网络请求无响应打印连接失败
        return;
    }
    //URL请求地址
    String url = "/v3/weather/now.json?key=S_xhO9flk_rjzOsJY&location=wuhan&language=zh-Hans&unit=c";
    //发送网络请求
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
        "Host: " + host + "\r\n" +
        "Connection: close\r\n\r\n");
    delay(2000);
    //定义answer变量用来存放请求网络服务器后返回的数据
    String answer;
    while (client.available())
    {
        String line = client.readStringUntil('\r');
        answer += line;
    }
    //断开服务器连接
    client.stop();
    Serial.println();
    Serial.println("closing connection");
    //获得json格式的数据
    String jsonAnswer;
    int jsonIndex;
    //找到有用的返回数据位置i 返回头不要
    for (int i = 0; i < answer.length(); i++)
    {
        if (answer[i] == '{')
        {
            jsonIndex = i;
            break;
        }
    }
    jsonAnswer = answer.substring(jsonIndex);
    Serial.println();
    Serial.println("JSON answer: ");
    Serial.println(jsonAnswer);
    const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) +
        210;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, jsonAnswer);
    JsonObject results_0 = doc["results"][0];
    JsonObject results_0_location = results_0["location"];
    const char* results_0_location_name = results_0_location["name"]; // "北京"
    location = results_0_location_name;
    JsonObject results_0_now = results_0["now"];
    const char* results_0_now_text = results_0_now["text"]; // "多云"
    temperature = results_0_now["temperature"]; // "5"
    if (strstr(results_0_now_text, determineqing) != 0)
    {
        weather = "晴";
        return;
    }
    if (strstr(results_0_now_text, determineduoyun) != 0)
    {
        weather = "多云";
        return;
    }
    if (strstr(results_0_now_text, determineyin) != 0)
    {
        weather = "阴";
        return;
    }
    if (strstr(results_0_now_text, determineyu) != 0)
    {
        weather = "雨";
        return;
    }

    if (strstr(results_0_now_text, determinexue) != 0)
    {
        weather = "雪";
    }
}
