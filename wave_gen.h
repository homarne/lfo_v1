
//#define maxSampleCount 8192-1
//#define halfSampleCount (8192/2)-1

#define DAC_FS 0x0000FFFF
// With a max WAVE_RATE of 4096, MAX_WAVE_COUNT sets maximum output frequency to 
// SAMPLE_RATE * (WAVE_RATE/MAX_WAVE_COUNT) = 500Hz 
#define MAX_WAVE_COUNT 0x10000


uint8_t selected_wave = 0;
#define WAVE_TYPE 0
#define WAVE_RATE 1
#define WAVE_LEVEL 2
#define WAVE_OFFSET 3
#define WAVE_FOLD 4
#define WAVE_ONESHOT 5
#define WAVE_SIGN 6
#define WAVE_PROGRESS 7

uint16_t waves[8][8] = 
{
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0}
};

uint16_t waves_values[8] =
{
  0,0,0,0,0,0,0,0
};


#define TYPE_SQUARE 0
#define TYPE_SAW 1
#define TYPE_TRIANGLE 2
#define TYPE_SINE 3

// This is defined here so drawing.h can use it, lol.
unsigned char knobsEnabled[] = {0, 0, 0, 0, 0};

// Sin lookup table
// This lookup table could be reduced in size by half to store only one fourth of the sine wave
// by changing sinGen() to flip the lookup at MAX_WAVE_COUNT*(1/4) and MAX_WAVE_COUNT*(3/4)
// to index downward, similar to how triGen() does it.
int16_t sin_table[MAX_WAVE_COUNT/2];

// generate one quarter sine wave centered at half full scale
void generateSineTable() {
  for (uint16_t i = 0; i < MAX_WAVE_COUNT/2; i++) {
    sin_table[i] = sin(((float)i / (float)DAC_FS) * 2 * M_PI) * DAC_FS/2 + DAC_FS/2;
  }
}

// Limits the rate of change between last_value and returned value
// Can be used for square and ramp waves to limit the rate of change 
// when transitioning between FS and 0, or 0 and FS
int rateLimit(int wave_value, int last_wave_value) {
  int return_value = wave_value;
  if (wave_value < last_wave_value) {
    if ((last_wave_value - wave_value) > DAC_FS/8) {
      return_value = last_wave_value - DAC_FS/32;
    }
  }
  if (wave_value > last_wave_value) {
    if ((wave_value - last_wave_value) > DAC_FS/8) {
      return_value = last_wave_value + DAC_FS/32;
    }
  }
  return return_value;
}

// Outputs a number in a sin pattern adjusted to the maximum sample count and amplitude
int sinGen(int num, int last_wave_value) {
  /* This code kept for legacy reasons.
  float input = ((float)num/(float)maxSampleCount)*2*M_PI;
  return sin(input) * halfAmplitude + halfAmplitude;
  */

  if (num <= MAX_WAVE_COUNT/2) {
    return sin_table[num];
    //return 0;
  } else {
    num = num - MAX_WAVE_COUNT/2;
    return DAC_FS - sin_table[num];
  }
}

// Outputs a number in a sawtooth pattern
int sawGen(int num, int last_wave_value) {
  return num;
}

// Outputs a DC level 
int levelGen(int wave_progress, int last_wave_value) {
  return wave_progress *16;
}

// Outputs a number in a square wave pattern
int squGen(int wave_progress, int last_wave_value) {
//  if (num > MAX_WAVE_COUNT/2) { return DAC_FS; } else { return 0; }

  uint16_t wave_value;
  // calculate what the value should be
  if (wave_progress > MAX_WAVE_COUNT/2) { 
    wave_value = DAC_FS; 
  } 
  else { 
      wave_value =  0; 
  }

  //wave_value = rateLimit(wave_value, last_wave_value);

  return wave_value;
}

// Outputs a number in a trangular wave pattern.
int triGen(int num, int last_wave_value) {
  if (num > MAX_WAVE_COUNT/2) {
    num = MAX_WAVE_COUNT - num;
  }
  // tri wave flips at half full scale, so mutiply by 2
  return num * 2;
}


// Gets the output value of a given wave number.
int getWaveCalculations(uint8_t wave_num) {
  // need a uint32 so the roll-over is not truncated
  uint32_t wave_progress = waves[wave_num][WAVE_PROGRESS] + waves[wave_num][WAVE_RATE]/8;
  //uint32_t wave_progress = waves[wave_num][WAVE_PROGRESS] + waves[wave_num][WAVE_RATE];

  if (wave_progress >= MAX_WAVE_COUNT) {
    waves[wave_num][WAVE_PROGRESS] = wave_progress - MAX_WAVE_COUNT;
  }
  else {
      waves[wave_num][WAVE_PROGRESS] = wave_progress;
  }
  
//  switch (waves[wave_num][WAVE_TYPE] / 1024) {
//    case TYPE_SQUARE:
//      return squGen(waves[wave_num][WAVE_PROGRESS]);
//    case TYPE_SAW:
//      return sawGen(waves[wave_num][WAVE_PROGRESS]);
//    case TYPE_TRIANGLE:
//      return triGen(waves[wave_num][WAVE_PROGRESS]);;
//    case TYPE_SINE:
//      return sinGen(waves[wave_num][WAVE_PROGRESS]);

  switch (waves[wave_num][WAVE_TYPE] / 1024) {
    case TYPE_SQUARE:
      waves_values[wave_num] = squGen(waves[wave_num][WAVE_PROGRESS], waves_values[wave_num]);
      break;
    case TYPE_SAW:
      waves_values[wave_num] = sawGen(waves[wave_num][WAVE_PROGRESS], waves_values[wave_num]);
      break;
    case TYPE_TRIANGLE:
      waves_values[wave_num] = triGen(waves[wave_num][WAVE_PROGRESS], waves_values[wave_num]);
      break;
    case TYPE_SINE:
      //waves_values[wave_num] = sinGen(waves[wave_num][WAVE_PROGRESS], waves_values[wave_num]);
      waves_values[wave_num] = levelGen(waves[wave_num][WAVE_RATE], waves_values[wave_num]);
  }
      
  return  waves_values[wave_num];


  return 0;
}
