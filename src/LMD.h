#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ESP32-HUB75-VirtualMatrixPanel_T.hpp>
// #include <LittleFS.h>
#include <ArduinoJson.h>
// #include <WiFi.h>
// #include <ESPAsyncWebServer.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/Picopixel.h>
#include <Fonts/TomThumb.h>
#include <Fonts/mythic_5pixels.h>
#include <Fonts/B_5px.h>
// #include <Fonts/hud5pt7b.h>
// #include <Fonts/04B_5px.h>



#include <Fonts/Tiny3x3a2pt7b.h>
// ==== Pin mapping (giữ nguyên như bạn đang dùng) ====
#define R1_PIN 3
#define G1_PIN 10
#define B1_PIN 11
#define R2_PIN 12
#define G2_PIN 13
#define B2_PIN 14

#define CLK_PIN 42
#define LAT_PIN 41
#define OE_PIN 47

#define A_PIN 36
#define B_PIN 35
#define C_PIN 45
#define D_PIN 46
#define E_PIN -1



#define FORMAT_LITTLEFS_IF_FAILED true


// ==== Panel config ====
#define PANEL_RES_X 64//104//64      // Số pixel ngang của panel
#define PANEL_RES_Y 32//52//32      // Số pixel dọc của panel

// Định nghĩa struct cho từng dòng chữ
struct TextLine {
    int x;
    int y;
    int id;
    uint16_t color; 
};




// Tối đa 10 nhóm, mỗi nhóm tối đa 10 dòng (bạn có thể tăng nếu cần)
#define MAX_GROUPS 10
#define MAX_LINES_PER_GROUP 10

TextLine textCoorID[MAX_GROUPS][MAX_LINES_PER_GROUP];// tao mang 2 chieu de luu toa do va id cua tung dong chu trong tung nhom
int rowsTextInEachShapeFilter[MAX_GROUPS]; // Số dòng thực tế trong mỗi nhóm

#define MAX_GROUPS 10
#define MAX_LINES_PER_GROUP 10
#define MAX_TEXT_LENGTH 10

char textContents[MAX_GROUPS][MAX_LINES_PER_GROUP][MAX_TEXT_LENGTH]; // [nhóm][dòng][ký tự]




// ==== Scan type mapping ====
#define PANEL_SCAN_TYPE FOUR_SCAN_32PX_HIGH // hoặc FOUR_SCAN_64PX_HIGH tùy panel thực tế
using MyScanTypeMapping = ScanTypeMapping<PANEL_SCAN_TYPE>;

// ==== Khai báo đối tượng DMA và VirtualPanel ====
MatrixPanel_I2S_DMA *dma_display = nullptr;
VirtualMatrixPanel_T<CHAIN_NONE, MyScanTypeMapping>* virtualDisp = nullptr;

// ==== Màu sắc mẫu ====
uint16_t myBLACK, myWHITE, myRED, myGREEN, myBLUE;

void setup_ledPanel(){
    HUB75_I2S_CFG mxconfig(
    PANEL_RES_X * 2,
    PANEL_RES_Y / 2,
    1,
    HUB75_I2S_CFG::i2s_pins{
      R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN,
      A_PIN, B_PIN, C_PIN, D_PIN, E_PIN,
      LAT_PIN, OE_PIN, CLK_PIN
    }
  );
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(90);
  dma_display->clearScreen();


  virtualDisp = new VirtualMatrixPanel_T<CHAIN_NONE, MyScanTypeMapping>(1, 1, PANEL_RES_X, PANEL_RES_Y);
  virtualDisp->setDisplay(*dma_display);

  myBLACK = virtualDisp->color565(0, 0, 0);
  myWHITE = virtualDisp->color565(255, 255, 255);

  virtualDisp->fillScreen(myBLACK);
}

uint16_t colorWheel(uint8_t pos) {
  if(pos < 85) {
    return dma_display->color565(pos * 3, 255 - pos * 3, 0);
  } else if(pos < 170) {
    pos -= 85;
    return dma_display->color565(255 - pos * 3, 0, pos * 3);
  } else {
    pos -= 170;
    return dma_display->color565(0, pos * 3, 255 - pos * 3);
  }
}

void drawText(int colorWheelOffset)
{
  
  // draw text with a rotating colour
  dma_display->setTextSize(1);     // size 1 == 8 pixels high
  dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves

  dma_display->setCursor(5, 0);    // start at top left, with 8 pixel of spacing
  uint8_t w = 0;
  const char *str = "ESP32 DMA";
  for (w=0; w<strlen(str); w++) {
    dma_display->setTextColor(colorWheel((w*32)+colorWheelOffset));
    dma_display->print(str[w]);
  }

  dma_display->println();
  dma_display->print(" ");
  for (w=9; w<18; w++) {
    dma_display->setTextColor(colorWheel((w*32)+colorWheelOffset));
    dma_display->print("*");
  }
  
  dma_display->println();

  dma_display->setTextColor(dma_display->color444(15,15,15));
  dma_display->println("LED MATRIX!");

  // print each letter with a fixed rainbow color
  dma_display->setTextColor(dma_display->color444(0,8,15));
  dma_display->print('3');
  dma_display->setTextColor(dma_display->color444(15,4,0));
  dma_display->print('2');
  dma_display->setTextColor(dma_display->color444(15,15,0));
  dma_display->print('x');
  dma_display->setTextColor(dma_display->color444(8,15,0));
  dma_display->print('6');
  dma_display->setTextColor(dma_display->color444(8,0,15));
  dma_display->print('4');

  // Jump a half character
  dma_display->setCursor(34, 24);
  dma_display->setTextColor(dma_display->color444(0,15,15));
  dma_display->print("*");
  dma_display->setTextColor(dma_display->color444(15,0,0));
  dma_display->print('R');
  dma_display->setTextColor(dma_display->color444(0,15,0));
  dma_display->print('G');
  dma_display->setTextColor(dma_display->color444(0,0,15));
  dma_display->print("B");
  dma_display->setTextColor(dma_display->color444(15,0,8));
  dma_display->println("*");

}


void DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color = myWHITE) {
  virtualDisp->drawLine(x0, y0, x1, y1, color);
}
void DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,uint16_t color = myWHITE) {
  virtualDisp->drawRect(x, y, w, h, color);
}
void DrawCircle(uint16_t x, uint16_t y, uint16_t r,uint16_t color = myWHITE) {
  virtualDisp->drawCircle(x, y, r, color);
}
void DrawRound(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t radius = 1, uint16_t color = myWHITE) {
  virtualDisp->drawRoundRect(x0, y0, x1 - x0, y1 - y0, radius, color);
}
void DrawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color = myWHITE) {
  virtualDisp->drawTriangle(x0, y0, x1, y1, x2, y2, color);
}

void DrawText(int x, int y, uint16_t color, const char* text) {
    virtualDisp->setTextColor(color);
    virtualDisp->setCursor(x, y);
    virtualDisp->print(text);
}
