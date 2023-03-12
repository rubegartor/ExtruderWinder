#pragma once

#include <Arduino.h>
#include <RotaryEncoder/RotaryEncoder.h>

#define LCD_BTN_PIN 34

#define MENU_OPTIONS_NUMBER 6
#define MENU_MAX_OPTIONS_SHOWED 4
#define LCD_ADRRESS 0x27
#define LCD_BUFFER 20

enum MenuOption {
  summaryOption,
  homeAlignerOption,
  pullerSpeedOption,
  togglePullerOption,
  measuringOption,
  resetCountersOption
};

enum MeasuringSubMenuOption {
  returnOption,
  maximumOption,
  minimumOption,
  stateOption
};

class LCDMenu {
 private:
  MenuOption selectedOption;
  MeasuringSubMenuOption selectedMeasuringOption;

  bool inSubMenu;
  uint8_t speedOption;
  uint8_t measuringSubMenuOption;

  bool inMeasuringSubMenuOptions();

  void initMenu(MenuOption option = summaryOption, bool clear = false);

  void measuringSubMenu();

  void pullerSpeedSubMenu();

  void setSpeedButtonUnderscore();

  MenuOption nextItem();

  MenuOption prevItem();

 public:
  uint8_t menuPosition;

  bool inSummary;

  void init();

  void initSummary(bool clear = false);

  void println(String text, uint8_t row, bool clear = false);

  void IRAM_ATTR onREncoderChange(REncoder rEncoder);

  void onREncoderClick(REncoder rEncoder);

  void checkLCDButtons(bool init = false);
};