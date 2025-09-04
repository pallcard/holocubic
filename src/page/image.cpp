//
// Created by 刘科 on 2025/9/5.
//

#include "image.h"
#include "res/image.h"

Image::Image()
{
}

void Image::setTft(TFT_eSPI* tft)
{
    this->tft = tft;
}

// 成员函数定义
void Image::show()
{
    index++;
    if (index >= imageallArray_LEN)
    {
        index = 0;
    }
    tft->pushImage(10, 55,  102, 136,  imageallArray[index]);
    delay(100);
}