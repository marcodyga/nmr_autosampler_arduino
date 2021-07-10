# Arduino Code for NMR Autosampler

## Introduction

The "RotoMate" is a 3D-printed autosampler for the [Magritek Spinsolve](https://magritek.com/products/spinsolve/) benchtop NMR spectrometer, based on Arduino, Python, and PHP.

Our open-access manuscript with detailed build instructions and CAD data can be found under the following URL: https://doi.org/10.1016/j.ohx.2021.e00211

This repository contains the **Arduino** code for the "RotoMate".

## Third-party libraries

The following [libraries](https://www.arduino.cc/en/guide/libraries) are required for this program:

| Library           | Licence      | Weblink                                               |
| ------------------|--------------|-------------------------------------------------------|
| AccelStepper      | GPLv3        | https://www.airspayce.com/mikem/arduino/AccelStepper/ |
| New-LiquidCrystal | GPLv3        | https://github.com/fmalpartida/New-LiquidCrystal      |
| Keypad            | GPLv3        | https://github.com/Chris--A/Keypad                    |
| Keypad_I2C        | GPLv3/LGPLv3 | https://github.com/joeyoung/arduino_keypads           |

## Setup

Download the latest version of the [Arduino IDE](https://www.arduino.cc/en/software). Install the [libraries](https://www.arduino.cc/en/guide/libraries) listed in the paragraph above. Then, connect your autosampler's Arduino to your PC via USB, and compile and upload the sketch "autosampler.ino".

## Hardware-specific modifications

### Stepper motor

The autosampler uses a NEMA 23 stepper motor to turn the rotor and select the correct sample. This motor is driven by a DFRobot DRV8825 motor shield ([for documentation, click here](https://wiki.dfrobot.com/Stepper_Motor_Shield_For_Arduino_DRV8825__SKU_DRI0023)). The shield allows to reduce the step size ("microstep resolution") to a fraction of the original value, improving the motor's precision. Crucially, **every motor I have tested has behaved differently at different microstep sizes!** While one operated smoothly with 1/16 steps, others refused to work with this setting and required a much coarser resolution.

It is therefore very important to test different microstep resolutions with the individual motor used in the autosampler, and stick to a setting which results in **smooth movement of the rotor** without any "jittering" movements.

Additionally, there are motors on the market with different step angles (typically 1.8° --> 200 steps per revolution, or 0.9° --> 400 steps per revolution). 

The arduino software **must know** these settings, i.e. **how far the rotor will turn when one "step" is performed**. This is set in `autosampler.ino` at the following position:

```cpp
// The "step size" of the stepper motor driving the rotor.
// Set this value such that the number of steps per revolution is 50*stepsize.
// For a motor with 200 steps per revolution, stepsize is the inverse "microstep revolution" (on the stepper driver 
// shield), multiplied by 4. For example, if 1/16-steps are configured, this value should be 64. 
// For a motor with 400 steps per revolution and a microstep resolution of 1/16, this value would be 128.
int stepsize = 64; 
```

Adjust the value ```stepsize``` such that the number of steps per revolution divided by 50 is ```stepsize```.

Depending on your motor, it might additionally be worthwile to tune the speed and acceleration parameters:

```cpp
// Standard speed
int standardSpeed = 4*stepsize;
// speeds for acceleration modification
float maximumSpeed = 4*stepsize;
float acceleration = 3.5*stepsize;
```

### I2C modules (LCD and Keypad)

The two modules connected by the digital I2C bus are the LCD and the Keypad. Make sure that you correctly define the I2C addresses of each module at the following positions:

LCD Display:

```cpp
// LCD display stuff
#define lcdAddress    0x27  // Define I2C Address for controller
```

Keypad:

```cpp
#define keypadAddress 0x20 // I2C Address of the PCF8574 breakout board for the keypad. Can be set using the jumpers on the board.
```

If you do not know the correct addresses, use the [I2C Scanner](https://playground.arduino.cc/Main/I2cScanner/) to find out.

### LCD

The LCD in particular also needs to be initialized correctly. This is done in the code at the following position:

```cpp
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7
#define LED_OFF  0
#define LED_ON  1A
LiquidCrystal_I2C  lcd(lcdAddress,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);
```

It seems that different LCD modules tend to use different configurations here, and it is often down to trial and error to find the correct one. Fortunately, there is Bill Perry's "I2C LCD Guesser" which cycles through all the possible configurations until it finds the correct one - add the configuration found this way into the code shown above.

Currently, you can find the LCD guesser for example in [this thread of the Arduino forum](https://forum.arduino.cc/index.php?topic=412355.0). Unfortunately, I am not allowed to publish it in this repository due to licencing restrictions. If the link is dead, it should be possible to find it via Google search.

### Keypad

Depending on the way the PCF8574 module is mounted, the keypad pins might be inverted. If the keypad does not work properly, play with the values shown below.

```cpp
char keypadLayout[keypadRows][keypadCols] = {
  {'1','2','3','A'}, 
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte keypadRowPins[keypadRows] = {7,6,5,4};
byte keypadColPins[keypadCols] = {3,2,1,0};
```

## Next step: Setup on the PC

You'll probably want to set up the autosampler control software on your PC next. Continue with the installation of the [webinterface](https://github.com/marcodyga/nmr_autosampler_webapp).

## Licence

This code is available under the conditions of [GNU General Public Licence version 3](https://www.gnu.org/licenses/gpl-3.0.en.html) or any later version.
