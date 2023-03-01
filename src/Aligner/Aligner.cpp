#include <Arduino.h>
#include <Aligner/Aligner.h>
#include <Commons/Commons.h>
#include <AccelStepper.h>
#include <LCD/LCDMenu.h>
#include <Connection/Connection.h>
#include <BlockNot.h>

enum MotorDirEnum {
  forward,
  backward
};

AccelStepper alignerMotor(AccelStepper::DRIVER, ALIGNER_STEP_PIN, ALIGNER_DIR_PIN);

BlockNot readDistance(200);

bool goToHome() {
  bool homeSensor = digitalRead(ALIGNER_HOME_SENSOR_PIN);

  if (homeSensor == LOW) {
    alignerMotor.runSpeed();
  }

  return homeSensor;
}

void onRotaryEncoderChange(int& lastRotaryEncoderRead) {
  delay(10);  // Debouncing
  int rValue = rotaryEncoder.value();

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

    if (canLCD) {
      if (isInSubmenu && selectedOption == viewInfo) {
        // Aquí podría comprobar si el resultado de `totalRevs` se ha actualizado desde el ultimo ciclo para ahorrar tiempo al MCU
        lcdMenu.println("Revs: " + (String)totalRevs, 0);

        char actualDistanceBuffer[30];
        sprintf(actualDistanceBuffer, "Tensioner: %-3d", actualDistance);

        lcdMenu.println(actualDistanceBuffer, 2);
      }

      if (rotaryEncoder.changed()) {
        delay(10);  // Debouncing
        int rotaryRead = rotaryEncoder.value();
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
          lcdMenu.initMenu(toggleSpool);
          selectedOption = toggleSpool;
        } else if (menuPosition == 1 && !isInSubmenu) {
          lcdMenu.initMenu(viewInfo);
          selectedOption = viewInfo;
        } else if (menuPosition == 2) {
          if (tempSpoolSpeed == -1) {
            tempSpoolSpeed = spoolSpeed;
          }

          if (isSelectingSpoolSpeed && isInSubmenu) {
            if (direction && (tempSpoolSpeed + 10) <= 10000) { // TODO: El valor 10000 debe equivaler a la velocidad máxima del winder
              tempSpoolSpeed += 10;
            } else if (tempSpoolSpeed - 10 >= 0) {
              tempSpoolSpeed -= 10;
            }

            char spoolSpeedBuffer[30] = "";

            sprintf(spoolSpeedBuffer, "Velocidad: %-5d", tempSpoolSpeed);
            lcdMenu.println(spoolSpeedBuffer, 0);

            spoolSpeed = tempSpoolSpeed;

            setSpoolSpeed(spoolSpeed);
            espnow.send();
          } else if (!isInSubmenu) {
            lcdMenu.initMenu(setSpeed);
            selectedOption = setSpeed;
          }
        }

        lastRotaryEncoderRead = rotaryRead;
      }

      if (rotaryEncoder.clicked()) {
        if (selectedOption == toggleSpool) {
          spool = !spool;
          lcdMenu.initMenu(toggleSpool);
        }

        if (selectedOption == viewInfo) {
          if (isResume) {
            lcdMenu.clear();
            lcdMenu.initMenu(toggleSpool);

            menuPosition = 0;
            selectedOption = toggleSpool;
            isResume = false;
            isInSubmenu = false;
          } else {
            lcdMenu.println("Revs: " + (String)totalRevs, 0, true);
            lcdMenu.println("Speed: " + (String)spoolSpeed, 1);
            lcdMenu.println("Tensioner: " + (String)actualDistance, 2);

            isResume = true;
            isInSubmenu = true;
          }
        }

        if (selectedOption == setSpeed) {
          if (isSelectingSpoolSpeed) {
            spoolSpeed = tempSpoolSpeed;

            menuPosition = 0;
            selectedOption = toggleSpool;

            lcdMenu.clear();
            lcdMenu.initMenu(toggleSpool);

            isSelectingSpoolSpeed = false;
            tempSpoolSpeed = -1;
            isInSubmenu = false;
          } else {
            lcdMenu.clear();
            lcdMenu.println("Velocidad: " + (String)spoolSpeed, 0);

            isSelectingSpoolSpeed = true;
            isInSubmenu = true;
          }
        }
      }
    }

    if (firstMove && !homed) {
      alignerMotor.setSpeed(1500);
      homed = goToHome();

      if (homed) {
        lcdMenu.println("Elige el comienzo de", 0, true);
        lcdMenu.println("la bobina", 2);
      }
    }

    if (totalRevs != lastTotalRevs) {
      lastTotalRevs = totalRevs;
      triggerMoving = true;
    }

    if (homed && isEndPosSet && readDistance.TRIGGERED) {
      actualDistance = tensioner.getDistance();
    }

    // Now we need start spool position
    if (homed && (!isStartPosSet && !isEndPosSet)) {
      if (rotaryEncoder.changed()) {
        onRotaryEncoderChange(lastRotaryEncoderRead);
      }

      if (rotaryEncoder.clicked()) {
        alignerMotor.setCurrentPosition(0);
        isStartPosSet = true;

        lcdMenu.println("Elige el final de la", 0, true);
        lcdMenu.println("bobina", 2);
      }
    }

    // Now we need end spool position
    if (homed && (isStartPosSet && !isEndPosSet)) {
      if (rotaryEncoder.changed()) {
        onRotaryEncoderChange(lastRotaryEncoderRead);
      }

      if (rotaryEncoder.clicked()) {
        spoolEndPos = alignerMotor.currentPosition();
        isEndPosSet = true;

        lcdMenu.println("Iniciando proceso",1, true);

        alignerMotor.setSpeed(1500);
        alignerMotor.moveTo(0);
        alignerMotor.runToPosition();

        delay(500);

        canLCD = true;
        lcdMenu.clear();
        lcdMenu.initMenu();
      }
    }

    if (homed && isEndPosSet && triggerMoving) {
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