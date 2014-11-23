/* The code used to create these scripts has come from example scripts included
with the associated libraries, along with scriptes created by M8k3r to support
the control of Roy the Robot.  All attribution is included in the library files,
but not in the test scripts provided.  Outside of the manipulation of example
code provided with the libraries to make Roy move properly, all attribution and
software restrictions are included in the libraries and must be honored. */

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

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define BUTTON_NONE 0
#define BUTTON_DOWN 1
#define BUTTON_RIGHT 2
#define BUTTON_SELECT 3
#define BUTTON_UP 4
#define BUTTON_LEFT 5

const int button1 = 6;  // Button 1 Pin
const int button2 = 3;  // Button 2 Pin
const int button3 = 5;  // Button 3 Pin
const int button4 = 7;  // Button 4 Pin

File writeFile;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);  // Set the servo board

int servomin[9] = {300,300,300,300,300,300,300,300,300};
int servomax[9] = {300,300,300,300,300,300,300,300,300};
int servo = 0,pos = 0,count = 0,command = 0,board = 0;  // initialize other variables
int numofservos = 9;
int numofboards = 1;
int mode = 0;
int showonce = 0, selectionlock = 0, buttonlock = 0, selectlatch = 0, playonce = 0;
int selection = 1;
int pagefile = 0, lastpage = 99;

int setupLoaded, boardsStarted, configLoaded = 0;
String inString = "";

uint8_t readButton(void) {
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
  pinMode(10, OUTPUT);
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  pinMode(button3, INPUT);
  pinMode(button4, INPUT);
  tft.initR(INITR_BLACKTAB); tft.fillScreen(0x0000); tft.setTextSize(1); tft.setTextColor(ST7735_RED); tft.setCursor(0,0); // Setup TFT  
  loadConfig();
  runReset();
  count = 0;
  
}

void loop() {
  runButtons();
}

void runButtons() {
  if(showonce == 0) {
    showHeader();
    writeTXTW("SD Buttons",3,30,2);
    showonce++;
  }

  String files[4] = {"","","",""};
  int filecount = 0, y = 70, buttonlock = 0;
  File dir = SD.open("/SCRIPTS/");
  dir.rewindDirectory();
  for(int z = 0; z < (pagefile * 4); z++) {
    File entry =  dir.openNextFile();
    entry.close();
  }
  while(filecount < 4) {
    File entry =  dir.openNextFile();
    if (! entry) {
      dir.rewindDirectory();
      break;
    }
    files[filecount] = entry.name();
    filecount++;
    entry.close();
  }
  dir.close();
  if (filecount < 4 && filecount > 0) lastpage = pagefile;
  if (filecount == 0) {
    pagefile--;
    lastpage = pagefile;
    return;
  }
  while (filecount < 4) {
    files[filecount] = "           ";
    filecount++;
  }

  for (int z = 0; z < 4; z++) {
    tft.setCursor(3,y);
    tft.setTextSize(1);
    tft.setTextColor(ST7735_BLUE,ST7735_WHITE);
    tft.print(z+1);
    writeTXTW(" - ",10,y,1);
    char charBuf[files[z].length()+1];
    files[z].toCharArray(charBuf,files[z].length()+1);
    writeTXTW(charBuf,30,y,1);
    tft.print("          ");
    y+=10;
  }
  
  while(1) {
    if(digitalRead(button1) == HIGH) {
      String fname = "/SCRIPTS/";
      fname += files[0];
      char charBuf[fname.length()+1];
      fname.toCharArray(charBuf,fname.length()+1);
      if(files[0]!="           ") playSD(charBuf);
      break;
    } else if(digitalRead(button2) == HIGH) {
      String fname = "/SCRIPTS/";
      fname += files[1];
      char charBuf[fname.length()+1];
      fname.toCharArray(charBuf,fname.length()+1);
      if(files[1]!="           ") playSD(charBuf);
      break;
    } else if(digitalRead(button3) == HIGH) {
      String fname = "/SCRIPTS/";
      fname += files[2];
      char charBuf[fname.length()+1];
      fname.toCharArray(charBuf,fname.length()+1);
      if(files[2]!="           ") playSD(charBuf);
      break;
    } else if(digitalRead(button4) == HIGH) {
      String fname = "/SCRIPTS/";
      fname += files[3];
      char charBuf[fname.length()+1];
      fname.toCharArray(charBuf,fname.length()+1);
      if(files[3]!="           ") playSD(charBuf);
      break;
    }
    uint8_t b = readButton();
    if (b == BUTTON_DOWN && buttonlock == 0) {
      pagefile++;
      if(pagefile > lastpage) pagefile--;
      buttonlock = 1;
    }
    if (b == BUTTON_UP && buttonlock == 0) {
      pagefile--;
      if(pagefile < 0) pagefile = 0;
      buttonlock = 1;
    }
    if (b == BUTTON_NONE && buttonlock == 1) {
      buttonlock = 0;
      break;
    }
    checkSelect();
    if(mode==9) break;
  }
}

void runReset() {
  for (int z = 0; z < 6; z++) pwm.setPWM(z,0,servomax[z]);
  pwm.setPWM(6,0,(((servomin[6]+servomax[6])/2)+40));
  pwm.setPWM(7,0,(((servomin[7]+servomax[7])/2)-40));
  pwm.setPWM(8,0,((servomin[8]+servomax[8])/2));
  mode = 10;
  selectionlock = 0;
}

void playSD(char* filename) {
  inString = "";
  int cmdcount = 0;
  showHeader ();
  writeTXTW("Running:",3,30,2);
  writeTXTW(filename,3,50,1);
  writeTXTW("SELECT to Stop",3,70,1);
  writeFile = SD.open(filename);
  if(writeFile) {
    uint32_t startTime = millis();
    while (writeFile.available()) {
      int inChar = writeFile.read();    // read in the byte
      if (isDigit(inChar) || inChar == '-') {
        inString += (char)inChar;
      }
      if (inChar == ',') {
        if (inString != "-1") {
          int servopos = inString.toInt();
          pwm.setPWM(cmdcount, 0, map(servopos,0,254,servomin[cmdcount],servomax[cmdcount]));
        }
        cmdcount++;
        inString = "";
      }
      if (inChar == '\n') {
        checkSelect();
        cmdcount = 0;
        inString = "";
        delay((33-(millis() - startTime)));
        startTime = millis();
      }
    }
    if(writeFile) writeFile.close();
    if(mode!=9) playonce = 1;
    showonce = 0;
  } else {
    tft.println(F("Could not open file"));
  }
}

void loadConfig () {
  tft.println(F("Roy OS\n\nLoading...\n\nInitializing SD card"));
  if (!SD.begin(SD_CS)) {
    tft.println(F("initialization Failed"));
    return;
  }
  tft.println(F("initialization Done"));

  if ((writeFile = SD.open("royconf.txt")) == NULL) {
    tft.println(F("File not found!\n"));
    writeFile.close();
    delay(10000);
    return;
  }
  if (writeFile) {
    tft.println(F("Reading Config"));
    int currServo = 0;
    int currPos = 0;
    int confcount = 0;
    String currData = "";
    numofboards = 1;
    numofservos = 9;
    while (writeFile.available()) {
      int inChar = writeFile.read();
      if (isDigit(inChar)) {
        inString += (char)inChar;
      }
      if (inChar == '\n') {
        if (currServo < numofservos) {
          if (currPos == 0) {
            servomin[currServo] = inString.toInt();
            currData += "Servo ";
            currData +=currServo; currData +=": ";
            currData +=servomin[currServo];
            currPos++;
          }
          else if (currPos == 1) {
            servomax[currServo] = inString.toInt();
            currData += ",";
            currData += servomax[currServo];
            currPos++;
            tft.println(currData);
            currPos = 0;
            currServo++;
            currData = "";
          }
          inString = "";
        }
        if (confcount > 1 && currServo == numofservos) configLoaded = 1;
        confcount++;  // loop logic
      }
    }
    writeFile.close(); // close the file
    if (configLoaded == 1) {
      startupBoards();
      boardsStarted = 1;
      tft.println(F("\nBoards Started"));
      delay(200);
    } else {
      tft.println(F("Error in Config File"));
      delay(10000);
    }
  } else {
    tft.println(F("\n\nPress RESET to Reboot")); // if the file didn't open, print an error
  }
}

void startupBoards () {
  pwm.begin();  // Start the boards
  pwm.setPWMFreq(50);  // Set PWM frequency - 50 HZ or 60 HZ
}

void updateTXT (char* newtxt, uint16_t newcolor, uint16_t newback, uint8_t x, uint8_t y, uint8_t fsize) {
  tft.setCursor(x,y);
  if(fsize) tft.setTextSize(fsize);
  if(newtxt != "") {
    tft.setTextColor(newcolor,newback);
    tft.print(newtxt);
  }
}

void writeTXTW (char* newtxt, uint8_t x, uint8_t y, uint8_t fsize) {
  tft.setCursor(x,y);
  if(newtxt != "") {
    if(fsize>0) tft.setTextSize(fsize);
    tft.setTextColor(ST7735_BLUE,ST7735_WHITE);
    tft.print(newtxt);
  }
}

void showHeader () {
  tft.setTextSize(2);
  tft.fillScreen(0xFFFF);
  tft.setTextColor(ST7735_BLUE);
  tft.setCursor(3,3);
  tft.println(F("RoyOS V0.9"));
}

void checkSelect() {
  uint8_t b = readButton();
  if (b == BUTTON_SELECT) {
    selectlatch = 1;
  }
  if (b != BUTTON_SELECT && selectlatch == 1) {
    showonce = 0;
    selectionlock = 0;
    mode = 9;
    selectlatch = 0;
    playonce = 0;
    if(writeFile) writeFile.close();
    runReset();
  }
}
