#include <Aligner/Aligner.h>
#include <Commons/Commons.h>
#include <LCD/LCDMenu.h>
#include <LiquidCrystal_I2C.h>
#include <RotaryEncoder/RotaryEncoder.h>
#include <Winder/Winder.h>
#include <math.h>

LiquidCrystal_I2C lcd(LCD_ADRRESS, LCD_BUFFER, MENU_MAX_OPTIONS_SHOWED);

byte caretCharacter[] = {B10000, B11000, B11100, B11110,
                         B11110, B11100, B11000, B10000};
byte unselectedCaretCharacter[] = {B10000, B11000, B01100, B00110,
                                   B00110, B01100, B11000, B10000};
byte checkCharacter[] = {B00000, B00001, B00011, B10110,
                         B11100, B01000, B00000, B00000};
byte underscoreCharacter[] = {B11111, B11111, B00000, B00000,
                              B00000, B00000, B00000, B00000};
byte spoolCharacter[] = {B00000, B00000, B10001, B11111,
                         B11111, B10001, B00000, B00000};
byte alertCharacter[] = {B01110, B01110, B01110, B01110,
                         B01110, B00000, B01110, B01110};
byte arrowCharacter[] = {B00000, B00100, B00110, B11111,
                         B11111, B00110, B00100, B00000};
byte backCharacter[] = {B00100, B01110, B11111, B00100,
                        B00100, B11100, B00000, B00000};

// Implementar una función para autocompletar con espacios el buffer
const char *optionsStr[MENU_OPTIONS_NUMBER] = {
    "Resumen            ", "Velocidad puller  ",  "Posicionar         ",
    "On/Off puller      ", "Alarmas            ", "Reset contadores   "};

const int speeds[4] = {5, 10, 100, 500};
const int speedsPositions[4] = {2, 5, 9, 14};  // LCD cursor positions

bool firstChangeIgnored;
bool showAlert = true;

void LCDMenu::init() {
  lcd.init();
  lcd.createChar(0, caretCharacter);
  lcd.createChar(1, checkCharacter);
  lcd.createChar(2, spoolCharacter);
  lcd.createChar(3, underscoreCharacter);
  lcd.createChar(4, unselectedCaretCharacter);
  lcd.createChar(5, alertCharacter);
  lcd.createChar(6, backCharacter);
  lcd.createChar(7, arrowCharacter);
  lcd.backlight();

  // Inicializar las variables para el menú
  LCDMenu::menuPosition = 0;
  LCDMenu::measuringSubMenuOption = 0;
  LCDMenu::speedOption = 0;
  LCDMenu::inSubMenu = false;
  LCDMenu::inSummary = true;
}

void LCDMenu::initSummary(bool clear) {
  if (clear) lcd.clear();

  this->inSummary = true;
  this->menuPosition = summaryOption;

  // ------------------------------------------------------------------

  char pullerSpeedBuffer[LCD_BUFFER];
  sprintf(pullerSpeedBuffer, "%-5d", pullerSpeed);

  lcd.setCursor(0, 0);
  lcd.print("S");
  lcd.setCursor(2, 0);
  lcd.print(pullerSpeedBuffer);

  lcd.setCursor(0, 3);

  if (isReady()) {
    lcd.write(byte(1));

    lcd.setCursor(2, 3);
    lcd.print("Todo listo");
  } else {
    if (showAlert) {
      lcd.print(" ");
    } else {
      lcd.write(byte(5));
    }

    showAlert = !showAlert;

    lcd.setCursor(2, 3);
    lcd.print("Tareas pendientes");
  }

  lcd.setCursor(11, 0);
  lcd.write(byte(2));

  char extrudedLengthBuffer[LCD_BUFFER];
  sprintf(extrudedLengthBuffer, " %.2fm", getExtrudedLength());

  lcd.print(extrudedLengthBuffer);

  lcd.setCursor(0, 1);
  lcd.print("D");
  lcd.setCursor(2, 1);
  lcd.print(" " + (String)calibration.minRead + "/" +
            (String)calibration.lastRead + "/" + (String)calibration.maxRead +
            "  ");
}

void LCDMenu::initMenu(MenuOption option, bool clear) {
  if (clear) lcd.clear();

  uint8_t row = (uint8_t)option;

  if (row >= MENU_MAX_OPTIONS_SHOWED) {
    row = MENU_MAX_OPTIONS_SHOWED - 1;
  }

  for (uint8_t i = 0; i < MENU_MAX_OPTIONS_SHOWED; i++) {
    lcd.setCursor(0, i);
    lcd.print(" ");
  }

  lcd.setCursor(0, row);
  lcd.write(byte(0));

  uint8_t maxOptions = MENU_MAX_OPTIONS_SHOWED;
  uint8_t menuOverflow = 0;

  if (option >= MENU_MAX_OPTIONS_SHOWED) {
    if (option == measuringOption) menuOverflow = 1;
    if (option == resetCountersOption) menuOverflow = 2;

    maxOptions = MENU_MAX_OPTIONS_SHOWED + menuOverflow;
  }

  // Imprimir los string correspondientes a las opciones del enum
  for (uint8_t j = menuOverflow; j < maxOptions; j++) {
    String strOpt = optionsStr[j];

    if (strOpt.startsWith("Posicionar")) {
      strOpt = "Posicionar [";
    }

    lcd.setCursor(1, j - menuOverflow);
    lcd.print(strOpt);

    if (strOpt.startsWith("Posicionar") && homed) {
      lcd.write(byte(1));
      lcd.print("]     ");
    } else if (strOpt.startsWith("Posicionar") && !homed) {
      lcd.write(byte(5));
      lcd.print("]     ");
    }

    if (strOpt.startsWith("Velocidad puller") || strOpt.startsWith("Alarmas") ||
        strOpt.startsWith("Posicionar")) {
      lcd.setCursor(19, j - menuOverflow);
      lcd.write(byte(7));
    }
  }
}

void LCDMenu::println(String text, uint8_t row, bool clear) {
  if (clear) lcd.clear();

  lcd.setCursor(0, row);
  lcd.print(text);
}

void LCDMenu::pullerSpeedSubMenu() {
  lcd.clear();

  for (uint8_t i = 0; i < sizeof(speeds) / sizeof(int); i++) {
    lcd.setCursor(speedsPositions[i], 2);
    lcd.print(speeds[i]);
  }

  char pullerSpeedBuffer[LCD_BUFFER];
  sprintf(pullerSpeedBuffer, "Velocidad: %-5d", pullerSpeed);

  lcd.setCursor(2, 0);
  lcd.print(pullerSpeedBuffer);

  this->checkLCDButtons(true);
}

void LCDMenu::measuringSubMenu() {
  lcd.clear();

  lcd.setCursor(0, returnOption);
  lcd.write(byte(0));

  lcd.setCursor(1, 0);
  lcd.print("Volver al menu");
  lcd.setCursor(19, 0);
  lcd.write(byte(6));

  lcd.setCursor(1, 1);
  lcd.print("Diam. max: " + (String)calibration.maxRange);

  lcd.setCursor(1, 2);
  lcd.print("Diam. min: " + (String)calibration.minRange);

  String calibrationState = calibration.state ? "Encendido" : "Apagado   ";
  lcd.setCursor(1, 3);
  lcd.print("Estado: " + calibrationState);
}

bool LCDMenu::inMeasuringSubMenuOptions() {
  return this->inSubMenu && this->menuPosition == measuringOption &&
         this->measuringSubMenuOption != 0 &&
         this->measuringSubMenuOption != stateOption;
}

void IRAM_ATTR LCDMenu::onREncoderChange(REncoder rEncoder) {
  // Al establecer el valor de inicio del rotatory encoder se lanza el evento
  // "change" y se necesita ignorarlo la primera vez
  if (!firstChangeIgnored) {
    firstChangeIgnored = true;
    return;
  }

  if (this->inSummary) return;

  if (this->inSubMenu && this->selectedMeasuringOption != returnOption) {
    if (rEncoder.direction == increased) {
      if (this->selectedMeasuringOption == minimumOption) {
        calibration.setMinRange(calibration.minRange + 0.01f);
      }

      if (this->selectedMeasuringOption == maximumOption) {
        calibration.setMaxRange(calibration.maxRange + 0.01f);
      }
    } else {
      if (this->selectedMeasuringOption == minimumOption) {
        calibration.setMinRange(calibration.minRange - 0.01f);
      }

      if (this->selectedMeasuringOption == maximumOption) {
        calibration.setMaxRange(calibration.maxRange - 0.01f);
      }
    }

    lcd.setCursor(12, this->selectedMeasuringOption);

    if (this->selectedMeasuringOption == minimumOption)
      lcd.print(calibration.minRange);
    if (this->selectedMeasuringOption == maximumOption)
      lcd.print(calibration.maxRange);

    return;
  }

  if (this->inSubMenu) {
    uint16_t actualPullerSpeed = pullerSpeed;

    switch (this->menuPosition) {
      case pullerSpeedOption:
        if (rEncoder.direction == increased) {
          if (actualPullerSpeed + speeds[this->speedOption] >
              PULLER_MAX_SPEED) {
            pullerSpeed = PULLER_MAX_SPEED;
          } else {
            pullerSpeed = actualPullerSpeed + speeds[this->speedOption];
          }
        } else {
          if (actualPullerSpeed - speeds[this->speedOption] < 0) {
            pullerSpeed = 0;
          } else {
            pullerSpeed = actualPullerSpeed - speeds[this->speedOption];
          }
        }

        spoolSpeed = (uint16_t)pullerSpeed * speedRatioMultiplier;

        char pullerSpeedBuffer[LCD_BUFFER];
        sprintf(pullerSpeedBuffer, "Velocidad: %-5d", pullerSpeed);

        lcd.setCursor(2, 0);
        lcd.print(pullerSpeedBuffer);
        break;
      case measuringOption:
        for (uint8_t i = 0; i < MENU_MAX_OPTIONS_SHOWED; i++) {
          lcd.setCursor(0, i);
          lcd.print(" ");
        }

        if (rEncoder.direction == increased) {
          if (this->measuringSubMenuOption < MENU_MAX_OPTIONS_SHOWED - 1) {
            this->measuringSubMenuOption++;
          }
        } else {
          if (this->measuringSubMenuOption > 0) {
            this->measuringSubMenuOption--;
          }
        }

        lcd.setCursor(0, this->measuringSubMenuOption);
        if (this->measuringSubMenuOption == stateOption ||
            this->measuringSubMenuOption == returnOption) {
          lcd.write(byte(0));
        } else {
          lcd.write(byte(4));
        }
        break;
    }

    return;
  }

  if (rEncoder.direction == increased) {
    MenuOption nextItem = this->nextItem();
    this->initMenu(nextItem);
  } else {
    MenuOption prevOption = this->prevItem();
    this->initMenu(prevOption);
  }
}

void LCDMenu::onREncoderClick(REncoder rEncoder) {
  if (this->inSummary) {
    this->inSummary = false;
    this->initMenu(summaryOption, true);

    return;
  }

  if (this->selectedMeasuringOption == minimumOption ||
      this->selectedMeasuringOption == maximumOption) {
    this->selectedMeasuringOption = (MeasuringSubMenuOption)0;

    for (uint8_t i = 0; i < MENU_MAX_OPTIONS_SHOWED; i++) {
      lcd.setCursor(0, i);
      lcd.print(" ");
    }

    lcd.setCursor(0, this->measuringSubMenuOption);
    lcd.write(byte(4));
    return;
  }

  if (this->measuringSubMenuOption == stateOption) {
    calibration.state = !calibration.state;

    lcd.setCursor(9, 3);
    lcd.print(calibration.state ? "Encendido" : "Apagado   ");

    return;
  }

  if (inMeasuringSubMenuOptions()) {
    this->selectedMeasuringOption =
        (MeasuringSubMenuOption)this->measuringSubMenuOption;

    lcd.setCursor(0, this->selectedMeasuringOption);
    lcd.write(byte(0));
    return;
  }

  if (this->inSubMenu) {
    this->measuringSubMenuOption = 0;
    this->selectedMeasuringOption = returnOption;
    // -----------------------------------------------------------
    this->inSubMenu = false;
    this->initMenu((MenuOption)this->menuPosition, true);

    return;
  }

  if (!this->inSummary) {
    String summaryMessage = "Posicionando ...";

    switch (this->menuPosition) {
      case summaryOption:
        this->initSummary(true);
        break;
      case homeAlignerOption:
        needHome = true;

        lcd.clear();
        lcd.setCursor((LCD_BUFFER - summaryMessage.length()) / 2, 1);
        lcd.print(summaryMessage);
        break;
      case pullerSpeedOption:
        this->inSubMenu = true;
        this->pullerSpeedSubMenu();
        break;
      case togglePullerOption:
        pullerState = !pullerState;
        lcd.setCursor(1, this->menuPosition);
        break;
      case resetCountersOption:
        pullerTotalRevs = 0;
        spoolTotalRevs = 0;
        calibration.reset();
        this->initSummary(true);
        break;
      case measuringOption:
        this->inSubMenu = true;
        this->measuringSubMenu();
        break;
    }
  }
}

void LCDMenu::setSpeedButtonUnderscore() {
  lcd.setCursor(0, 3);
  for (uint8_t k = 0; k < LCD_BUFFER; k++) {
    lcd.print(" ");
  }

  lcd.setCursor(speedsPositions[this->speedOption], 3);
  uint8_t numOfDigits = log10(speeds[this->speedOption]) + 1;
  for (uint8_t j = 0; j < numOfDigits; j++) {
    lcd.write(byte(3));
  }
}

void LCDMenu::checkLCDButtons(bool init) {
  if (this->inSubMenu && this->menuPosition == pullerSpeedOption) {
    uint16_t buttonArrayRead = analogRead(LCD_BTN_PIN);
    uint8_t btnIndex = this->speedOption;

    if (init) {
      this->setSpeedButtonUnderscore();
    }

    if (buttonArrayRead > 400) {
      if (buttonArrayRead < 500) {
        btnIndex = 0;
      } else if (buttonArrayRead > 1200 && buttonArrayRead < 1500) {
        btnIndex = 1;
      } else if (buttonArrayRead > 1500 && buttonArrayRead < 2200) {
        btnIndex = 2;
      } else if (buttonArrayRead > 2300) {
        btnIndex = 3;
      }

      if (btnIndex != this->speedOption) {
        this->speedOption = btnIndex;

        this->setSpeedButtonUnderscore();
      }
    }
  }
}

MenuOption LCDMenu::nextItem() {
  if (this->menuPosition >= MENU_OPTIONS_NUMBER - 1)
    return (MenuOption)this->menuPosition;

  this->menuPosition++;

  return (MenuOption)(this->menuPosition);
}

MenuOption LCDMenu::prevItem() {
  if (this->menuPosition <= 0) return (MenuOption)0;

  this->menuPosition--;

  return (MenuOption)(this->menuPosition);
}