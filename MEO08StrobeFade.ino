#include <FastLED.h>

#define LED_PIN     5
#define COLOR_ORDER GRB
#define CHIPSET     WS2801
#define NUM_LEDS    100

#define BRIGHTNESS  100
#define FRAMES_PER_SECOND 100

CRGB leds[NUM_LEDS];

int randomTable[NUM_LEDS];
bool effectInitialised = false; //so only initialises first time
int settingEyePosition; //
int settingStartHue; // Colour on color wheel Red (0), Green (512) or Blue (1024)
int settingColorShiftAmount; //how much to shift the hue as it fades
int settingColorEffectType; // whether fixed colour, rainbow, or rainbow change color fade
int settingColorWheelType; // full rainbow (0) or one of the lines RG (1) GB (2) BR (3)
int settingColorShiftPosition; //

int variation; //the effect variation - 0 to 8

void setup() {
  delay(3000); // sanity delay
  //FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.addLeds<CHIPSET, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  // create non-repeating randomised table
  // Start off ordered, randomTable[0]=0, randomTable[1]=1... etc...
  for (int range = 0; range < NUM_LEDS; range++) {
    randomTable[range] = range;
  }
  // Random shuffle, so, randomTable[0]=38, randomTable[1]=13... etc...
  for (int shuffle = 0; shuffle < NUM_LEDS - 1; shuffle++) {
    int myrand = shuffle + random(NUM_LEDS - shuffle);
    int save = randomTable[shuffle];
    randomTable[shuffle] = randomTable[myrand];
    randomTable[myrand] = save;
  }
}

void loop() {
  Program08StrobeFade(8); //change this value to 0 to 8 to see all effect types

  FastLED.show(); // display this frame
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}

void Program08StrobeFade(int variation) {
  if (!effectInitialised) { // Initialize effect?
    settingEyePosition = 0; // eye position
    settingStartHue = random(3) * 512; // Colour on color wheel Red (0), Green (512) or Blue (1024)
    settingColorShiftAmount = 5;
    if (random(2) == 0)
      settingStartHue = -settingStartHue;
    if (random(2) == 0)
      settingColorShiftAmount = -settingColorShiftAmount;
    settingColorShiftPosition = 0; // Current position

    // fxInt1 = span of colour - increases, so 0 will become higher etc
    // - 0 is single color across string
    // - 1536 is all colours across string (or 1024 for RG/GB/BR)
    // - half and double also used
    //String varName = "";
    switch (variation) {
      case 0: // Fixed Colour
        settingColorEffectType = 0;
        settingColorWheelType = 0;
        //varName = "Fixed color";
        break;
      case 1: // Rainbow
        settingColorEffectType = 1;
        settingColorWheelType = 0;
        //varName = "Rainbow";
        break;
      case 2: // RG
        settingColorEffectType = 1;
        settingColorWheelType = 1;
        //varName = "Red to green";
        break;
      case 3: // GB
        settingColorEffectType = 1;
        settingColorWheelType = 2;
        //varName = "Green to blue";
        break;
      case 4: // BR
        settingColorEffectType = 1;
        settingColorWheelType = 3;
        //varName = "Blue to red";
        break;
      case 5: // Rainbow change
        settingColorEffectType = 2;
        settingColorWheelType = 0;
        //varName = "Rainbow fade";
      case 6: // RG change
        settingColorEffectType = 2;
        settingColorWheelType = 1;
        //varName = "Red to green fade";
        break;
      case 7: // GB change
        settingColorEffectType = 2;
        settingColorWheelType = 2;
        //varName = "Green to blue fade";
        break;
      case 8: // BR change
        settingColorEffectType = 2;
        settingColorWheelType = 3;
        //varName = "Blue to red fade";
        break;
    }
    effectInitialised = true; // end initialise
  }

  //The meat starts here...
  int offset;
  int color[3];

  for (int i = 0; i < NUM_LEDS; i++) {
    offset = (NUM_LEDS + settingEyePosition - randomTable[i]) % NUM_LEDS;

    if (settingColorEffectType == 0) { //fixed color
      HSVtoRGB(settingStartHue, 255, GetSmoothFade27(offset), 0, color);
    } else if (settingColorEffectType == 1) {// rainbow - the complex color term is due to needing to fade in the same colour - otherwise the fade carry on rotating through colours
      HSVtoRGB(((settingColorShiftPosition + settingStartHue * i / NUM_LEDS) - (offset * settingColorShiftAmount)) % 1536, 255,
              GetSmoothFade27(offset), settingColorWheelType, color);
    } else { // fade shifts colour
      HSVtoRGB(((settingColorShiftPosition + settingStartHue * i / NUM_LEDS) + (offset * settingColorShiftAmount) * 10) % 1536,
              255, GetSmoothFade27(offset), settingColorWheelType, color);
    }
    leds[i].r = color[0];
    leds[i].g = color[1];
    leds[i].b = color[2];
  }

  // increase/decrease eye position
  settingEyePosition++;

  // for rainbow version - move through rainbow
  settingColorShiftPosition += settingColorShiftAmount;
}

int fadeTable27[27] = {255, 223, 191, 159, 127, 111, 95, 79, 63, 55, 47, 39, 31, 27, 23, 19, 15, 13, 11, 9, 7, 6, 5, 4, 3, 2, 1};

int GetSmoothFade27(int x) {
  if ((x < 27) && (x >= 0)) {
    return fadeTable27[x];
  } else {
    return 0;
  }
}


//started off as an Adafruit function, but has evolved...
void HSVtoRGB(int h, int s, int v, int wheelLine, int color[3]) {
  int r, g, b, lo;
  int satMultiplier;
  float valueMultiplier;

  // Hue
  switch (wheelLine) {
    case 0: // Full RGB Wheel (pburgess original function)
      h %= 1536; // -1535 to +1535
      if (h < 0) h += 1536; //     0 to +1535
      lo = h & 255; // Low byte  = primary/secondary color mix
      switch (h >> 8) { // High byte = sextant of colorwheel
        case 0:
          r = 255;
          g = lo;
          b = 0;
          break; // R to Y
        case 1:
          r = 255 - lo;
          g = 255;
          b = 0;
          break; // Y to G
        case 2:
          r = 0;
          g = 255;
          b = lo;
          break; // G to C
        case 3:
          r = 0;
          g = 255 - lo;
          b = 255;
          break; // C to B
        case 4:
          r = lo;
          g = 0;
          b = 255;
          break; // B to M
        default:
          r = 255;
          g = 0;
          b = 255 - lo;
          break; // M to R
      }
      break;
    case 1: //RG Line only
      h %= 1024;
      if (h < 0) h += 1024;
      lo = h & 255;
      switch (h >> 8) {
        case 0:
          r = 255;
          g = lo;
          b = 0;
          break; // R to Y
        case 1:
          r = 255 - lo;
          g = 255;
          b = 0;
          break; // Y to G
        case 2:
          r = lo;
          g = 255;
          b = 0;
          break; // G to Y
        default:
          r = 255;
          g = 255 - lo;
          b = 0;
          break; // Y to R
      }
      break;
    case 2: //GB Line only
      h %= 1024;
      if (h < 0) h += 1024;
      lo = h & 255;
      switch (h >> 8) {
        case 0:
          r = 0;
          g = 255;
          b = lo;
          break; // G to C
        case 1:
          r = 0;
          g = 255 - lo;
          b = 255;
          break; // C to B
        case 2:
          r = 0;
          g = lo;
          b = 255;
          break; // B to C
        default:
          r = 0;
          g = 255;
          b = 255 - lo;
          break; // C to G
      }
      break;
    case 3: //BR Line only
      h %= 1024;
      if (h < 0) h += 1024;
      lo = h & 255;
      switch (h >> 8) {
        case 0:
          r = lo;
          g = 0;
          b = 255;
          break; // B to M
        case 1:
          r = 255;
          g = 0;
          b = 255 - lo;
          break; // M to R
        case 2:
          r = 255;
          g = 0;
          b = lo;
          break; // R to M
        default:
          r = 255 - lo;
          g = 0;
          b = 255;
          break; // M to B
      }
      break;
  }

  satMultiplier = s + 1;
  r = 255 - (((255 - r) * satMultiplier) >> 8);
  g = 255 - (((255 - g) * satMultiplier) >> 8);
  b = 255 - (((255 - b) * satMultiplier) >> 8);

  valueMultiplier = (v + 1.0) / 256.0;

  color[0] = (int) (r * valueMultiplier);
  color[1] = (int) (g * valueMultiplier);
  color[2] = (int) (b * valueMultiplier);
}