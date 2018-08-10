#define maxSampleCount 8192-1
#define halfSampleCount (8192/2)-1
#define oneHzSample 1000000/maxSampleCount  // sample for the 1Hz signal expressed in microseconds 
//#define maxAmplitude 4096
int maxAmplitude = 4096-1;
int halfAmplitude = (4096/2)-1;

uint8_t selected_wave = 0;
#define WAVE_TYPE 0
#define WAVE_RATE 1
#define WAVE_LEVEL 2
#define WAVE_OFFSET 3
#define WAVE_FOLD 4
#define WAVE_ONESHOT 5
#define WAVE_SIGN 6
#define WAVE_PROGRESS 7
uint16_t waves[][8] = 
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

#define TYPE_SQUARE 0
#define TYPE_SAW 1
#define TYPE_TRIANGLE 2
#define TYPE_SINE 3

// This is defined here so drawing.h can use it, lol.
unsigned char knobsEnabled[] = {0, 0, 0, 0, 0};

// Sin lookup table
int16_t sin_table[halfSampleCount];

void generateSineTable() {
  for (uint16_t i = 0; i < halfSampleCount; i++) {
    sin_table[i] = sin(((float)i / (float)maxSampleCount) * 2 * M_PI) * halfAmplitude + halfAmplitude;
  }
}

// Outputs a number in a sin pattern adjusted to the maximum sample count and amplitude
int sinGen(int num) {
  /* This code kept for legacy reasons.
  float input = ((float)num/(float)maxSampleCount)*2*M_PI;
  return sin(input) * halfAmplitude + halfAmplitude;
  */

  if (num <= halfSampleCount) {
    return sin_table[num];
    //return 0;
  } else {
    num -= halfSampleCount;
    return halfAmplitude - sin_table[num] + halfAmplitude;
  }
}

// Outputs a number in a sawtooth pattern
int sawGen(int num) {
  //return num * (8192 / maxSampleCount);
  return num;
}

#define    DAC_FS 0x0000FFFF
// Outputs a number in a square wave pattern
int squGen(int num) {
  if (num > halfSampleCount) { return DAC_FS; } else { return 0; }
}
//// Outputs a number in a square wave pattern
//int squGen(int num) {
//  if (num > halfSampleCount) { return maxAmplitude; } else { return 0; }
//}

// Outputs a number in a trangular wave pattern.
int triGen(int num) {
  if (num > halfSampleCount) {
    num = halfSampleCount - num;
  }
  //return num * (8192 / maxSampleCount);
  return num;
}


// Gets the output value of a given wave number.
int getWaveCalculations(uint8_t wave_num) {
  waves[wave_num][WAVE_PROGRESS] += waves[wave_num][WAVE_RATE];

  if (waves[wave_num][WAVE_PROGRESS] >= maxSampleCount) {
    waves[wave_num][WAVE_PROGRESS] -= maxSampleCount;
  }
  
  switch (waves[wave_num][WAVE_TYPE] / 1024) {
    case TYPE_SQUARE:
      return squGen(waves[wave_num][WAVE_PROGRESS]);
    case TYPE_SAW:
      return sawGen(waves[wave_num][WAVE_PROGRESS]);
    case TYPE_TRIANGLE:
      return triGen(waves[wave_num][WAVE_PROGRESS]);;
    case TYPE_SINE:
      return sinGen(waves[wave_num][WAVE_PROGRESS]);
  }
  return 0;
}
