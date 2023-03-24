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
    "Resumen            ", "Velocidad puller  ", "Posicionar         ",
    "Ajustar            ", "Configuracion     ", "Reset contadores   "};

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
  LCDMenu::configSubMenuOption = 0;
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
  lcd.print(" " + (String)measuring.minRead + "/" + (String)measuring.lastRead +
            "/" + (String)measuring.maxRead + "  ");
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
    if (option == configOption) menuOverflow = 1;
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

    if (strOpt.startsWith("Velocidad puller") ||
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

  if (measuring.mode == measuringAutoMode) {
    lcd.setCursor(1, 1);
    lcd.print("No disponible para");
    lcd.setCursor(2, 2);
    lcd.print("modo automatico.");

    return;
  }

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

void LCDMenu::configSubMenu() {
  lcd.clear();

  lcd.setCursor(0, this->configSubMenuOption);
  lcd.write(byte(0));

  lcd.setCursor(1, 0);
  lcd.print("Volver al menu");
  lcd.setCursor(19, 0);
  lcd.write(byte(6));

  lcd.setCursor(1, 1);
  lcd.print("Modo: " + measuring.measuringModeString());

  if (measuring.mode == measuringAutoMode) {
    lcd.setCursor(1, 2);
    String pidStabilizedString = pidPuller.stabilized ? "Si" : "No";
    lcd.print("Estabilizado: " + pidStabilizedString);

    lcd.setCursor(1, 3);
    lcd.print("Diametro: " + (String)filamentDiameter);
  }
}

bool LCDMenu::inConfigSubMenuOptions() {
  return this->inSubMenu && this->menuPosition == configOption &&
         this->configSubMenuOption != returnConfigOption;
}

void IRAM_ATTR LCDMenu::onREncoderChange(REncoder rEncoder) {
  // Al establecer el valor de inicio del rotatory encoder se lanza el evento
  // "change" y se necesita ignorarlo la primera vez
  if (!firstChangeIgnored) {
    firstChangeIgnored = true;
    return;
  }

  if (this->inSummary) return;

  if (this->inConfigSubMenuOptions() &&
      this->configSubMenuOptionSelected == targetDiameterOption) {
    if (rEncoder.direction == increased) {
      filamentDiameter += 0.01f;
    } else {
      filamentDiameter -= 0.01f;
    }

    // Guardar el último diametro de filamento seleccionado
    pref.putFloat(FILAMENT_DIAMETER_MODE_PREF, filamentDiameter);
    pidPuller.updateSetPoint(filamentDiameter);

    lcd.setCursor(11, 3);
    lcd.print(filamentDiameter);

    return;
  }

  if (this->inSubMenu) {
    uint16_t actualPullerSpeed = pullerSpeed;

    switch (this->menuPosition) {
      case pullerSpeedOption:
        if (measuring.mode == measuringAutoMode) break;

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

        char pullerSpeedBuffer[LCD_BUFFER];
        sprintf(pullerSpeedBuffer, "Velocidad: %-5d", pullerSpeed);

        lcd.setCursor(2, 0);
        lcd.print(pullerSpeedBuffer);
        break;
      case configOption:
        uint8_t omitOptions = 0;

        if (measuring.mode == measuringManualMode) {
          omitOptions = 2;
        }

        for (uint8_t i = 0; i < MENU_CONFIG_OPTIONS_NUMBER - omitOptions; i++) {
          lcd.setCursor(0, i);
          lcd.print(" ");
        }

        if (rEncoder.direction == increased) {
          if (this->configSubMenuOption <
              MENU_CONFIG_OPTIONS_NUMBER - omitOptions - 1) {
            this->configSubMenuOption++;
          }
        } else {
          if (this->configSubMenuOption > 0) {
            this->configSubMenuOption--;
          }
        }

        lcd.setCursor(0, this->configSubMenuOption);
        lcd.write(byte(0));
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

  if (this->configSubMenuOption == statusMeasuringOption) {
    uint8_t measuringMode = measuring.mode;

    if (measuring.mode == measuringAutoMode) {
      measuringMode = measuringManualMode;
      pullerSpeed = DEFAULT_PULLER_SPEED;
    }

    if (measuring.mode == measuringManualMode)
      measuringMode = measuringAutoMode;

    measuring.mode = (MeasuringMode)measuringMode;
    pref.putUInt(MEASURING_MODE_PREF, measuring.mode);

    this->configSubMenu();

    return;
  }

  if (this->configSubMenuOption == stabilizedPIDOption) {
    pidPuller.stabilized = !pidPuller.stabilized;

    lcd.setCursor(15, 2);
    lcd.print(pidPuller.stabilized ? "Si" : "No");

    return;
  }

  if (this->configSubMenuOption == targetDiameterOption) {
    lcd.setCursor(0, 3);

    if (this->configSubMenuOptionSelected != targetDiameterOption) {
      this->configSubMenuOptionSelected = targetDiameterOption;
      lcd.write(byte(4));
    } else {
      this->configSubMenuOptionSelected = returnConfigOption;
      lcd.write(byte(0));
    }

    return;
  }

  if (this->inSubMenu) {
    this->configSubMenuOption = 0;
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
      case resetCountersOption:
        pullerTotalRevs = 0;
        resetSpoolerRevs();
        measuring.reset();
        this->initSummary(true);
        break;
      case configOption:
        this->inSubMenu = true;
        this->configSubMenu();
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