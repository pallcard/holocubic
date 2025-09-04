//
// Created by 刘科 on 2025/9/4.
//

#ifndef HOLOCUBIC_HOME_H
#define HOLOCUBIC_HOME_H
#include "NTPClient.h"
#include "TFT_eSPI.h"

class Home
{
public:
    Home();
    TFT_eSPI* tft;   // 长度
    NTPClient* timeClient;
    int  astronautIndex;
    int  weatherIndex;
    // 成员函数声明
    void show();
    void setTft(TFT_eSPI* tft);
    void setTimeClient(NTPClient* timeClient);
    TFT_eSPI& getTFT() const { return *tft; }
};

#endif //HOLOCUBIC_HOME_H