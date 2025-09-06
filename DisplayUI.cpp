// DisplayUI.cpp
#include "DisplayUI.h"

bool DisplayUI::begin(uint8_t sda, uint8_t scl, uint8_t addr){
  Wire.begin(sda,scl); Wire.setClock(400000);
  bool ok = d_.begin(SSD1306_SWITCHCAPVCC, addr);
  d_.clearDisplay(); d_.display();
  return ok;
}
void DisplayUI::clear(){ d_.clearDisplay(); d_.display(); }
void DisplayUI::drawPlateIconsAt_(int x, int y, HeatState sel, bool blinkOn){
  d_.setCursor(x, y); d_.print("F"); x += 6;
  bool front = (sel==HEAT_FRONT||sel==HEAT_BOTH);
  if(front&&blinkOn) d_.fillRect(x,y+1,6,6,SSD1306_WHITE); else d_.drawRect(x,y+1,6,6,SSD1306_WHITE);
  x += 10;
  d_.setCursor(x, y); d_.print("B"); x += 6;
  bool back  = (sel==HEAT_BACK ||sel==HEAT_BOTH);
  if(back&&blinkOn)  d_.fillRect(x,y+1,6,6,SSD1306_WHITE); else d_.drawRect(x,y+1,6,6,SSD1306_WHITE);
}

//Menu Selection 
void DisplayUI::showMenu(int index, float tFrontC, float tBackC, HeatState sel){
  static const char* items[] = { "Plates", "Profile", "Constant", "Test" };

  d_.clearDisplay();
  d_.setTextColor(SSD1306_WHITE); d_.setTextSize(1);
  d_.setCursor(0,0); d_.print("Reflow Station");

  // Menu items
  for (int i = 0; i < 4; ++i) {
    int y = 16 + i*12;
    d_.setCursor(0, y);
    d_.print((i==index) ? "> " : "  ");
    d_.print(items[i]);
  }

  // --- Icons on "Plates" row like your original: at (74,20) ---
  int xIcon = 74, yIcon = 20;
  d_.setCursor(xIcon, yIcon); d_.print("F");
  int xBoxF = xIcon + 6;
  bool useF = (sel==HEAT_FRONT || sel==HEAT_BOTH);
  if (useF) d_.fillRect(xBoxF, yIcon+1, 6, 6, SSD1306_WHITE);
  else      d_.drawRect(xBoxF, yIcon+1, 6, 6, SSD1306_WHITE);

  int xB = xBoxF + 8 + 6; // little gap + box width
  d_.setCursor(xB, yIcon); d_.print("B");
  int xBoxB = xB + 6;
  bool useB = (sel==HEAT_BACK || sel==HEAT_BOTH);
  if (useB) d_.fillRect(xBoxB, yIcon+1, 6, 6, SSD1306_WHITE);
  else      d_.drawRect(xBoxB, yIcon+1, 6, 6, SSD1306_WHITE);

  // --- Temps under them ---
  char buf[24];
  d_.setCursor(74, 32);  // x=74, y=32
  snprintf(buf, sizeof(buf), "F:%3dC", (int)tFrontC);
  d_.print(buf);

  d_.setCursor(74, 44);  // x=74, y=44
  snprintf(buf, sizeof(buf), "B:%3dC", (int)tBackC);
  d_.print(buf);

  d_.display();
}


void DisplayUI::showRun(float spC, float tFrontC, float tBackC, int dutyFrontPct, int dutyBackPct, Mode mode){
  d_.clearDisplay(); d_.setTextColor(SSD1306_WHITE); d_.setTextSize(1);
  d_.setCursor(0,0); d_.print(mode==PROFILE_RUN?"Profile":"Constant");
  d_.setCursor(80,0); d_.print("SP:"); d_.print((int)spC); d_.print("C");
  d_.setCursor(0,18); d_.print("Front: "); d_.print((int)tFrontC); d_.print("C  "); d_.print(dutyFrontPct); d_.print("%");
  d_.setCursor(0,32); d_.print("Back : "); d_.print((int)tBackC);  d_.print("C  "); d_.print(dutyBackPct);  d_.print("%");
  int barW=100, fW=constrain(map(dutyFrontPct,0,100,0,barW),0,barW), bW=constrain(map(dutyBackPct,0,100,0,barW),0,barW);
  d_.drawRect(0,46,barW,6,SSD1306_WHITE); d_.fillRect(1,47,max(0,fW-2),4,SSD1306_WHITE);
  d_.drawRect(0,56,barW,6,SSD1306_WHITE); d_.fillRect(1,57,max(0,bW-2),4,SSD1306_WHITE);
  d_.display();
}
