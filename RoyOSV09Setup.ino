/* The code used to create these scripts has come from example scripts included
with the associated libraries, along with scriptes created by M8k3r to support
the control of Roy the Robot. All attribution is included in the library files,
but not in the test scripts provided. Outside of the manipulation of example code
provided with the libraries to make Roy move properly, all attribution and
software restrictions are included in the libraries and must be honored.
Most all of the example scripts that have been modified were originally written
by Limor Fried/Ladyada for Adafruit Industries. */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SD.h>
#include <SPI.h>

#define SD_CS    4  // Chip select line for SD card
#define TFT_CS  10  // Chip select line for TFT display
#define TFT_DC   8  // Data/command line for TFT
#define TFT_RST  0  // Reset line for TFT (or connect to +5V)

#define BUTTON_NONE 0    // Define joystick variables
#define BUTTON_DOWN 1    // Define joystick variables
#define BUTTON_RIGHT 2   // Define joystick variables
#define BUTTON_SELECT 3  // Define joystick variables
#define BUTTON_UP 4      // Define joystick variables
#define BUTTON_LEFT 5    // Define joystick variables

#define NUMOFSERVOS  9  // Number of servos being run

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);  // Create servo board object

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);  // Create TFT object

File writeFile;  // Create the SD card file object

int servomin[9] = {300,300,300,300,300,300,300,300,300};  // Default servo min values to 300
int servomax[9] = {300,300,300,300,300,300,300,300,300};  // Default servo max values to 300
uint8_t servo = 0 ,count = 0 ,mode = 0;  // Init variables
boolean selectionlock = 0, buttonlock = 0, command = 0, showonce = 0, selectlatch = 0, playonce = 0, setupLoaded = 0, boardsStarted = 0, configLoaded = 0;  // Init variables
uint8_t pos = 0;  // Init variables
uint8_t selection = 1;  // Init variables
String inString = "";  // Init variables

uint8_t readButton(void) {  // Return the value of the joystick
  float a = analogRead(3);
  
  a *= 5.0;
  a /= 1024.0;
  
  if (a < 0.2) return BUTTON_DOWN;
  if (a < 1.0) return BUTTON_RIGHT;
  if (a < 1.5) return BUTTON_SELECT;
  if (a < 2.0) return BUTTON_UP;
  if (a < 3.2) return BUTTON_LEFT;
  else return BUTTON_NONE;
}

void setup() {
  pinMode(10, OUTPUT);        // Required for SD
  tft.initR(INITR_BLACKTAB);  // Init TFT  
  loadConfig();               // Load the servo settings from the SD card
}

void loop() {
  runSetup();     // If mode = 5, run the Setup function to set servo min / max values
  tft.println(F("Setup Complete"));
  while(1);
}

void runReset() {
  for(int h = 0; h < 6; h++) pwm.setPWM(h,0,servomax[h]);
  pwm.setPWM(6,0,((servomin[6]+servomax[6])/2)+30);
  pwm.setPWM(7,0,((servomin[7]+servomax[7])/2)-30);
  pwm.setPWM(8,0,((servomin[8]+servomax[8])/2));
  mode = 9;
  selectionlock = 0;
}

void loadConfig () {
  if (!SD.begin(SD_CS)) {
    tft.println(F("SD NA"));
    return;
  }
  if ((writeFile = SD.open("royconf.txt")) == NULL) {
    writeFile.close();
    runSetup();
  }
  if (writeFile) {
    int currServo = 0;
    int currPos = 0;
    int confcount = 0;
    while (writeFile.available()) {
      uint8_t inChar = writeFile.read();
      if (isDigit(inChar)) {
        inString += (char)inChar;
      }
      if (inChar == '\n') {
        if (currServo < NUMOFSERVOS) {
          if (currPos == 0) {
            servomin[currServo] = inString.toInt();
            currPos++;
          }
          else if (currPos == 1) {
            servomax[currServo] = inString.toInt();
            currPos = 0;
            currServo++;
          }
          inString = "";
        }
        if (currServo == NUMOFSERVOS) configLoaded = 1;
        confcount++;  // loop logic
      }
    }
    writeFile.close(); // close the file
    if (configLoaded == 1) {
      startupBoards();
      boardsStarted = 1;
      runReset();
    } else {
      tft.println(F("Conf File Bad"));
    }
  } else {
    tft.println(F("\n\nReboot")); // if the file didn't open, print an error
  }
}

void runSetup() {
  int setupStep = 1;
  int saveConfig = 0;
  int y = 0;
  int selectLatch = 0;
  while(setupStep <= 4) {
    if(setupStep == 1) {
      tft.setTextSize(1);
      tft.setTextColor(ST7735_RED);
      tft.fillScreen(0x0000);
      tft.setCursor(0,0);
      tft.println("Roy Setup");
      setupStep++;
    }
    if (setupStep == 2) {
      startupBoards();
      tft.setCursor(0,10); tft.print(F("Servo:"));
      tft.setCursor(50,10); tft.print(F("Min:"));
      tft.setCursor(80,10); tft.print(F("Max:"));
      for(int setupLoop = 0; setupLoop < (NUMOFSERVOS - 3); setupLoop++) {
        y = 20 + (setupLoop*10);
        tft.setCursor(0,y);
        tft.print(setupLoop);
        servomin[setupLoop]=getLimit(servomin[setupLoop],50,y,0,1024,setupLoop);
        servomax[setupLoop]=getLimit(servomax[setupLoop],80,y,0,1024,setupLoop);
        pwm.setPWM(setupLoop,0,servomax[setupLoop]);
      }

      int avg6 = ((servomin[6]+servomax[6])/2);
      int avg7 = ((servomin[7]+servomax[7])/2);
      int offset = 0;

      y +=10;
      tft.setCursor(0,y);
      tft.print(F("Offset:"));
      offset = getOffset(80,y,avg6,avg7);

      y +=10;
      avg6 = servomin[6];
      avg7 = servomin[7] - offset;

      tft.setCursor(0,y);
      tft.print(F("W Down:"));
      tft.setTextColor(ST7735_RED,ST7735_BLACK);
      tft.setCursor(50,y);
      tft.print(avg6);
      tft.setCursor(80,y);
      tft.print(avg7);
      while(1) {
        uint8_t b = readButton();
        if (b == BUTTON_UP) {
          avg6++;
          avg7--;
          if(avg6 > 1024) avg6 = 1024;
          if(avg7 < 0) avg7 = 0;
          tft.setTextColor(ST7735_RED,ST7735_BLACK);
          tft.setCursor(50,y);
          tft.print(avg6);
          tft.setCursor(80,y);
          tft.print(avg7);
        }
        if (b == BUTTON_RIGHT) {
          avg6+=10;
          avg7-=10;
          if(avg6 > 1024) avg6 = 1024;
          if(avg7 < 0) avg7 = 0;
          tft.setTextColor(ST7735_RED,ST7735_BLACK);
          tft.setCursor(50,y);
          tft.print(avg6);
          tft.setCursor(80,y);
          tft.print(avg7);
        }
        if (b == BUTTON_DOWN) {
          avg6--;
          avg7++;
          if(avg6 < 0) avg6 = 0;
          if(avg7 > 1024) avg7 = 1024;
          tft.setTextColor(ST7735_RED,ST7735_BLACK);
          tft.setCursor(50,y);
          tft.print(avg6);
          tft.setCursor(80,y);
          tft.print(avg7);
        }
        if (b == BUTTON_LEFT) {
          avg6-=10;
          avg7+=10;
          if(avg6 < 0) avg6 = 0;
          if(avg7 > 1024) avg7 = 1024;
          tft.setTextColor(ST7735_RED,ST7735_BLACK);
          tft.setCursor(50,y);
          tft.print(avg6);
          tft.setCursor(80,y);
          tft.print(avg7);
        }
        if (b == BUTTON_SELECT) {
          selectLatch = 1;
        }
        if (b != BUTTON_SELECT && selectLatch == 1) {
          selectLatch = 0;
          break;
        }
        pwm.setPWM(6,0,avg6);
        pwm.setPWM(7,0,avg7);
        delay(150);
      }
      servomin[6] = avg6;
      servomin[7] = avg7;

      y +=10;
      avg6 = servomax[6];
      avg7 = servomax[7] - offset;

      tft.setCursor(0,y);
      tft.print(F("W Up:"));
      tft.setTextColor(ST7735_RED,ST7735_BLACK);
      tft.setCursor(50,y);
      tft.print(avg6);
      tft.setCursor(80,y);
      tft.print(avg7);
      while(1) {
        uint8_t b = readButton();
        if (b == BUTTON_UP) {
          avg6++;
          avg7--;
          if(avg6 > 1024) avg6 = 1024;
          if(avg7 < 0) avg7 = 0;
          tft.setTextColor(ST7735_RED,ST7735_BLACK);
          tft.setCursor(50,y);
          tft.print(avg6);
          tft.setCursor(80,y);
          tft.print(avg7);
        }
        if (b == BUTTON_RIGHT) {
          avg6+=10;
          avg7-=10;
          if(avg6 > 1024) avg6 = 1024;
          if(avg7 < 0) avg7 = 0;
          tft.setTextColor(ST7735_RED,ST7735_BLACK);
          tft.setCursor(50,y);
          tft.print(avg6);
          tft.setCursor(80,y);
          tft.print(avg7);
        }
        if (b == BUTTON_DOWN) {
          avg6--;
          avg7++;
          if(avg6 < 0) avg6 = 0;
          if(avg7 > 1024) avg7 = 1024;
          tft.setTextColor(ST7735_RED,ST7735_BLACK);
          tft.setCursor(50,y);
          tft.print(avg6);
          tft.setCursor(80,y);
          tft.print(avg7);
        }
        if (b == BUTTON_LEFT) {
          avg6-=10;
          avg7+=10;
          if(avg6 < 0) avg6 = 0;
          if(avg7 > 1024) avg7 = 1024;
          tft.setTextColor(ST7735_RED,ST7735_BLACK);
          tft.setCursor(50,y);
          tft.print(avg6);
          tft.setCursor(80,y);
          tft.print(avg7);
        }
        if (b == BUTTON_SELECT) {
          selectLatch = 1;
        }
        if (b != BUTTON_SELECT && selectLatch == 1) {
          break;
          selectLatch = 0;
        }
        pwm.setPWM(6,0,avg6);
        pwm.setPWM(7,0,avg7);
        delay(150);
      }
      servomax[6] = avg6;
      servomax[7] = avg7;
      
      avg6 = ((servomin[6]+servomax[6])/2);
      avg7 = ((servomin[7]+servomax[7])/2);
      pwm.setPWM(6,0,avg6+30);
      pwm.setPWM(7,0,avg7-30);

      y+=10;
      tft.setCursor(0,y);
      tft.print(F("8"));
      servomin[8]=getLimit(servomin[8],50,y,0,1024,8);
      servomax[8]=getLimit(servomax[8],80,y,0,1024,8);

      setupLoaded = 1;
      runReset();
      y +=10;
      tft.setCursor(0,y);
      tft.print(F("Save Config? "));
      saveConfig = saveSettings(80,y);
      if(saveConfig == 2) setupStep = 3;
      if(saveConfig == 1) { setupStep = 1; runReset(); selectLatch = 0; }
    }
    if (setupStep == 3) {
      tft.fillScreen(0x0000);
      tft.setCursor(0,0);
      tft.println(F("Saving config..."));
      if (SD.exists("royconf.txt")) { SD.remove("royconf.txt"); tft.println(F("Prev file del")); }
      tft.println(F("Creat file"));
      if ((writeFile = SD.open("royconf.txt", FILE_WRITE)) == NULL) {
        tft.println(F("Error opening file"));
        delay(10000);
        setupStep = 4;
      } else {
        tft.println(F("File open"));
        for(int writeloop = 0; writeloop < NUMOFSERVOS; writeloop++) {
          writeFile.println(servomin[writeloop]);
          writeFile.println(servomax[writeloop]);
          tft.print(F("Servo ")); tft.print(writeloop); tft.println(F(" saved"));
        }
        writeFile.close();
        tft.println(F("File closed"));
        tft.println(F("Saved"));
        setupStep = 4;
      }
    }
    if (setupStep == 4) {
      mode = 9;
      selectionlock = 0;
      setupStep = 5;
      runReset();
    }
  }
}

void startupBoards () {
  pwm.begin();  // Start the boards
  pwm.setPWMFreq(50);  // Set PWM frequency - 50 HZ or 60 HZ
}

int getLimit(uint16_t v, uint8_t x, uint8_t y, uint16_t l, uint16_t h, uint8_t serv) {
  int selectLatch = 0;
  tft.setCursor(x,y);
  tft.print(v);
  while(1) {
    uint8_t b = readButton();
    if (b == BUTTON_UP) {
      tft.setTextColor(0x0000);
      tft.setCursor(x,y);
      tft.print(v);
      v++;
      if(v > h) v = h;
      tft.setTextColor(ST7735_RED);
      tft.setCursor(x,y);
      tft.print(v);
    }
    if (b == BUTTON_RIGHT) {
      tft.setTextColor(0x0000);
      tft.setCursor(x,y);
      tft.print(v);
      v+=10;
      if(v > h) v = h;
      tft.setTextColor(ST7735_RED);
      tft.setCursor(x,y);
      tft.print(v);
    }
    if (b == BUTTON_DOWN) {
      tft.setTextColor(0x0000);
      tft.setCursor(x,y);
      tft.print(v);
      v--;
      if(v < l) v = l;
      tft.setTextColor(ST7735_RED);
      tft.setCursor(x,y);
      tft.print(v);
    }
    if (b == BUTTON_LEFT) {
      tft.setTextColor(0x0000);
      tft.setCursor(x,y);
      tft.print(v);
      if (v > 10) v-=10; else v = l;
      if(v < l) v = l;
      tft.setTextColor(ST7735_RED);
      tft.setCursor(x,y);
      tft.print(v);
    }
    if (b == BUTTON_SELECT) {
      selectLatch = 1;
    }
    if (b != BUTTON_SELECT && selectLatch == 1) {
      return(v);
    }
    pwm.setPWM(serv,0,v);
    delay(150);
  }
}

int getOffset(uint8_t x, uint8_t y, uint16_t a6, uint16_t a7) {
  int v = 0;
  uint8_t selectLatch = 0;
  tft.setCursor(x,y);
  tft.print(v);
  pwm.setPWM(6,0,a6);
  while(1) {
    uint8_t b = readButton();
    if (b == BUTTON_UP) {
      v++;
      if(v > 200) v = 200;
      tft.setTextColor(ST7735_RED, ST7735_BLACK);
      tft.setCursor(x,y);
      tft.print(v);
      tft.print("  ");
    }
    if (b == BUTTON_RIGHT) {
      v+=10;
      if(v > 200) v = 200;
      tft.setTextColor(ST7735_RED, ST7735_BLACK);
      tft.setCursor(x,y);
      tft.print(v);
      tft.print("  ");
    }
    if (b == BUTTON_DOWN) {
      v--;
      if(v < -200) v = -200;
      tft.setTextColor(ST7735_RED, ST7735_BLACK);
      tft.setCursor(x,y);
      tft.print(v);
      tft.print("  ");
    }
    if (b == BUTTON_LEFT) {
      v-=10;
      if(v < -200) v = -200;
      tft.setTextColor(ST7735_RED, ST7735_BLACK);
      tft.setCursor(x,y);
      tft.print(v);
      tft.print("  ");
    }
    if (b == BUTTON_SELECT) {
      selectLatch = 1;
    }
    if (b != BUTTON_SELECT && selectLatch == 1) {
      return(v);
    }
    pwm.setPWM(7,0,(a7-v));
    delay(150);
  }
}

int saveSettings(uint8_t x, uint8_t y) {
  int v = 2;
  int selectLatch = 0;
  tft.setCursor(x,y);
  tft.print("Yes");
  while(1) {
    uint8_t b = readButton();
    if (b == BUTTON_UP) {
      if(v < 2) v++; else v = 2;
      tft.setTextColor(ST7735_RED,ST7735_BLACK);
      tft.setCursor(x,y);
      if(v==1) tft.print(F("No "));
      if(v==2) tft.print(F("Yes"));
    }
    if (b == BUTTON_DOWN) {
      if(v > 1) v--; else v = 1;
      tft.setTextColor(ST7735_RED, ST7735_BLACK);
      tft.setCursor(x,y);
      if(v==1) tft.print(F("No "));
      if(v==2) tft.print(F("Yes"));
    }
    if (b == BUTTON_SELECT) {
      selectLatch = 1;
    }
    if (b != BUTTON_SELECT && selectLatch == 1) {
      return(v);
    }
  }
}
