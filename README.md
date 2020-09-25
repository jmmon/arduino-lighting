# arduino-lighting
Uses Arduino Nano and DMX to control RGB+WW strips.

Uses 1 input pin (for buttons, distinguished by their resistor) and 1 output pin (DMX).

Functionality: detects how many consecutive presses, and whether the button was held:
  - 1 press: turn on (medium brightness WW) / turn off (WW)
  - 1 press+hold: fade up or down WW, toggling between fading up and fading down
  
  - 2 presses: turn RGB+WW on max / turn off ALL
  - 2 press+hold: fade up/down ALL,...
  
  - 3 presses: activate "colorProgress": pick a random RGB color to start, and progress through the rainbow.
  
  - 4 presses: turn on R to medium brightness / turn off R
  - 4 press+hold: fade up/down R,...
  
  - 5 presses: turn on G to medium brightness / turn off G
  - 5 press+hold: fade up/down G,...
  
  - 6 presses: turn on B to medium brightness / turn off B
  - 6 press+hold: fade up/down B,...

Kind of a pain to get to R, G, B.

Next version being worked on is ditching the 1 input pin idea and has a separate pin per set of two buttons, and each set of buttons will control one light section. Top button will fade up, bottom button will fade down, and some other easier functionality for switching modes/colors.
