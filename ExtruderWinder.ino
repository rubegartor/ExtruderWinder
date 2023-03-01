#include <TMCStepper.h>
#include <AccelStepper.h>
#include <VL53L0X.h>
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "AiEsp32RotaryEncoder.h"
#include <WiFi.h>
#include <esp_now.h>
#include <LiquidCrystal_I2C.h>
#include <BlockNot.h>
#include <Wire.h>


#define SPOOL_DIR_PIN 33
#define SPOOL_STEP_PIN 32
#define SPOOL_CS_PIN 15
#define SPOOL_MAX_SPEED 10000

#define ALIGNER_DIR_PIN 5
#define ALIGNER_STEP_PIN 4
#define ALIGNER_CS_PIN 2
#define ALIGNER_HOME_SENSOR_PIN 39
#define ALIGNER_FIRST_MOVE -300
#define ALIGNER_MAX_SPEED 10000

#define R_SENSE 0.11f  // TMC2130

#define ROTARY_ENCODER_A_PIN 27       // DT
#define ROTARY_ENCODER_B_PIN 26       // CLK
#define ROTARY_ENCODER_BUTTON_PIN 35  // SW
#define ROTARY_ENCODER_STEPS 4
#define ROTARY_ENCODER_VCC_PIN -1
#define RORATY_ENCODER_ACCELERATION 120

#define LOX_ADDRESS 0x30
#define SHT_LOX 13

typedef struct puller_message {
  uint16_t revs;
  uint32_t speed;
} puller_message;

puller_message pullerMessage;

// Aligner and Spooler consts
const float spoolMotorRatio = 5.18;
const uint16_t stepsPerCm = 476;                 // Steps to travel 1cm
const uint16_t oneRev = 3200 * spoolMotorRatio;  // Steps for 1 revolution at  microsteps
const float filamentDiameter = 0.175;            // 1.75mm

// Tensioner consts
const uint16_t minDistance = 80;
const uint16_t offsetDistance = 100;

byte arrow[] = {
  B10000,
  B11000,
  B11100,
  B11110,
  B11110,
  B11100,
  B11000,
  B10000
};


TMC2130Stepper driverAligner = TMC2130Stepper(ALIGNER_CS_PIN, R_SENSE);
TMC2130Stepper driverSpool = TMC2130Stepper(SPOOL_CS_PIN, R_SENSE);

using namespace TMC2130_n;

AccelStepper spoolMotor(AccelStepper::DRIVER, SPOOL_STEP_PIN, SPOOL_DIR_PIN);
AccelStepper alignerMotor(AccelStepper::DRIVER, ALIGNER_STEP_PIN, ALIGNER_DIR_PIN);


VL53L0X lox;

BlockNot readDistance(200);

enum MotorDirEnum {
  forward,
  backward
};

enum MenuOption {
  toggleSpool,
  viewInfo,
  setSpeed
} menuOption;

#define MENU_OPTIONS_NUMBER 2

//Global data shared between cores
struct gData {
  uint16_t totalRevs;
  bool homed;
  bool canLCD;
  uint16_t actualDistance;
  MenuOption selectedOption;
  bool spool = true;
  uint16_t spoolSpeed = 2000;
};

gData globalData;

typedef struct winder_message {
  uint16_t spoolSpeed;
} winder_message;

winder_message winderMessage;

esp_now_peer_info_t peerInfo;

//C8:F0:9E:A2:91:F8
uint8_t broadcastAddress[] = {0xC8, 0xF0, 0x9E, 0xA2, 0x91, 0xF8};

TaskHandle_t winderTask;
TaskHandle_t alignerTask;


AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

LiquidCrystal_I2C lcd(0x27, 20, 4);

void IRAM_ATTR readEncoderISR() {
  rotaryEncoder.readEncoder_ISR();
}

void initMenu(MenuOption option = toggleSpool) {
  lcd.setCursor(0, 0);
  lcd.print(" ");
  lcd.setCursor(0, 1);
  lcd.print(" ");
  lcd.setCursor(0, 2);
  lcd.print(" ");

  if (option == toggleSpool) {
    lcd.setCursor(0, 0);
  }

  if (option == viewInfo) {
    lcd.setCursor(0, 1);
  }

  if (option == setSpeed) {
    lcd.setCursor(0, 2);
  }

  lcd.write(byte(0));

  lcd.setCursor(1, 0);

  if (globalData.spool) {
    lcd.print("On/Off [On] ");
  } else {
    lcd.print("On/Off [Off]");
  }

  lcd.setCursor(1, 1);
  lcd.print("Resumen");

  lcd.setCursor(1, 2);
  lcd.print("Velocidad");
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Starting...");

  pinMode(SPOOL_CS_PIN, OUTPUT);
  digitalWrite(SPOOL_CS_PIN, HIGH);

  pinMode(ALIGNER_CS_PIN, OUTPUT);
  digitalWrite(ALIGNER_CS_PIN, HIGH);

  SPI.begin();

  pinMode(MISO, INPUT_PULLUP);

  driverAligner.begin();            //Initiate pins and registers
  driverAligner.toff(4);            //off time
  driverAligner.blank_time(24);     //blank time
  driverAligner.rms_current(400);   //600mAh RMS
  driverAligner.microsteps(2);      //2 microsteps

  driverSpool.begin();              //Initiate pins and registers
  driverSpool.toff(4);              //off time
  driverSpool.blank_time(24);       //blank time
  driverSpool.rms_current(600);     //600mAh RMS
  driverSpool.microsteps(16);       //16 microsteps
  driverSpool.en_pwm_mode(true);    //Enable StealthChop
  driverSpool.pwm_autoscale(true);  //StealthChop


  pinMode(SPOOL_DIR_PIN, OUTPUT);
  pinMode(SPOOL_STEP_PIN, OUTPUT);

  digitalWrite(SPOOL_DIR_PIN, LOW);

  pinMode(ALIGNER_DIR_PIN, OUTPUT);
  pinMode(ALIGNER_STEP_PIN, OUTPUT);

  digitalWrite(ALIGNER_DIR_PIN, LOW);

  lcd.init();
  lcd.createChar(0, arrow);
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 1);
  lcd.print("Iniciando bobinador");

  delay(1500);

  Wire.begin();

  if (!lox.init()) {
    Serial.println(F("Failed to boot VL53L0X!"));

    lcd.setCursor(0, 0);
    lcd.print("Failed to boot VL53L0X!");

    while (1);
  }
  
  lox.setTimeout(250);

  Serial.println(F("Starting rotary encoder"));

  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  rotaryEncoder.setBoundaries(-10000000, 10000000, false);  //minValue, maxValue, circleValues true|false (when max go to min and vice versa)
  rotaryEncoder.setAcceleration(RORATY_ENCODER_ACCELERATION);

  Serial.println(F("Starting tasks"));

  pinMode(ALIGNER_HOME_SENSOR_PIN, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);

  delay(500);

  if (esp_now_init() != ESP_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Error initializing ESP-Now"));

    while(1);
  }

  esp_now_register_recv_cb(OnDataRecv);
  
  esp_now_peer_info_t slaveInfo;
  memset(&slaveInfo, 0, sizeof(slaveInfo));
  memcpy(slaveInfo.peer_addr, broadcastAddress, 6);
  slaveInfo.channel = 0;
  slaveInfo.encrypt = false;

  // Add slave
  if (esp_now_add_peer(&slaveInfo) != ESP_OK) {
    return;
  }

  xTaskCreatePinnedToCore(
    winder,             /* Task function. */
    "Winder",           /* name of task. */
    10000,              /* Stack size of task */
    (void*)&globalData, /* parameter of the task */
    10,                 /* priority of the task */
    &winderTask,        /* Task handle to keep track of created task */
    0);                 /* pin task to core 0 */

  xTaskCreatePinnedToCore(
    aligner,            /* Task function. */
    "Aligner",          /* name of task. */
    80000,              /* Stack size of task */
    (void*)&globalData, /* parameter of the task */
    10,                 /* priority of the task */
    &alignerTask,       /* Task handle to keep track of created task */
    1);                 /* pin task to core 1 */
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&pullerMessage, incomingData, sizeof(pullerMessage));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Puller Speed: ");
  Serial.println(pullerMessage.speed);
  Serial.print("Puller revs: ");
  Serial.println(pullerMessage.revs);
  Serial.println();
}

void winder(void* pvParameters) {
  spoolMotor.setMaxSpeed(SPOOL_MAX_SPEED);

  bool run = true;
  bool minTriggered = false;

  for (;;) {
    if (globalData.actualDistance <= minDistance) {
      minTriggered = true;
      run = false;
    }

    if (minTriggered && (globalData.actualDistance - offsetDistance) > 60) {
      run = true;
      minTriggered = false;
    }

    if (globalData.homed && run && globalData.spool) {
      spoolMotor.setSpeed(globalData.spoolSpeed);
      spoolMotor.runSpeed();

      if (spoolMotor.currentPosition() >= oneRev) {
        globalData.totalRevs += 1;
        spoolMotor.setCurrentPosition(0);
      }
    }

    watchDogFeed();
  }
}

bool goToHome() {
  bool homeSensor = digitalRead(ALIGNER_HOME_SENSOR_PIN);

  if (homeSensor == LOW) {
    alignerMotor.runSpeed();
  }

  return homeSensor;
}

void onRotaryEncoderChange(int& lastRotaryEncoderRead) {
  delay(10);  // Debouncing
  int rValue = rotaryEncoder.readEncoder();

  int steps = stepsPerCm * filamentDiameter;

  if (rValue > lastRotaryEncoderRead) {
    steps = -steps;
  }

  if (rValue != lastRotaryEncoderRead) {
    lastRotaryEncoderRead = rValue;

    alignerMotor.setAcceleration(20000);
    alignerMotor.setSpeed(200);
    alignerMotor.runToNewPosition(alignerMotor.currentPosition() + steps);
  }
}

void aligner(void* pvParameters) {
  bool firstMove = false;
  bool triggerMoving = false;  // Variable that sets whether the aligner should move
  uint16_t lastTotalRevs = 0;

  bool isStartPosSet = false;
  bool isEndPosSet = false;
  int spoolEndPos = 0;

  int lastRotaryEncoderRead = 0;

  uint8_t menuPosition = 0;
  bool isResume = false;

  bool isSelectingSpoolSpeed = false;
  int32_t tempSpoolSpeed = -1;

  bool isInSubmenu = false;

  MotorDirEnum motorDir = forward;

  alignerMotor.setMaxSpeed(ALIGNER_MAX_SPEED);
  alignerMotor.setSpeed(1500);

  for (;;) {
    if (!firstMove) {
      alignerMotor.setSpeed(2000);
      alignerMotor.setAcceleration(20000);

      alignerMotor.runToNewPosition(ALIGNER_FIRST_MOVE);  //Blocking call

      firstMove = true;
    }

    if (globalData.canLCD) {
      if (isInSubmenu && globalData.selectedOption == viewInfo) {
        // Aquí podría comprobar si el resultado de `globalData.totalRevs` se ha actualizado desde el ultimo ciclo para ahorrar tiempo al MCU
        lcd.setCursor(0, 0);
        lcd.print("Revs: " + (String)globalData.totalRevs);

        lcd.setCursor(0, 2);

        char actualDistanceBuffer[30];
        sprintf(actualDistanceBuffer, "Tensioner: %-3d", globalData.actualDistance);

        lcd.print(actualDistanceBuffer);
      }

      if (rotaryEncoder.encoderChanged()) {
        delay(10);  // Debouncing
        int rotaryRead = rotaryEncoder.readEncoder();
        bool direction = false;

        if (rotaryRead > lastRotaryEncoderRead) {
          direction = true;
        }

        if (direction && menuPosition < MENU_OPTIONS_NUMBER && !isInSubmenu) {
          menuPosition += 1;
        } else if (menuPosition > 0 && !(direction && menuPosition == MENU_OPTIONS_NUMBER) && !isInSubmenu) {
          menuPosition -= 1;
        }

        if (menuPosition == 0 && !isInSubmenu) {
          initMenu(toggleSpool);
          globalData.selectedOption = toggleSpool;
        } else if (menuPosition == 1 && !isInSubmenu) {
          initMenu(viewInfo);
          globalData.selectedOption = viewInfo;
        } else if (menuPosition == 2) {
          if (tempSpoolSpeed == -1) {
            tempSpoolSpeed = globalData.spoolSpeed;
          }

          if (isSelectingSpoolSpeed && isInSubmenu) {
            if (direction && (tempSpoolSpeed + 10) <= SPOOL_MAX_SPEED) {
              tempSpoolSpeed += 10;
            } else if (tempSpoolSpeed - 10 >= 0) {
              tempSpoolSpeed -= 10;
            }

            lcd.setCursor(0, 0);

            char spoolSpeedBuffer[30] = "";

            sprintf(spoolSpeedBuffer, "Velocidad: %-5d", tempSpoolSpeed);
            lcd.print(spoolSpeedBuffer);

            globalData.spoolSpeed = tempSpoolSpeed;

            winderMessage.spoolSpeed = globalData.spoolSpeed;
            esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &winderMessage, sizeof(winderMessage));
          } else if (!isInSubmenu) {
            initMenu(setSpeed);
            globalData.selectedOption = setSpeed;
          }
        }

        lastRotaryEncoderRead = rotaryRead;
      }

      if (rotaryEncoder.isEncoderButtonClicked()) {
        if (globalData.selectedOption == toggleSpool) {
          globalData.spool = !globalData.spool;
          initMenu(toggleSpool);
        }

        if (globalData.selectedOption == viewInfo) {
          if (isResume) {
            lcd.clear();
            initMenu(toggleSpool);

            menuPosition = 0;
            globalData.selectedOption = toggleSpool;
            isResume = false;
            isInSubmenu = false;
          } else {
            lcd.clear();

            lcd.setCursor(0, 0);
            lcd.print("Revs: " + (String)globalData.totalRevs);
            lcd.setCursor(0, 1);
            lcd.print("Speed: " + (String)globalData.spoolSpeed);
            lcd.setCursor(0, 2);
            lcd.print("Tensioner: " + (String)globalData.actualDistance);
            lcd.setCursor(0, 3);
            lcd.print((String)WiFi.macAddress());

            isResume = true;
            isInSubmenu = true;
          }
        }

        if (globalData.selectedOption == setSpeed) {
          if (isSelectingSpoolSpeed) {
            globalData.spoolSpeed = tempSpoolSpeed;

            menuPosition = 0;
            globalData.selectedOption = toggleSpool;

            lcd.clear();
            initMenu(toggleSpool);

            isSelectingSpoolSpeed = false;
            tempSpoolSpeed = -1;
            isInSubmenu = false;
          } else {
            lcd.clear();

            lcd.setCursor(0, 0);
            lcd.print("Velocidad: " + (String)globalData.spoolSpeed);

            isSelectingSpoolSpeed = true;
            isInSubmenu = true;
          }
        }
      }
    }

    if (firstMove && !globalData.homed) {
      alignerMotor.setSpeed(1500);
      globalData.homed = goToHome();

      if (globalData.homed) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Elige el comienzo de");
        lcd.setCursor(0, 2);
        lcd.print("la bobina");
      }
    }

    if (globalData.totalRevs != lastTotalRevs) {
      lastTotalRevs = globalData.totalRevs;
      triggerMoving = true;
    }

    if (globalData.homed && isEndPosSet && readDistance.TRIGGERED) {
      globalData.actualDistance = getDistance();
    }

    // Now we need start spool position
    if (globalData.homed && (!isStartPosSet && !isEndPosSet)) {
      if (rotaryEncoder.encoderChanged()) {
        onRotaryEncoderChange(lastRotaryEncoderRead);
      }

      if (rotaryEncoder.isEncoderButtonClicked()) {
        alignerMotor.setCurrentPosition(0);
        isStartPosSet = true;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Elige el final de la");
        lcd.setCursor(0, 2);
        lcd.print("bobina");
      }
    }

    // Now we need end spool position
    if (globalData.homed && (isStartPosSet && !isEndPosSet)) {
      if (rotaryEncoder.encoderChanged()) {
        onRotaryEncoderChange(lastRotaryEncoderRead);
      }

      if (rotaryEncoder.isEncoderButtonClicked()) {
        spoolEndPos = alignerMotor.currentPosition();
        isEndPosSet = true;

        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Iniciando proceso");

        alignerMotor.setSpeed(1500);
        alignerMotor.moveTo(0);
        alignerMotor.runToPosition();

        delay(500);

        globalData.canLCD = true;
        lcd.clear();
        initMenu();
      }
    }

    if (globalData.homed && isEndPosSet && triggerMoving) {
      int stepsToGo = -(stepsPerCm * filamentDiameter);

      if (motorDir == backward) {
        stepsToGo = -(stepsToGo);
      }

      alignerMotor.setSpeed(2000);
      alignerMotor.setAcceleration(20000);

      alignerMotor.runToNewPosition(alignerMotor.currentPosition() + stepsToGo);  //Blocking call

      if (alignerMotor.currentPosition() <= spoolEndPos) {
        motorDir = backward;
      }

      if (alignerMotor.currentPosition() >= 0) {  // 0 is always start position of spool
        motorDir = forward;
      }

      triggerMoving = false;
    }

    watchDogFeed();
  }
}

IRAM_ATTR uint16_t getDistance() {
  return lox.readRangeSingleMillimeters();
}

void loop() {}

void watchDogFeed() {
  TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
  TIMERG0.wdt_feed = 1;
  TIMERG0.wdt_wprotect = 0;
}