# Arduino Code for NMR Autosampler

## Introduction

The "NMR-Killer" is a 3D-printed autosampler for the [Magritek Spinsolve](https://magritek.com/products/spinsolve/) benchtop NMR spectrometer, based on Arduino, Python, and PHP.

This repository contains the **Arduino** code for the "NMR-Killer".

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

You'll probably want to set up the autosampler control software on your PC next. Continue with the installation of the [webinterface](https://github.com/marcodyga/nmr_autosampler_webapp).

## Licence

This code is available under the conditions of [GNU General Public Licence version 3](https://www.gnu.org/licenses/gpl-3.0.en.html) or any later version.
