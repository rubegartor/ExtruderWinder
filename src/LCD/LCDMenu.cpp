#include <LiquidCrystal_I2C.h>
#include <LCD/LCDMenu.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

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

void LCDMenu::init()
{
    lcd.init();
    lcd.createChar(0, arrow);
    lcd.backlight();
}

void LCDMenu::initMenu(MenuOption option)
{
    lcd.setCursor(0, 0);
    lcd.print(" ");
    lcd.setCursor(0, 1);
    lcd.print(" ");
    lcd.setCursor(0, 2);
    lcd.print(" ");

    if (option == toggleSpool)
    {
        lcd.setCursor(0, 0);
    }

    if (option == viewInfo)
    {
        lcd.setCursor(0, 1);
    }

    if (option == setSpeed)
    {
        lcd.setCursor(0, 2);
    }

    lcd.write(byte(0));

    lcd.setCursor(2, 0);
    lcd.print("On/Off");

    lcd.setCursor(2, 1);
    lcd.print("Resumen");

    lcd.setCursor(2, 2);
    lcd.print("Velocidad");
}

void LCDMenu::println(String text, uint8_t row, bool clear)
{
    if (clear)
        lcd.clear();

    lcd.setCursor(0, row);
    lcd.print(text);
}

void LCDMenu::clear()
{
    lcd.clear();
}