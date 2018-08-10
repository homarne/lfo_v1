/*********************************************************************
This is an example for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

This example is for a 128x64 size display using I2C to communicate
3 pins are required to interface (2 I2C and one reset)

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
#include <Adafruit_SH1106.h>
#include "math.h"
#include <ltc2656_fast_spi.h>

// Relevant Headers
#include "icons.h"
#include "drawing.h"

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };


#if (SH1106_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SH1106.h!");
#endif

int analogInputA0 = A0;
int varA0;
int analogInputA1 = A1;
int varA1;
int analogInputA2 = A2;
int varA2;
int analogInputA3 = A3;
int varA3;
int analogInputA4 = A4;
int varA4;

#define BUTTON_UP 0
#define BUTTON_DOWN 1
#define BUTTON_MODE 2
#define BUTTON_PLUSMINUS 3
#define BUTTON_ONESHOT 4
unsigned char button_states[] = {0, 0, 0, 0, 0};

uint16_t last_knob_read[] = {0, 0, 0, 0, 0};
uint16_t knob_switched_states[] = {0, 0, 0, 0, 0};

const int button0 = 2, button1 = 3;
volatile int wave0 = 0, wave1 = 0;

signed short sweepDir = 1;
signed short triWaveDir = 1;

// variables for the interrupt wave calulation
uint32_t ulOutput_0 = 0;
uint32_t ulOutput_1 = 0;

// ltc2656 support
LTC2656FastSPI FASTDAC;
dac_data dac_data_8ch[8];
//const int dacSelectPin = 31;

uint32_t out_value = 0;
#define    DAC_FS 0x0000FFFF
#define STEP_SIZE 0x00000800

// -------------------------------------------

void setup()   {                
  Serial.begin(9600);
  
  
  //pinMode(dacSelectPin, OUTPUT);

  SPI.begin();
  SPI.beginTransaction(SPISettings(50000000, MSBFIRST, SPI_MODE0));
  FASTDAC.Begin();
  

  // Generate sin table
  generateSineTable();

  // timer and timer interrupt configuration
  
  /* turn on the timer clock in the power management controller */
  pmc_set_writeprotect(false);
  pmc_enable_periph_clk(ID_TC4);

  /* we want wavesel 01 with RC */
  TC_Configure(/* clock */TC1,/* channel */1, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK2);
  //TC_SetRC(TC1, 1, 238); // sets <> 44.1 Khz interrupt rate
  TC_SetRC(TC1, 1, 6*220); // sets <> 8.0 (measured) Khz interrupt rate
  TC_Start(TC1, 1);
  
  // enable timer interrupts on the timer 
  TC1->TC_CHANNEL[1].TC_IER=TC_IER_CPCS;
  TC1->TC_CHANNEL[1].TC_IDR=~TC_IER_CPCS;
  
  /* Enable the interrupt in the nested vector interrupt controller */
  /* TC4_IRQn where 4 is the timer number * timer channels (3) + the channel number (=(1*3)+1) for timer1 channel1 */
  NVIC_EnableIRQ(TC4_IRQn);

  // this is a cheat - enable the DAC
  analogWrite(DAC0,0);
  analogWrite(DAC1,0);
  
  // ---------------------------------------
  // configure pins for button inputs
  pinMode(53, INPUT_PULLUP);
  pinMode(51, INPUT_PULLUP);
  pinMode(49, INPUT_PULLUP);
  pinMode(47, INPUT_PULLUP);
  pinMode(45, INPUT_PULLUP);
  // ---------------------------------------

  analogReadResolution(12);  //set ADC resolution to 12 bits
  analogWriteResolution(12);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)

}

void draw() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  drawStaticContent();
  drawWaveData(waves[selected_wave]);
  display.display();
}

void loop() {

  // Controls how fast numbers blink when locked.
  blink_timer += 8;
  
  // read potentiometers
  uint16_t vals[5] = {0,0,0,0,0};
  vals[WAVE_FOLD]   = analogRead(analogInputA0);  //read ADC input
  vals[WAVE_OFFSET] = analogRead(analogInputA1);  //read ADC input
  vals[WAVE_LEVEL]  = analogRead(analogInputA2);  //read ADC input
  vals[WAVE_RATE]   = analogRead(analogInputA3);  //read ADC input
  vals[WAVE_TYPE]   = analogRead(analogInputA4);  //read ADC input

  //smooth potentiomer readings

  for (uint8_t i = 0; i < 5; i++) {
    int difference = waves[selected_wave][i] - vals[i];
    signed int theValue = waves[selected_wave][i];
        
    // If the given knob is not enabled
    if (knobsEnabled[i] == 0) { 
      bool valuePassed = false;

      int knob_turn_dist = abs(last_knob_read[i] - vals[i]);

      if (theValue < knob_switched_states[i]) { // The knob started on the right side
        if (vals[i] - knob_turn_dist < theValue) {
          valuePassed = true;
        }
      } else { // The knob started on the left side
        if (vals[i] + knob_turn_dist > theValue) {
          valuePassed = true;
        }
      }
      
      if (valuePassed) { // Detects when the value goes near the already set value
        knobsEnabled[i] = 1;
      } else {
        last_knob_read[i] = vals[i];
        continue; 
      }
    }

    last_knob_read[i] = vals[i];
    
    // simple smoothing filter
    // larger diviser gives more smoothing, longer settling time
    // smaller diviser gives less smoothing, faster settling time
    int scaled_difference = difference / 10;
    waves[selected_wave][i]= waves[selected_wave][i] - scaled_difference;
  }

  //waves[selected_wave][WAVE_TYPE] /= 1024; // Difference smoothing will not allow this to work.

  // read buttons
  unsigned char button_vals[5] = {0,0,0,0,0};
  button_vals[0] = digitalRead(53);
  button_vals[1] = digitalRead(51);
  button_vals[2] = digitalRead(49);
  button_vals[3] = digitalRead(47);
  button_vals[4] = digitalRead(45);

  // Act on button presses
  for (unsigned char i = 0; i < 5; i++) {
    if (button_states[i] != button_vals[i]) {
      if (button_vals[i] == 0) { // Detects on button up
        onButtonPressed(i);
      }
    }
    button_states[i] = button_vals[i];
  }
  
  // update display
  draw();
}

// Called when a button is pressed
void onButtonPressed(char button_num) {
  switch (button_num) {
    case BUTTON_UP:
      switchWaves(selected_wave + 1);
      break;
    case BUTTON_DOWN:
      switchWaves(selected_wave - 1);
      break;
    case BUTTON_MODE:

      break;
    case BUTTON_PLUSMINUS:
      if (waves[selected_wave][WAVE_SIGN] == 0) waves[selected_wave][WAVE_SIGN] = 1;
      else waves[selected_wave][WAVE_SIGN] = 0;
      break;
    case BUTTON_ONESHOT:
      if (waves[selected_wave][WAVE_ONESHOT] == 0) waves[selected_wave][WAVE_ONESHOT] = 1;
      else waves[selected_wave][WAVE_ONESHOT] = 0;
      break;
  }
}

// Locks all knobs so values aren't updated from them, and update their read states
void lockKnobs() {
  
  // Read current knob positions
  uint16_t vals[5];
  vals[WAVE_FOLD]   = analogRead(analogInputA0);
  vals[WAVE_OFFSET] = analogRead(analogInputA1);
  vals[WAVE_LEVEL]  = analogRead(analogInputA2);
  vals[WAVE_RATE]   = analogRead(analogInputA3);
  vals[WAVE_TYPE]   = analogRead(analogInputA4);
  
  for (char i = 0; i < 5; i++) {
    knobsEnabled[i] = 0;
    knob_switched_states[i] = vals[i]; // Set the current knob positions on switch
  }
}

// Switches the active wave to the given wave #
void switchWaves(signed char toWave) {
  lockKnobs();
  
  if (toWave < 0) { toWave = 7; }
  if (toWave > 7) { toWave = 0; }
  selected_wave = toWave;
}


void update_dac_data() {
  // outputs raw ltc2656 data array
  // set to 1 to ramp DAC data between zero and DAC_FS
  // STEP_SIZE sets ramp rate - (DAC_FS+1)/STEP_SIZE = number of steps in ramp
  out_value = out_value + STEP_SIZE;
  if (out_value >= DAC_FS+1){
    out_value = out_value - (DAC_FS+1);
  } 

  // set DAC A data to out_value
  dac_data_8ch[DAC_A].dac_word = out_value;

  // set all DACs to the same value
  dac_data_8ch[DAC_B] = 
  dac_data_8ch[DAC_C] = 
  dac_data_8ch[DAC_D] = 
  dac_data_8ch[DAC_E] = 
  dac_data_8ch[DAC_F] = 
  dac_data_8ch[DAC_G] = 
  dac_data_8ch[DAC_H] = dac_data_8ch[0];

  return;
}


void TC4_Handler()
{
  // We need to get the status to clear it and allow the interrupt to fire again
  TC_GetStatus(TC1, 1);

  // write the dac output immediatly so subsequent computations dont skew sample timing
  // use dacc_write_conversion_data() for fast execution
  dacc_set_channel_selection(DACC_INTERFACE, 0);
  dacc_write_conversion_data(DACC_INTERFACE, ulOutput_0);
  dacc_set_channel_selection(DACC_INTERFACE, 1);
  dacc_write_conversion_data(DACC_INTERFACE, ulOutput_1);

  FASTDAC.WriteDac8(dac_data_8ch);


  // Write the wave calculations to out
  ulOutput_0 = getWaveCalculations(0);
  ulOutput_1 = getWaveCalculations(1); 

  update_dac_data();

//  dac_data_8ch[0].dac_word = getWaveCalculations(0);
//  dac_data_8ch[1].dac_word = getWaveCalculations(1); 
//  dac_data_8ch[2].dac_word = 0;
//  dac_data_8ch[3].dac_word = 0; 
//  dac_data_8ch[4].dac_word = 0;
//  dac_data_8ch[5].dac_word = 0; 
//  dac_data_8ch[6].dac_word = 0;
//  dac_data_8ch[7].dac_word = 0; 


}
