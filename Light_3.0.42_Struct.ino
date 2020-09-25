/* The color fading pattern https://github.com/mathertel/DMXSerial/blob/master/examples/DmxSerialSend/DmxSerialSend.ino ******* COLOR ChANGING THRU DMX
 *  
 * WORKING 6/24 basic functionality
 * WORKS AS EXPECTED except extra color changing functionality
 * WORKS WELL
 * 

single press:       if off, turn on medium; if on, turn off

double press:       if off, turn on max; if on, turn off

triple press:       start/stop color flow/fade?

single press+hold:  (if off, turn on low and) fade up; (if on, ) fade down;

double press+hold:  ()

triple press+hold:  ()

 * On press, add 1 to press counter   //check if buttons have been released (releaseTimer)??      (Start of buttonSequence
 * start heldDelayTimer //delay before it starts to adjust
 * 
 * On release
 * start release timer    //max time between release and new press
 * 
 *
 * if heldDelayTImer is ended, check state of button.   //held long enough for fade up / down ( end of buttonSequence )
 *  if held, (if releaseTimer is 0 (button has not been released lately) == double check)
 *    then held, do held action (based on pressCounter will do held action for 1, 2, or 3 presses)
 *  if not held,    //no action
 * 
 * 
 * after end of releaseTimer, check button?       //quarter second after release of first, second, third press
 *  if button == 0 && releaseTimer == 0     //no additional button was pressed
 *    end of this buttonSequence
 *    start action based on buttonCount
 *    reset button count
 *  if button was pressed 
 *    do nothing 
 *  (register an additional buttonpress with "On Press" so postpone action until decision is finalized)
 */


#include <avr/pgmspace.h> //for storing in progmem
#include <DmxSimple.h>
const String VERSION = "Light_3.0.4: Structs and Light ID\n - SERIAL Enabled; DMX Disabled";
const bool DEBUG = false;                                                                                       // DEBUG }}

const uint8_t DMX_PIN = 3;
const uint8_t BUTTON_PIN = A3;

const uint8_t CHANNELS = 64;    //uses only 32 channels, but every odd channel from 1-63, so set max to 64.
const uint8_t BUTTON_COUNT = 3;
const uint8_t MAX_PRESS_COUNT = 6;

//[main, kitchen, upstairs]
const uint16_t BUTTON_RESISTANCE[] = {1023, 706, 544};  // each button produces different resistances on single button input pin, reducing pin requirements but preventing simultaneous multi-button use.
const uint8_t BUTTON_RESISTANCE_TOLERANCE = 100;  // +- 50  
const uint16_t BUTTON_RELEASE_TIMER = 250;        // time before release action is processed; time allowed between release and next press for multipresses
const uint8_t BUTTON_FADE_DELAY = 230;               // minimum time the button must be held to start "held" action;

const uint16_t MAX_BRIGHTNESS = 1023;             // index of array
const uint8_t MIN_BRIGHTNESS = 1;               //
const uint16_t DEFAULT_BRIGHTNESS = 400;         // one press turns on to this brightness //supposed to feel around 50%.
const float FADE_FACTOR = 1.8;                       //base fade adjustment; modified by lightSection[].BRIGHTNESS_FACTOR

const uint8_t LOOP_DELAY_INTERVAL = 15;          // refresh speed in ms          {for ms=1: factor=0.00001; amount=0.018}
uint32_t loopStartTime = 0;              // loop start time
const uint16_t MIDDLE = MAX_BRIGHTNESS/2;
const uint16_t RED_LIST[]   = {MAX_BRIGHTNESS, MIDDLE,   0,   0,   0, MIDDLE};    // colorProgress cycle layout
const uint16_t GREEN_LIST[] = {  0, MIDDLE, MAX_BRIGHTNESS, MIDDLE,   0,   0};
const uint16_t BLUE_LIST[]  = {  0,   0,   0, MIDDLE, MAX_BRIGHTNESS, MIDDLE};
const uint8_t COLOR_PROGRESS_DELAY_COUNT = 1;   // color progresses every n loop occurances



  //Storing in PROGMEM allows for such a large array (1023 values). Without PROGMEM, an array of 512 values pushed the limit of dynamic memory but now it's ~51%.
const uint8_t DIMMER_TABLE_WIDTH = 32;
const uint8_t DIMMER_TABLE_HEIGHT = 32;
const uint8_t DIMMER_LOOKUP_TABLE[DIMMER_TABLE_HEIGHT][DIMMER_TABLE_WIDTH] PROGMEM = {      //light values lookup table, built on an S curve, to make LED dimming feel linear.
{ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, },
{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, },
{ 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, },
{ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, },
{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, },
{ 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, },
{ 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, },
{ 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, },
{ 13, 13, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 19, },
{ 19, 19, 19, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 23, 24, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, },
{ 27, 27, 27, 27, 28, 28, 28, 29, 29, 29, 30, 30, 30, 30, 31, 31, 31, 32, 32, 32, 33, 33, 33, 34, 34, 34, 35, 35, 35, 36, 36, 37, },
{ 37, 37, 38, 38, 38, 39, 39, 40, 40, 40, 41, 41, 42, 42, 42, 43, 43, 44, 44, 45, 45, 45, 46, 46, 47, 47, 48, 48, 49, 49, 49, 50, },
{ 50, 51, 51, 52, 52, 53, 53, 54, 54, 55, 55, 56, 56, 57, 57, 58, 58, 59, 60, 60, 61, 61, 62, 62, 63, 63, 64, 64, 65, 66, 66, 67, },
{ 67, 68, 69, 69, 70, 70, 71, 71, 72, 73, 73, 74, 75, 75, 76, 76, 77, 78, 78, 79, 80, 80, 81, 82, 82, 83, 84, 84, 85, 86, 86, 87, },
{ 88, 88, 89, 90, 90, 91, 92, 92, 93, 94, 94, 95, 96, 96, 97, 98, 99, 99, 100, 101, 101, 102, 103, 104, 104, 105, 106, 107, 107, 108, 109, 109, },
{ 110, 111, 112, 112, 113, 114, 115, 115, 116, 117, 118, 118, 119, 120, 121, 121, 122, 123, 124, 124, 125, 126, 127, 127, 128, 129, 130, 130, 131, 132, 133, 133, },

{ 134, 135, 136, 136, 137, 138, 139, 139, 140, 141, 142, 142, 143, 144, 145, 145, 146, 147, 147, 148, 149, 150, 150, 151, 152, 153, 153, 154, 155, 155, 156, 157, },
{ 158, 158, 159, 160, 160, 161, 162, 162, 163, 164, 165, 165, 166, 167, 167, 168, 169, 169, 170, 171, 171, 172, 173, 173, 174, 175, 175, 176, 177, 177, 178, 178, },
{ 179, 180, 180, 181, 182, 182, 183, 183, 184, 185, 185, 186, 186, 187, 188, 188, 189, 189, 190, 191, 191, 192, 192, 193, 193, 194, 194, 195, 196, 196, 197, 197, },
{ 198, 198, 199, 199, 200, 200, 201, 201, 202, 202, 203, 203, 204, 204, 205, 205, 206, 206, 207, 207, 208, 208, 208, 209, 209, 210, 210, 211, 211, 212, 212, 212, },
{ 213, 213, 214, 214, 214, 215, 215, 216, 216, 216, 217, 217, 218, 218, 218, 219, 219, 220, 220, 220, 221, 221, 221, 222, 222, 222, 223, 223, 223, 224, 224, 224, },
{ 225, 225, 225, 226, 226, 226, 227, 227, 227, 227, 228, 228, 228, 229, 229, 229, 229, 230, 230, 230, 231, 231, 231, 231, 232, 232, 232, 232, 233, 233, 233, 233, },
{ 234, 234, 234, 234, 235, 235, 235, 235, 235, 236, 236, 236, 236, 237, 237, 237, 237, 237, 238, 238, 238, 238, 238, 239, 239, 239, 239, 239, 239, 240, 240, 240, },
{ 240, 240, 241, 241, 241, 241, 241, 241, 242, 242, 242, 242, 242, 242, 242, 243, 243, 243, 243, 243, 243, 243, 244, 244, 244, 244, 244, 244, 244, 245, 245, 245, },
{ 245, 245, 245, 245, 245, 246, 246, 246, 246, 246, 246, 246, 246, 246, 247, 247, 247, 247, 247, 247, 247, 247, 247, 247, 248, 248, 248, 248, 248, 248, 248, 248, },
{ 248, 248, 248, 249, 249, 249, 249, 249, 249, 249, 249, 249, 249, 249, 249, 249, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 251, 251, },
{ 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, },
{ 252, 252, 252, 252, 252, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, },
{ 253, 253, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, },
{ 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
{ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
{ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
};

struct button_t {
    uint32_t releaseTimer;    //when was this button released?
    uint32_t timePressed;     //when was this button pressed?
    uint8_t pressedCount;     //If button is pressed before releaseTimer ends, add one to count
    bool beingHeld;           //is the button being held? (for longer than BUTTON_FADE_DELAY
    uint16_t RESISTANCE;      //grabs BUTTON_RESISTANCE constant defined above
    uint8_t lightID;         //which index of lightSection this button controls

} button[] = {
    {0, 0, 0, false, BUTTON_RESISTANCE[0], 0},  //entry button
    {0, 0, 0, false, BUTTON_RESISTANCE[1], 1},  //kitchen button (left)
    //{0, 0, 0, false, BUTTON_RESISTANCE[2], 7},  //Back wall button
    //{0, 0, 0, false, BUTTON_RESISTANCE[2], 2},  //Bath light button
    {0, 0, 0, false, BUTTON_RESISTANCE[2], 2},  //sconce 1 button
    
};

struct lightSection_t {

    float RGBW[4];              //stores current RGBW color levels
    float lastRGBW[4];          //last color levels
    int8_t fadeDir[4];          //RGBW fadeDir
    
    bool isOn;                  // Are any levels > 0?
    int8_t fadeDirection;       // -1, 1
    float BRIGHTNESS_FACTOR;    // affects default brightness and fade speed.
    uint8_t DMXout;             // DMX OUT number (set of 4 channels) this lightSection controls
        //colorProgress variables for this section
    uint8_t colorDelayCounter;  //slows the color cycle
    uint8_t colorState;         //next color state in the cycle
    bool colorProgress;         //while true, colors for this section will cycle

    uint8_t nextRGB[3];         //next state of RGB for colorProgress cycle
    uint8_t colorCycleFadeLevel;
    int8_t colorCycleFadeDir;

} lightSection[] = {
    {{0., 0., 0., 0.}, {0., 0., 0., 0.}, {1, 1, 1, 1},      false, 1, 1., 4,        0, 0, false,        {0, 0, 0}, 0, 1},      //  ID 0    Living Room Lights

    {{0., 0., 0., 0.}, {0., 0., 0., 0.}, {1, 1, 1, 1},      false, 1, 1.2, 3,       0, 0, false,        {0, 0, 0}, 0, 1},      //  ID 1    Kitchen Lights
    
    {{0., 0., 0., 0.}, {0., 0., 0., 0.}, {1, 1, 1, 1},      false, 1, 1.2, 2,        0, 0, false,        {0, 0, 0}, 0, 1},      //  ID 2   PORCH LIGHTS
    
//below: not yet used
//    {{0., 0., 0., 0.}, {0., 0., 0., 0.}, {false, false, false, false}, {1, 1, 1, 1},      false, 1, 1., 0,        0, 0, false,        {0, 0, 0}, 0, 1},      //  ID 2    Bath Light
//
//    {{0., 0., 0., 0.}, {0., 0., 0., 0.}, {false, false, false, false}, {1, 1, 1, 1},      false, 1, 1., 0,        0, 0, false,        {0, 0, 0}, 0, 1},      //  ID 3    Overhead Bedroom
//
//    {{0., 0., 0., 0.}, {0., 0., 0., 0.}, {false, false, false, false}, {1, 1, 1, 1},      false, 1, 1., 0,        0, 0, false,        {0, 0, 0}, 0, 1},      //  ID 4    Overhead Main
//
//    {{0., 0., 0., 0.}, {0., 0., 0., 0.}, {false, false, false, false}, {1, 1, 1, 1},      false, 1, 1., 0,        0, 0, false,        {0, 0, 0}, 0, 1},      //  ID 5    Overhead Small Loft
//    
//    {{0., 0., 0., 0.}, {0., 0., 0., 0.}, {false, false, false, false}, {1, 1, 1, 1},      false, 1, 1., 0,        0, 0, false,        {0, 0, 0}, 0, 1},      //  ID 6    Exterior Lights
//
//    {{0., 0., 0., 0.}, {0., 0., 0., 0.}, {false, false, false, false}, {1, 1, 1, 1},      false, 1, 1., 0,        0, 0, false,        {0, 0, 0}, 0, 1},      //  ID 7    Greenhouse Lights

    
    
};

//************************************************************************************************************************************************************ END VARIABLES

void progressColor(uint8_t id) {
    if (lightSection[id].colorDelayCounter < COLOR_PROGRESS_DELAY_COUNT) {
        lightSection[id].colorDelayCounter++;
    } else {
        lightSection[id].colorDelayCounter = 0;
        lightSection[id].nextRGB[0] = RED_LIST[lightSection[id].colorState];           // target levels for the current state
        lightSection[id].nextRGB[1] = GREEN_LIST[lightSection[id].colorState];
        lightSection[id].nextRGB[2] = BLUE_LIST[lightSection[id].colorState];
        
        if ((lightSection[id].nextRGB[0] == lightSection[id].RGBW[0]) && (lightSection[id].nextRGB[1] == lightSection[id].RGBW[1]) && (lightSection[id].nextRGB[2] == lightSection[id].RGBW[2])) {
                lightSection[id].colorState += 1;
                if (lightSection[id].colorState == 6) 
                    lightSection[id].colorState = 0;
        
        } else {
            for (uint8_t i = 0; i < 3; i++) {
                if (lightSection[id].RGBW[i] < lightSection[id].nextRGB[i])  lightSection[id].RGBW[i]++; 
                else if (lightSection[id].RGBW[i] > lightSection[id].nextRGB[i])  lightSection[id].RGBW[i]--; 
            }

        }
        updateLights(id);
    }
}

void setLight(button_t *BUTTON) { // set lights to a value, takes button struct and gets info for what will happen (for press actions)
    uint8_t id = BUTTON->lightID;
    uint8_t count = BUTTON->pressedCount;
    uint16_t brightness = 0;
    if (DEBUG == true) {
      Serial.println(" ");
      Serial.print(count);
    }

    if (count == 3) {   //  colorProgress
        if (lightSection[id].colorProgress == false) {
            lightSection[id].colorState = random(6);   //get a random state to start at
            lightSection[id].colorProgress = true;
            if (lightSection[id].isOn == true) {
                for (uint8_t i = 0; i<4; i++) lightSection[id].RGBW[i] = brightness;
            } else {
                lightSection[id].isOn = true;
            }
        } else {
            lightSection[id].colorProgress = false;
            lightSection[id].isOn = false;
            for (uint8_t i = 0; i<4; i++) lightSection[id].RGBW[i] = brightness;
        }
      
    } else if (count == 2) {  //  All (RGBW)
        if (lightSection[id].isOn == true) {
            lightSection[id].isOn = false;
            lightSection[id].colorProgress = false;
        } else {
            brightness = MAX_BRIGHTNESS;
            lightSection[id].isOn = true;
        }
        for (uint8_t i = 0; i<4; i++) lightSection[id].RGBW[i] = brightness;
      
    } else {  //  count = 1, 4, 5, 6    (white, r, g, b)
        if (lightSection[id].isOn == true) {
            lightSection[id].isOn = false;
        } else {
            brightness = DEFAULT_BRIGHTNESS*lightSection[id].BRIGHTNESS_FACTOR;
            lightSection[id].isOn = true;
        }
        
        if (count != 1) {   //  RGB
            lightSection[id].colorProgress = false;        //if turning on specific color, we want colorProgress off.
            lightSection[id].RGBW[count-4] = brightness;
        } else {
            lightSection[id].RGBW[3] = brightness;  //  white
        }
            
    }
    updateLights(id);
}

void updateLights(uint8_t id) {   //updates a specific light section
    uint16_t brightness;
    if (DEBUG == true) {                                                                                                                                                                           // DEBUG }}
            const uint8_t i= 3;
            uint8_t height = (uint16_t(lightSection[id].RGBW[i])/DIMMER_TABLE_HEIGHT);
            uint8_t width = (uint16_t(lightSection[id].RGBW[i])%DIMMER_TABLE_WIDTH);
            memcpy_P(&brightness, &(DIMMER_LOOKUP_TABLE[height][width]), sizeof(brightness)); //  looks up brighness from table and saves as uint8_t brightness
            Serial.print(height);
            Serial.print(F("."));
            Serial.print(width);
            Serial.print(F(" "));
            Serial.print(brightness);
            Serial.print(F(" W: "));   Serial.print(lightSection[id].RGBW[3]); Serial.print(F(" R: "));  Serial.print(lightSection[id].RGBW[0]);
            Serial.print(F(" G: "));  Serial.print(lightSection[id].RGBW[1]); Serial.print(F(" B: "));  Serial.print(lightSection[id].RGBW[2]);
            Serial.print(F(" "));     Serial.println(millis());
    } else {
        for (uint8_t i = 0; i < 4; i++) {
            if (lightSection[id].lastRGBW[i] != lightSection[id].RGBW[i]) {
                uint8_t height = (uint16_t(lightSection[id].RGBW[i]) / DIMMER_TABLE_HEIGHT);
                uint8_t width = (uint16_t(lightSection[id].RGBW[i]) % DIMMER_TABLE_WIDTH);
                memcpy_P(&brightness, &(DIMMER_LOOKUP_TABLE[height][width]), sizeof(brightness)); //  looks up brighness from table and saves as uint8_t brightness ( sizeof(brightness) resolves to 1 [byte of data])

                DmxSimple.write( lightSection[id].DMXout*8-8+(i*2+1), brightness );   //main underloft lights
                lightSection[id].lastRGBW[i] = lightSection[id].RGBW[i];
            }
        }
    }
    if (lightSection[id].RGBW[3] == 0 && lightSection[id].RGBW[0] == 0 && lightSection[id].RGBW[1] == 0 && lightSection[id].RGBW[2] == 0)
        lightSection[id].isOn = false;
}


void fadeAdj (uint8_t id, uint8_t i) {
    if (lightSection[id].fadeDirection == 1) {
        if (lightSection[id].RGBW[i] < (MAX_BRIGHTNESS - lightSection[id].BRIGHTNESS_FACTOR*FADE_FACTOR)) lightSection[id].RGBW[i] += lightSection[id].BRIGHTNESS_FACTOR*FADE_FACTOR;
        else lightSection[id].RGBW[i] = MAX_BRIGHTNESS;
                    
    } else if (lightSection[id].fadeDirection == -1) {
        if (lightSection[id].RGBW[i] > (MIN_BRIGHTNESS + lightSection[id].BRIGHTNESS_FACTOR*FADE_FACTOR)) lightSection[id].RGBW[i] -= lightSection[id].BRIGHTNESS_FACTOR*FADE_FACTOR;
        else lightSection[id].RGBW[i] = 0;
    }
}

void fade(button_t *BUTTON) {
    uint8_t id = BUTTON->lightID;   // temp variable for simpler typing
    if (DEBUG == true) {                                                                                                   // {{ DEBUG }}
        Serial.print(F("Fade: "));
        Serial.println(lightSection[id].fadeDirection);
    }
    if (BUTTON->pressedCount == 2) {    //  All (RGBW)
        for (uint8_t i=0; i<4; i++) {   //for each color, set them.
            fadeAdj(id, i);
        }
    } else if (BUTTON->pressedCount == 1) {   //  white
        fadeAdj(id, 3);
    } else if (BUTTON->pressedCount == 3) {   //  colorProgress
      //TODO
    } else {  //4, 5, 6
        fadeAdj(id, BUTTON->pressedCount - 4);  //  RGB
    }
    updateLights(id);
}

void setup() {//************************************************************************************************* SETUP
    if (DEBUG == true) {                                                                                                                                                                           // DEBUG }}
        Serial.begin(9600);
        Serial.println(VERSION);
    } else {
        DmxSimple.maxChannel(CHANNELS);
        DmxSimple.usePin(DMX_PIN);
        for (uint8_t i = 1; i <= CHANNELS; i++) DmxSimple.write(i, 0);
    }
    randomSeed(analogRead(0));    //get random seed; used to start colorProgress state at a random color
    pinMode(BUTTON_PIN, INPUT);
    loopStartTime = millis();
}

void loop() {//************************************************************************************************* LOOP
    uint32_t currentTime = millis();
    if ((currentTime - loopStartTime) >= LOOP_DELAY_INTERVAL) {   // timer for updatiing the loop
        loopStartTime += LOOP_DELAY_INTERVAL;                       // set time for next timer
        
        uint16_t buttonStatus = analogRead(BUTTON_PIN);                  // read the button pin to check if any buttons are pressed

        for (uint8_t i = 0; i < BUTTON_COUNT; i++) {                     // cycle through each button, check buttonStatus against each button's RESISTANCE constant to detect which button is pressed
            bool thisButton = (buttonStatus >= (button[i].RESISTANCE - BUTTON_RESISTANCE_TOLERANCE) &&  buttonStatus <= (button[i].RESISTANCE + BUTTON_RESISTANCE_TOLERANCE));  // answers the question: is button[i] the button that is being pressed? [true/false]
            uint8_t id = button[i].lightID;                           // temp variable to save on typing
            if (lightSection[id].colorProgress == true) {            // progress color if necessary
                progressColor(id);
            }
            // REGISTER PRESSES/RELEASES:
            if (buttonStatus <= 64 ) {                            // if no button is pressed:
                if (button[i].timePressed > 0) {                    //  check if button[i] was pressed on the last loop through by checking for non-zero timePressed
                    button[i].releaseTimer = currentTime + BUTTON_RELEASE_TIMER;  // if so, since it is no longer pressed we set the releaseTimer
                    button[i].timePressed = 0;                                    // and since it's not pressed, we reset timePressed to 0 so the next press is detected as a new press rather than a held press.
                    
                } else {                                            // else if timePressed == 0 then button[i] is already in the "RELEASED" phase
                    if (currentTime >= button[i].releaseTimer) {      // In that case, check current time against releaseTimer to see if enough time has passed to start RELEASE action.
                        if (button[i].beingHeld == true) {              // RELEASE ACTION
                            button[i].beingHeld = false;
                            
                        } else {  // if button is not being held, commence RELEASE ACTION
                            uint8_t brightness = 0;
                            if (button[i].pressedCount > 0) {
                                if (button[i].pressedCount > MAX_PRESS_COUNT) button[i].pressedCount = MAX_PRESS_COUNT;
                                
                                setLight( &button[i] );  // turn on or off white
                                if (DEBUG == true) {                                                                                                   // {{ DEBUG }}
                                    Serial.print(F(" "));
                                    Serial.println(button[i].pressedCount);
                                }
                            }
                        }
                        button[i].pressedCount = 0;                                        // after RELEASE ACTIONS, reset pressedCount counter to 0.
                    }// END RELEASE ACTIONS
                }
                
            } else { // else buttonStatus > 64; a button is being pressed.  // PRESSED ACTIONS
                if (DEBUG == true) {                                                                                                     // {{ DEBUG }}
                    Serial.print(F(" "));
                    Serial.print(buttonStatus); // (print button reading)
                }
                if (thisButton == true) {   // Check if the detected button is button[i] by referring to the RESISTANCE constants
                    if (button[i].timePressed == 0 )  {                              // if button[i].timePressed == 0 this is a NEW button press
                        button[i].timePressed = currentTime;                                                            // save the time to detect multipresses
                        button[i].pressedCount++;                                                                       // add one press to its counter
                        
                    } else if (currentTime >= button[i].timePressed + BUTTON_FADE_DELAY) { //  if button has been pressed and held past BUTTON_FADE_DELAY      // BUTTON HELD ACTION
                        if (button[i].beingHeld == false) {                         //  if not yet held, initialize fading:
                            button[i].beingHeld = true;
                            if (lightSection[id].isOn == false) {                                                     // if light is off, fade up; else flip-flop fade direction 
                                lightSection[id].fadeDirection = 1;
                                lightSection[id].isOn = true;
                            } else {
                                lightSection[id].fadeDirection *= -1;
                            }
                        }
                        fade( &button[i] ); //fade whichever light section this button is controlling
                    }
                }
            }// end {button held} thread
        } // end {check each button} for loop
    }// timer
}// void
