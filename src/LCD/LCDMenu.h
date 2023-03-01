#pragma once

#include <Arduino.h>

enum MenuOption
{
    toggleSpool,
    viewInfo,
    setSpeed
};

#define MENU_OPTIONS_NUMBER 2

class LCDMenu
{
public:
    void init();

    void initMenu(MenuOption option = toggleSpool);

    void println(String text, uint8_t row, bool clear = false);

    void clear();
};