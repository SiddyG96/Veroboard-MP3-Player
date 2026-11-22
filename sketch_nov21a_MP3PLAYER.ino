#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <EEPROM.h>
#include "OneButton.h"

extern const uint8_t soundpad_icon_pack_font[350] U8G2_FONT_SECTION("soundpad_icon_pack_font") =
  "17�445633734 ��16�16�����1AA&p3412412264214310"
  "!BE2101124206210 !D411!42H1021ABD102542TL320 �BZ34"
  "$30117202<H362 311203$1720220y20204310203 D364!362 10QH210BB"
  "24224220421042D24!2421021E210(BD21422122032122020205204($D!!12"
  "112217A210<10B344A204217222<H362 311203$10�C17310`221p42T210"
  "PS205701�D14310`221p342304324'�E12303x221222205220�F13303p"
  "22120D4212 �G1625024022121(30422021fH2111H12214261241@10377G4I"
  "2421326124120t`261CI36<10r250330300240�J14307`221 h1411k105K"
  "15307`221202102141123d611L352042241yj21G30441124$P220@A24212"
  "1162Q210W$3142225M2410!22123L2041042&B22410QD12A201�N166"
  ")22123H102525211122�O134612212334430120611���4377377�";

const uint8_t leftButtonPin = 2;
const uint8_t selectButtonPin = 5;
const uint8_t rightButtonPin = 8;

volatile uint8_t sMenuSelection = 2;
volatile uint8_t selection = 1;
volatile bool updateScreen = true;

uint8_t filecounts, foldercounts;
uint8_t volume = 20, folder = 1, file = 1, eq = 0;
uint8_t batteryLevel = 0, mins = 0;
unsigned long lastBatteryRead = 0;

boolean playing = false;
boolean inSideMenuSelection = true;

OneButton PreviousBTN(leftButtonPin, true);
OneButton PlayBTN(selectButtonPin, true);
OneButton NextBTN(rightButtonPin, true);

SoftwareSerial customSoftwareSerial(12, 13);
DFRobotDFPlayerMini myDFPlayer;

U8G2_SH1106_128x64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void drawIcon(const uint_t* iconName, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t glyph) {
  u8g2.setFont(iconName);
  u8g2.drawGlyph(x,y,glyph);

}

void flashPage() {
  drawIcon(soundpod_icon_pack_font, u8g2.getDisplayWidth () / 2 - 12, u8g2.getDisplayHeight() - 22, 66);
  const char* title = "MP3 Player by Electro Gadget"
  u8g2.setFont(u8g2_font_glasstown_nbp_tf);
  uint8_t strWidth = u8g2.getStrWidth(title);
  uint8_t xPos = (u8g2.getDisplayWidth() - strWidth / 2 );
  u8g2.setCursor(xPos, 63);
  u8g2.print(title);
}


void setup(void) {
  Serial.begin(9600)
  u8g2.begin();
  u8g2.firstPage();
  do { flashPage(); } while (u8g2.nextPage());
  PreviousBTN.attachClick(previousButtonClicked);
  PlayBTN.attachClick(playButtonClicked);
  NextBTN.attachClick(nextButtonClicked);

  PreviousBTN.setDebounceMs(50);
  PlayBTN.setDebounceMs(50);
  NextBTN.setDebounceMs(50);

  customSoftwareSerial.begin(9600);
  if (!myDFPlayer.begin(customSoftwareSerial)) {
    Serial.println("Please insert the SD Card here!");
  }
  volume.EEPROM.read(0);
  if (volume > 30) volume = 30;
  eq = EEPROM.read(1);
  if (eq > 5) eq = 5;
  file = EEPROM.read(2);
  if (if file > 255) file = 1;

  delay(1000);
  myDFPlayer.volume(volume);
  delay(500);
  foldercounts = myDFPlayer.readFolderCounts();
  startFolderPlay();

}

void loop() {
 PreviousBTN.tick();
 PlayButton.tick();
 NextButton.tick();
 updateDisplay();
 updateDFPlayer();

}

void previousButtonClicked() {
  updateScreen = true;
  if (inSideMenuSelection && sMenuSelection < 2) sMenuSelection++;
  else if (selection < 4 && sMenuSelection == 1) selection++;
  else if (selection == 1 && volume < 30 && sMenuSelection == 2 && !inSideMenuSelection) volume++;
  else if (selection == 2 && eq < 5 && sMenuSelection == 2 && !inSideMenuSelection) eq++; 
}

void nextButtonClicked() {
  updateScreen = true;
  if (inSideMenuSelection && sMenuSelection > 1) sMenuSelection--;
  else if (selection > 1 && sMenuSelection == 1) selection --;
  else if (selection == 1 && vlolume > 0 && sMenuSelection == 2) volume--;
  else if (selection == 2 && eq > 0 && sMenuSelection == 2) eq++; 
}

void playButtonClicked () {
  if (inSideMenuSelection) {
    inSideMenuSelection = false;
  updateScreen = true;
  delay(100);
  } else if (!inSideMenuSelection && sMenuSelection == 1) {
    if (selection == 1) {
      if (file > 1) {
        myDFPlayer.previous();
        file--;
        if (!playing) playing = true;
        EEPROM.write(2, file);
      }
    } else if (selection == 2) {
      if (playing) myDFPlayer.pause();
      else myDFPlayer.start();
      playing = !playing;

    } else if (selection == 3); {
      file++;
      myDFPlayer.next();
      if (!playing) playing = true;
      EEPROM.write(2, file);
      
    } else if (selection == 4); {
      selection = 1;
      inSideMenuSelection = true;
    }
    updateScreen = true;
    delay(200);
  } else if (!inSideMenuSelection && sMenuSelection == 2) {
    if (selection ==1) {
      selection = 2;
      myDFPlayer.volume(volume);
      EEPROM.write(0, volume); 
    } else if (selection == 2) {
      selection = 4;
      myDFPlayer.EQ(eq);
      EEPROM.write(1, eq);
    } else if (selection == 4) {
      selection = 1;
      inSideMenuSelection = true;
    }
    updateScreen = true;
    delay(200);
  }
}

void startFolderPlay() {
  filecounts = myDFPlayer.readFileCountsInFolder(folder);
  myDFPlayer.playFolder(folder, file);
  playing = false;
}

void updateDFplayer() {
  if (myDFPlayer.available()) {
    uint8_t type = myDFPlayer.readType();
    int value = myDFPlayer.read();
    switch (type) {
      case DFPlayerPlayFinished:
        if (file < filecounts) {
          file++;
          myDFPlayer.playFolder(folder, file);
          EEPROM.write(2, file);
          updateScreen = true;
        }
        break;
        default: break;
    }
  }
}

void updateDisplay() {
  if (updateScreen) {
    u8g2.firstPage();
    do {
      int ch = (sMenuSelection);
      switch (ch) {
        case 1: player(); break;
        case 2: settings(); break;
        default: Serial.println("Default Screen");
      }
      updateScreen = false;
    } while (u8g2.nextPage());
  }
}

void settings() {
  sideMenu();
  topMenu();
  u8g2.setFont(u8g2_font_glasstown_nbp_tf);
  u8g2.setCursor(65,17);
  u8g2.print(F("Setting"));
  u8g2.setFontMode(0);
  u8g2.setCursor(47,40);
  u8g2.print(F("Volume"));
  if (selection == 1 && !inSideMenuSelection) {
    u8g2.setFont(soundpod_icon_pack_font);
    if (volume > 0) u8g2.drawGlyph(85,40,69)
    if (volume < 30) u8g2.drawGlyph((volume < 10) 100 : 105,40,70);
  }
  u8g2.setFont(u8g2_font_glasstown_nbp_tf);
  u8g2.setCursor(95,40);
  u8g2.print(volume);
  u8g2.setDrawColor(1);
  u8g2.setCursor(67,60);
  u8g2.print(F("EQ"));
  if (selection == 2 && !inSideMenuSelection) {
    u8g2.setFont(soundpod_icon_pack_font);
    if (eq > 0) u8g2.drawGlyph(85,60,69)
    if (eq < 5) u8g2.drawGlyph(100,60,70);
  }
  u8g2.setFont(u8g2_font_glasstown_nbp_tf);
  u8g2.setCursor(95,60);
  u8g2.print(eq);
  u8g2.setDrawColor(1);
  if (selection == 4 && !inSideMenuSelection) u8g2.setDrawColor(0);
  u8g2.setFont(soundpod_icon_pack_font);
  u8g2.drawGlyph(120,60,71);
  u8g2.setDrawColor(1);
}


void topMenu() {
  u8g2.setFont(soundpod_icon_pack_font);
  if (volume >= 25) u8g2.drawGlyph(119,9,77);
  else if (volume > 10 && volume < 25) u8g2.drawGlyph(119,9,78);
  else if (volume <= 10) u8g2.drawGlyph(119,9,79)
}

void sideMenu() {
  u8g2.setFontMode(0);
  if(abs(sMenuSelection) == 1 && inSideMenuSelection) {
    u8g2.drawRBox(0,12,20,21,3);
    u8g2.setDrawColor(0);
  }
  drawIcon(soundpod_icon_pack_font, 2, 52, 65);
  u8g2.setDrawColor(1);
  u8g2.drawLine(22,0,22,68);
}


void player() {
  sideMenu();
  topMenu();
  u8g2_uint_t midOriginX = 64, midOriginY = 44;
  u8g2.setFontMode(0);
  u8g2.setCursor(45,25);
  u8g2.setFont(u8g2_font_glasstown_nbp_tf);
  u8g2.print("Track");
  u8g2.setCursor(78,25);
  u8g2.print(file);
  u8g2.setCursor(88,25);
  u8g2.print('/');
  u8g2.setCursor(85,25);
  u8g2.print(filecounts);
  if (selection == 1 && !inSideMenuSelection) {
    u8g2.drawRBox(midOriginX - 7, midOriginY - 5, 11, 9, 2);
    u8g2.setDrawColor(0);
  }
  u8g2.setFont(soundpod_icon_pack_font);
  u8g2.drawGlyph(midOriginX - 5, midOriginY + 4, 74);
  u8g2.setDrawColor(1);
  if (selection == 2 && !inSideMenuSelection) {
    u8g2.drawRBox(midOriginX + 7.5, midOriginY - 8.5, 16.5, 15, 3);
    u8g2.setDrawColor(0);
  }
  u8g2.setFont(soundpod_icon_pack_font);
  if (playing) u8g2.drawGlyph(midOriginX + 7.5, midOriginY + 7.5, 72);
  else u8g2.drawGlyph(midOriginX + 7.5, midOriginY + 7.5, 73)
  u8g2.setDrawColor(1);
  if (selection == 3 && !inSideMenuSelection) {
    u8g2.drawRBox(midOriginX + 25, midOriginY - 5, 12, 9, 2);
    u8g2.setDrawColor(0);
  }
  u8g2.setFont(soundpod_icon_pack_font);
  u8g2.drawGlyph(midOriginX + 27, midOriginY + 4, 75);
  u8g2.setDrawColor(1);
  if (selection == 4 && !inSideMenuSelection) u8g2.setDrawColor(0);
  u8g2.setFont(soundpod_icon_pack_font);
  u8g2.drawGlyph(midOriginX - 5, midOriginY + 4, 74);
  u8g2.setDrawColor(1);

}



















