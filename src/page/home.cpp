//
// Created by 刘科 on 2025/9/4.

#include "home.h"

#include "NTPClient.h"
#include "TFT_eSPI.h"
#include "../res/pic_as.h"
#include "../res/Weather.h"


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
}


// 成员函数定义
void Home::show()
{
    timeClient->update();
    astronautIndex++;
    if (astronautIndex >= astronautallArray_LEN)
    {
        astronautIndex = 0;
    }
    tft->pushImage(10, 55,  96, 128,  astronautallArray[astronautIndex]);

    //    delay(100);
    weatherIndex++;
    if (weatherIndex >= 8)
    {
        weatherIndex = 0;
    }
    tft->pushImage(135, 55,  64, 64,  weather[weatherIndex]);

    unsigned long epochTime = timeClient->getEpochTime();
    // Serial.print("Epoch Time: ");
    // Serial.println(epochTime);
    // //打印时间
    // int currentHour = timeClient.getHours();
    // Serial.print("Hour: ");
    // Serial.println(currentHour);
    // int currentMinute = timeClient.getMinutes();
    // Serial.print("Minutes: ");
    // Serial.println(currentMinute);
    // int weekDay = timeClient.getDay();
    // Serial.print("Week Day: ");
    // Serial.println(weekDay);
    // //将epochTime换算成年月日
    struct tm *ptm = gmtime ((time_t *)&epochTime);
    int monthYear = ptm->tm_year;
    int monthDay = ptm->tm_mday;
    Serial.print("Month day: ");
    Serial.println(monthDay);
    int currentMonth = ptm->tm_mon+1;
    Serial.print("Month: ");
    Serial.println(currentMonth);

    tft->fillRect(10, 0, 100, 55, TFT_BLACK);
    tft->setCursor(10,30,2);
    tft->setTextColor(TFT_BLUE);
    tft->println(timeClient->getFormattedTime());

    tft->setCursor(10,0,2);
    tft->setTextColor(TFT_WHITE);
    tft->println(monthYear);

    tft->setCursor(10,10,2);
    tft->setTextColor(TFT_RED);
    tft->println(currentMonth);

    tft->setCursor(10,20,2);
    tft->setTextColor(TFT_GREEN);
    tft->println(monthDay);


    delay(100);
}