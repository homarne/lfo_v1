#include "wave_gen.h"

// This bit defines 'display'.
#define OLED_RESET 4
Adafruit_SH1106 display(OLED_RESET);

#define UI_TEXT_VSEP 16 // Vertical separation on the shown data.

uint8_t blink_timer = 0;

// Prints a number right-justified
void printRightJustified(uint16_t num, uint8_t x, uint8_t y) {
  uint16_t place = 0;
  uint16_t toPrint = num;
  for (uint16_t p = 0; p < 4; p++) {
    num /= 10;
    place++;
    if (num <= 0) {
      break;
    }
  }

  display.setCursor(x - (place * 12), y);
  display.print(toPrint);
}

// Draws a "special character" defined in icons.h.
void drawSpecialChar(const unsigned char *c, uint8_t x, uint8_t y) {
  for (uint8_t p = 0; p < 35; p++) {
    if (p!= 0 && p % 5 == 0) {
      x -= 5;
      y++;
    }
    
    if (c[p] == 1) {
      display.drawPixel(x, y, WHITE);
    }
    x++;
  }
}

// Draws an icon defined in icons.h.
void drawIcon(const unsigned char *c, uint8_t x, uint8_t y, uint8_t sz, uint16_t width) {
  for (uint16_t p = 0; p < pow(width, 2); p++) {
    if (p!= 0 && p % width == 0) {
      x -= width * sz;
      y += sz;
    }
    
    if (c[p] == 1) {
      for (uint8_t xx = 0; xx < sz; xx++) {
        for (uint8_t yy = 0; yy < sz; yy++) {
          display.drawPixel(x + xx, y + yy, WHITE);
        }
      }
    }
    
    x += sz;
  }
}

// Draws the static (unchanging) content to the display.
void drawStaticContent() {
  // Left boxes
  display.drawRect(0, 0, 23, 41, WHITE);

  // Seperation in boxes
  display.drawLine(0, 24, 22, 24, WHITE);

  // Static Text
  display.setTextSize(2);

  uint8_t y_pos = 1;
  display.setCursor(26, y_pos);
  display.println("RATE");

  y_pos += UI_TEXT_VSEP;

  display.setCursor(26, y_pos);
  display.println("LEVL");

  y_pos += UI_TEXT_VSEP;

  display.setCursor(26, y_pos);
  display.println("OFST");

  y_pos += UI_TEXT_VSEP;

  display.setCursor(26, y_pos);
  display.println("FOLD");

  // Checkbox Tags
  drawIcon(icon_oneshot, 1, 42, 2, 7);
  drawIcon(icon_plusminus, 1, 51, 2, 7);
}

// Draws wave data for an array of wave data.
void drawWaveData(uint16_t *wave) {
  display.setTextSize(2);
  uint8_t y_pos = 1;

  for (uint8_t i = 0; i < 4; i++) {
    if (knobsEnabled[i + 1] || (!knobsEnabled[i + 1] && blink_timer > 127)){
      printRightJustified(wave[i + 1], 127, i * UI_TEXT_VSEP + 1);
    }
  }

  drawIcon(icon_bool[wave[WAVE_ONESHOT]], 12, 39, 2, 7);
  drawIcon(icon_bool[wave[WAVE_SIGN]], 12, 51, 2, 7);

  display.setCursor(4, 2);
  display.setTextSize(3);
  display.println(selected_wave + 1);

  if (knobsEnabled[0] || (!knobsEnabled[0] && blink_timer > 127)) {
    switch (wave[WAVE_TYPE] / 1024) {
      case TYPE_SQUARE:
        drawIcon(icon_square, 1, 22, 3, 7);
        break;
      case TYPE_SAW:
        drawIcon(icon_saw, 1, 22, 3, 7);
        break;
      case TYPE_TRIANGLE:
        drawIcon(icon_triangle, 1, 22, 3, 7);
        break;
      case TYPE_SINE:
        drawIcon(icon_sine, 1, 23, 2, 11);
        break;
    }
  }
}
