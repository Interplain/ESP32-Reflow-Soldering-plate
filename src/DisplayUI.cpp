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


void DisplayUI::showProfileSetup(const Profile& prof, int plateCount) {
  d_.clearDisplay();
  d_.setTextSize(1);
  d_.setTextColor(SSD1306_WHITE);
  
  d_.setCursor(0, 0);
  d_.print("Profile Setup");
  
  d_.setCursor(0, 20);
  d_.print("Pick profile");
  
  d_.setCursor(10, 32);
  d_.print(prof.name);
  
  d_.setCursor(0, 44);
  d_.print("Plates: ");
  d_.print(plateCount);
  
  d_.setCursor(0, 56);
  d_.print("Start  Long=Back");
  
  d_.display();
}

void DisplayUI::showConstantSetup(int targetTemp, int duration) {
  d_.clearDisplay();
  d_.setTextSize(1);
  d_.setTextColor(SSD1306_WHITE);
  
  d_.setCursor(0, 0);
  d_.print("Constant Setup");
  
  d_.setCursor(0, 24);
  d_.print("Setpoint: ");
  d_.print(targetTemp);
  d_.print("C");
  
  d_.setCursor(0, 36);
  d_.print("Duration: ");
  d_.print(duration);
  d_.print("s");
  
  d_.setCursor(0, 56);
  d_.print("Start  Long=Back");
  
  d_.display();
}

void DisplayUI::showTest(int dutyCycle, float tF, float tB, HeatState heatSel) {
  d_.clearDisplay();
  d_.setTextSize(1);
  d_.setTextColor(SSD1306_WHITE);
  
  d_.setCursor(0, 0);
  d_.print("Test Heaters");
  
  d_.setCursor(0, 15);
  d_.print("Enc changes duty");
  
  d_.setCursor(15, 30);
  d_.print("Duty: ");
  d_.print(dutyCycle);
  d_.print("%");
  
  d_.setCursor(0, 42);
  d_.print("F:");
  d_.print((int)tF);
  d_.print("C  B:");
  d_.print((int)tB);
  d_.print("C");
  
 
  d_.setCursor(0, 56);
  d_.print("Hold=CoolTest");
  
  d_.display();
}

void DisplayUI::showCoolTest(float tF, float tB) {
  d_.clearDisplay();
  d_.setTextSize(1);
  d_.setTextColor(SSD1306_WHITE);
  
  d_.setCursor(0, 0);
  d_.print("Cooling Test");
  
  d_.setCursor(0, 20);
  d_.print("F:");
  d_.print((int)tF);
  d_.print("C");
  
  d_.setCursor(0, 32);
  d_.print("B:");
  d_.print((int)tB);
  d_.print("C");
  
  d_.setCursor(0, 50);
  d_.print("Check Serial");
  
  d_.display();
}


//Menu Selection 
void DisplayUI::showMenu(int index, float tFrontC, float tBackC, HeatState sel, bool fanMode, bool fanState){
  static const char* items[] = { "Plates", "Profile", "Constant", "Test" };

  d_.clearDisplay();
  d_.setTextColor(SSD1306_WHITE); d_.setTextSize(1);
  d_.setCursor(0,0); d_.print("Reflow Station");

 // Menu items
for (int i = 0; i < 4; ++i) {
  int y = 16 + i*12;
  if (i == index) {
    d_.fillTriangle(0, y-3, 0, y+3, 5, y, SSD1306_WHITE);
  }
  d_.setCursor(10, y);
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

  d_.setCursor(74, 56);  // Add this line
  d_.print(fanMode ? (fanState ? "Fan:ON" : "Fan:OFF") : "Auto");  // Add this line

  d_.display();
}

// ADD THESE NEW METHODS TO YOUR EXISTING DisplayUI.cpp FILE:

void DisplayUI::setupProfileDisplay(const Profile& prof, int durationSec) {
  currentProfile_ = &prof;
  profileDuration_ = durationSec;
  secsPerDispSlot_ = (float)durationSec / 108.0f;  // 108 pixels wide for graph
  lastPlotX_ = -1;
  
  // Clear display and draw static elements
  d_.clearDisplay();
  d_.setTextSize(1);
  d_.setTextColor(SSD1306_WHITE);
  
  // Draw the condensed profile outline
  drawCondensedProfileOutline_(prof);
  
  d_.display();
}

void DisplayUI::drawCondensedProfileOutline_(const Profile& prof) {
  // Use condensed graph area: Y 16-40 (24 pixels high)
  int slot = 0;
  float targetTempC = (float)prof.profMinTemp;
  int slotEndSecs = prof.slots[0].slotSecs;
  int slotSecs = slotEndSecs;
  float rate = ((float)prof.slots[0].targetTempC - targetTempC) / (float)slotSecs;
  int dispSlot = 0;
  
  int dispHeight = 24; // Condensed height for graph
  int graphTop = 16;   // Graph starts at Y=16
  int graphLeft = 10;  // Graph starts at X=20
  float degPerPixel = ((float)prof.profMaxTemp - (float)prof.profMinTemp) / (float)dispHeight;
  
  // Draw temperature scale labels (condensed)
  d_.setTextSize(1);
  d_.setCursor(0, 12);
  d_.print(prof.profMaxTemp);
  d_.setCursor(0, 20);
  d_.print(prof.profMinTemp);
  
  // Draw graph border
  //d_.drawRect(graphLeft, graphTop, 108, dispHeight, SSD1306_WHITE);
  
  // Calculate and draw setpoint line
  for (int t = 0; t < profileDuration_; t++) {
    // Move to next slot if needed
    if (t > slotEndSecs && slot < (prof.slotCount - 1)) {
      slot++;
      slotEndSecs = prof.slots[slot].slotSecs;
      slotSecs = slotEndSecs - t;
      if ((float)prof.slots[slot].targetTempC == (targetTempC - rate)) {
        rate = 0.0f;
      } else {
        rate = (((float)prof.slots[slot].targetTempC - (targetTempC - rate)) / (float)slotSecs);
      }
    }
    
    // Draw setpoint pixel
    if (((float)t / secsPerDispSlot_) > (float)dispSlot && dispSlot < 108) {
      float tempY = (float)graphTop + (float)dispHeight - 1.0f - ((targetTempC - (float)prof.profMinTemp) / degPerPixel);
      int pointY = (int)tempY;
      if (pointY >= graphTop && pointY < (graphTop + dispHeight)) {
        setPointDisp_[dispSlot] = (uint8_t)pointY;
  d_.drawPixel(graphLeft + dispSlot, pointY, SSD1306_WHITE);
      }
      dispSlot++;
    }
    
    targetTempC = targetTempC + rate;
  }
}

void DisplayUI::plotTemperature(float avgTemp, int currentSec) {
  if (!currentProfile_) return;
  
  int xPoint = (int)((float)currentSec / secsPerDispSlot_);
  if (lastPlotX_ != xPoint && xPoint < 108) {
    // Calculate Y position in condensed graph area
    float degPerPixel = ((float)currentProfile_->profMaxTemp - (float)currentProfile_->profMinTemp) / 24.0f;
    float tempY = 16.0f + 24.0f - 1.0f - ((avgTemp - (float)currentProfile_->profMinTemp) / degPerPixel);
    int lineY = (int)tempY;
    
    // Draw actual temperature line (within graph bounds)
    if (lineY >= 16 && lineY < 40 && xPoint >= 0) {
      d_.drawLine(10 + xPoint, 39, 10 + xPoint, lineY, SSD1306_WHITE);
    }
    
    lastPlotX_ = xPoint;
  }
}

void DisplayUI::showProfileRun(const Profile& prof, float sp, float tF, float tB, 
                               int dutyF, int dutyB, int elapsed, int remaining, 
                               bool done, bool aborted) {
  if (done || aborted) {
    d_.clearDisplay();
    d_.setTextSize(1);
    d_.setTextColor(SSD1306_WHITE);
    d_.setCursor(40, 25);
    d_.print(done ? "COMPLETE" : "ABORTED");
    d_.setCursor(17, 45);
    d_.print("Press to continue");
    d_.display();
    return;
  }
  
  // Clear only the dynamic areas, keep the graph outline
  
  // Top line: Profile name and time
  d_.fillRect(0, 0, 128, 12, SSD1306_BLACK);
  d_.setTextSize(1);
  d_.setCursor(0, 0);
  d_.print(prof.name);
  d_.setCursor(90, 0);
  d_.print(remaining);
  d_.print("s");
  
  // Plot current temperature on the condensed graph
  float avgTemp = (tF + tB) / 2.0f;
  plotTemperature(avgTemp, elapsed);
  
  // Bottom section: Detailed temperature and duty cycle info
  d_.fillRect(0, 42, 128, 22, SSD1306_BLACK);
  
  // Line 1: Setpoint and average temp
  d_.setCursor(0, 43);
  d_.print("SP:");
  d_.print((int)sp);
  d_.print("C Avg:");
  d_.print((int)avgTemp);
  d_.print("C");
  
  // Line 2: Front plate
  d_.setCursor(0, 52);
  d_.print("F:");
  d_.print((int)tF);
  d_.print("C ");
  d_.print(dutyF);
  d_.print("%");
  
  // Line 3: Back plate  
  d_.setCursor(64, 52);
  d_.print("B:");
  d_.print((int)tB);
  d_.print("C ");
  d_.print(dutyB);
  d_.print("%");
  
  d_.display();
}

void DisplayUI::showRun(float spC, float tFrontC, float tBackC, int dutyFrontPct, int dutyBackPct, Mode mode){
  d_.clearDisplay(); d_.setTextColor(SSD1306_WHITE); d_.setTextSize(1);
  d_.setCursor(0,0); d_.print(mode==PROFILE_RUN?"Profile":"Constant");
  d_.setCursor(80,0); d_.print("Set Point:"); d_.print((int)spC); d_.print("C");
  d_.setCursor(0,18); d_.print("Front: "); d_.print((int)tFrontC); d_.print("C  "); d_.print(dutyFrontPct); d_.print("%");
  d_.setCursor(0,32); d_.print("Back : "); d_.print((int)tBackC);  d_.print("C  "); d_.print(dutyBackPct);  d_.print("%");
  int barW=100, fW=constrain(map(dutyFrontPct,0,100,0,barW),0,barW), bW=constrain(map(dutyBackPct,0,100,0,barW),0,barW);
  d_.drawRect(0,46,barW,6,SSD1306_WHITE); d_.fillRect(1,47,max(0,fW-2),4,SSD1306_WHITE);
  d_.drawRect(0,56,barW,6,SSD1306_WHITE); d_.fillRect(1,57,max(0,bW-2),4,SSD1306_WHITE);
  d_.display();
}
