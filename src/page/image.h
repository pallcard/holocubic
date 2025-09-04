//
// Created by 刘科 on 2025/9/5.
//

#ifndef HOLOCUBIC_IMAGE_H
#define HOLOCUBIC_IMAGE_H
#include "TFT_eSPI.h"

class Image
{
public:
    Image();
    TFT_eSPI* tft;
    int  index;
    void setTft(TFT_eSPI* tft);
    void show();
};
#endif //HOLOCUBIC_IMAGE_H