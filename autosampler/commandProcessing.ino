// Processing of commands from the user or the Autosampler table.

void comport_proc() {
  // This part deals with input from the COM port.
  // Look through the cases below to find out what you need to yell at the Autosampler so it does your bidding
  if(listening == 0) {
    // the Autosampler should only listen to the comport, if no one is doing anything on the numpad.
    input = Serial.read();
    if(input != -1) {
      //Serial.println(input);
      //Initialize variable which might be needed later
      bool success = false;
      switch(input) {
        case 69: //E
          // Raise error from outside
          unexpectedError = 9;
          errorcode = '9';
          break;
        case 77: //M
          // Measure sample at position.
          i = getPositionNumber();
          if(i != -1) {
            measureSample(i);
          }
          break;
        case 78: //N
          // Measure sample at position inside of a running Queue. Does not eject to holder 32 first.
          i = getPositionNumber();
          if(i != -1) {
            measureSample(i, true);
          }
          break;
        case 82: //R
          // Return measured sample to position.
          i = getPositionNumber();
          if(i != -1) {
            returnSample(i);
          }
          break;
        case 97: //a
          setPusher(true);
          break;
        case 98: //b
          setPusher(false);
          setAirHard(false);
          break;
        case 99: //c
          setAir(true);
          break;
        case 100: //d
          setAir(false);
          break;
        case 101: //e
          // "eject"
          success = removeSample();
          if(success) {
            runMeasurement = -1;
          }
          break;
        case 104: //h
          homing();
          break;
        case 105: //i
          success = insertSample();
          if(success) {
            runMeasurement = 0;
            listening = 0;
          }
        case 107: //k
          calibration();
          break;
        case 108: //l
          // Lock the motor. Motor can only be used in locked state.
          myStepper.enableOutputs();
          break;
        case 109: //m
          // move to position.
          i = getPositionNumber();
          if(i != -1) {
            gotoPos(i);
          }
          break;
        case 114: //r
          // reset unexpected error mode, exit calibration/listening mode, remove measurement
          listening = 0;
          unexpectedError = 0;
          runMeasurement = -1;
          lockLCD = false;
          break;
        case 115: //s
          // set speed
          input = getNextDigit();
          if(input != -1) {
            input++;
            myStepper.setSpeed(input*stepsize);
          }
          break;
        case 116: //t
          // Test function (requires all positions to be filled)
          test();
          break;
        case 117: //u
          // Unlock the motor. Motor will stop being active and getting heated up.
          myStepper.disableOutputs();
          break;
        case 122: //z
          // make some noise
          buzz(500);
          break;
      }
    }
  }
}

void numpad_proc() {
  //  This part deals with input from the numpad.
  currentKey = i2cKeypad.getKey();
  if(currentKey != NO_KEY) {
    // A fresh key has been pressed. Decide what to do.
    if(currentKey == 'D') {
      // press D -> Reset error mode, turn off listening and calibration mode, 
      // if not in listening mode and no error is present, cancel measurement mode.
      if(lockLCD) {
        lockLCD = false;
      } else if(unexpectedError != 0) {
        unexpectedError = 0;
      } else if (listening != 0) {
        listening = 0;
      } else {
        runMeasurement = -1;
      }
    } else {
      //-------------------------------------
      // START OF CALIBRATION MODE PART
      //-------------------------------------
      // here is the movement control via the numpad
      if(listening == 1) {
        if(currentKey == '#') {
          // when # is pressed, stop listening and throw everything out
          numpadKeysPressed[0] = -1;
          numpadKeysPressed[1] = -1;
          listening = 0;
        } else if(currentKey == '*') {
          // start when * is pressed.
          if(numpadKeysPressed[1] == -1) {
            // one-digit number
            if(numpadKeysPressed[0] > 0) {
              homing();
              gotoPos(numpadKeysPressed[0]);
            } else {
              badInputWarning();
            }
          } else {
            // two-digit number
            numpadKeysPressed[0] = numpadKeysPressed[0]*10 + numpadKeysPressed[1];
            if(numpadKeysPressed[0] > 0 and numpadKeysPressed[0] <= 32) {
              homing();
              gotoPos(numpadKeysPressed[0]);
            } else {
              badInputWarning();
            }
          }
          numpadKeysPressed[0] = -1;
          numpadKeysPressed[1] = -1;
        } else if(isdigit(currentKey)) {
          // a number was pressed, it will be put into the numpadKeysPressed array.
          i = currentKey - '0';
          if(numpadKeysPressed[0] == -1) {
            numpadKeysPressed[0] = i;
          } else if(numpadKeysPressed[1] == -1) {
            numpadKeysPressed[1] = i;
          } else {
            // there were already 2 digits entered, and the user tried to enter a 3rd digit. 
            // this will reset everything like pressing the cancel button. then a short warning signal will be played.
            badInputWarning();
          }
        } else if(currentKey == 'A') {
          // go to calibration mode if A is pressed a second time
          listening++;
        }
      } else if(listening == 2) {
        if(currentKey == '1') {
          bool success = insertSample();
          if(success) {
            runMeasurement = 0;
            listening = 0;
          }
        } else if(currentKey == '2') {
          bool success = removeSample();
          if(success) {
            runMeasurement = -1;
          }
        } else if(currentKey == 'A') {
          // if no measurement is running, go to calibration mode
          // otherwise return to "go to holder"
          if(runMeasurement == -1) {
            listening++;
          } else {
            listening = 1;
          }
        }
      } else if(listening == 3) {
        // before the calibration of the rotor, a homing must be done to find the position 0.
        if(currentKey == '*') {
          // press * to run a homing.
          homing();
        } else if(currentKey == 'A') {
          listening++;
        }
      } else if(listening == 4) {
        // for the Calibration, the position should move forward when * is pressed and backward when # is pressed.
        // the step size (in variable j) can be changed with the numbers in the numpad.
        cP = myStepper.currentPosition();
        if(isdigit(currentKey)) {
          i = currentKey - '0';
          // change step size to 2^i
          calib_stepsize = 1<<i;
        } else if(currentKey == '*') {
          // move forward when * is pressed
          target = cP + calib_stepsize;
          runMotor = true;
        } else if(currentKey == '#') {
          // move backward when # is pressed
          target = cP - calib_stepsize;
          runMotor = true;
        } else if(currentKey == 'B') {
          // B --> Confirm value as calibrated and save it into EEPROM.
          POS1 = cP;
          writeIntIntoEEPROM(0, cP);
          customWriteLCD("Saved new value POS1=" + String(cP));
          writeLEDs(0,1,1);
          buzz(100);
          delay(2500);
        } else if(currentKey == 'A') {
          // go to Pusher calibration mode when A is pressed again
          if(calib_stepsize > 81) {
            calib_stepsize = 81;
          }
          listening++;
        } 
      } else if(listening == 5) {
        // The Calibration of the Pusher should work the same way as that for the rotor.
        if(isdigit(currentKey)) {
          // change step size to i^2
          i = currentKey - '0';
          calib_stepsize = i*i;
        } else if(currentKey == '*') {
          // move forward when * is pressed
          pusherServo.write(pusherServo.read() + calib_stepsize);
        } else if(currentKey == '#') {
          // move backward when # is pressed
          pusherServo.write(pusherServo.read() - calib_stepsize);
        } else if(currentKey == 'B') {
          // B --> Confirm SLIDER_PUSH as calibrated and save it into EEPROM.
          SLIDER_PUSH = pusherServo.read();
          writeIntIntoEEPROM(100, SLIDER_PUSH);
          customWriteLCD("Saved new value SLIDER_PUSH=" + String(SLIDER_PUSH));
          writeLEDs(0,1,1);
          buzz(100);
          delay(2500);
        } else if(currentKey == 'C') {
          // C --> Confirm SLIDER_PULL as calibrated and save it into EEPROM.
          SLIDER_PULL = pusherServo.read();
          writeIntIntoEEPROM(110, SLIDER_PULL);
          customWriteLCD("Saved new value SLIDER_PULL=" + String(SLIDER_PULL));
          writeLEDs(0,1,1);
          buzz(100);
          delay(2500);
        } else if(currentKey == 'A') {
          // go to air calibration mode when A is pressed again
          listening++;
        } 
      } else if(listening == 6) {
        // Calibration of the air servo, also works the same way as the other ones above.
        if(isdigit(currentKey)) {
          // change step size to i^2
          i = currentKey - '0';
          calib_stepsize = i*i;
        } else if(currentKey == '*') {
          // move forward when * is pressed
          airServo.write(airServo.read() + calib_stepsize);
        } else if(currentKey == '#') {
          // move backward when # is pressed
          airServo.write(airServo.read() - calib_stepsize);
        } else if(currentKey == 'B') {
          // B --> Confirm AIR_PUSH as calibrated and save it into EEPROM.
          AIR_PUSH = airServo.read();
          writeIntIntoEEPROM(200, AIR_PUSH);
          customWriteLCD("Saved new value AIR_PUSH=" + String(AIR_PUSH));
          writeLEDs(0,1,1);
          buzz(100);
          delay(2500);
        } else if(currentKey == 'C') {
          // C --> Confirm AIR_VENT as calibrated and save it into EEPROM.
          AIR_VENT = airServo.read();
          writeIntIntoEEPROM(210, AIR_VENT);
          customWriteLCD("Saved new value AIR_VENT=" + String(AIR_VENT));
          writeLEDs(0,1,1);
          buzz(100);
          delay(2500);
        } else if(currentKey == 'A') {
          listening++;
        }
      } else if(listening == 7) {
        if(currentKey == '*') {
          // press * to run a full test.
          bool continueFlag = false;
          customWriteLCD("Insert NMR tubes  then press *  ");
          while(!continueFlag) {
            delay(100);
            currentKey = i2cKeypad.getKey();
            if(currentKey == 'D' or currentKey == '#') {
              // cancel
              continueFlag = true;
            } else if(currentKey == '*') {
              listening = 0;
              continueFlag = true;
              test();
            }
          }
        } else if(currentKey == 'A') {
          // go to "move to position" mode when A is pressed again
          listening = 1;
        }
      //-------------------------------------
      // END OF CALIBRATION MODE PART
      //-------------------------------------
      } else {
        if(currentKey == 'A') {
          // start listening when A is pressed
          listening = 1;
        } else if(currentKey == 'B') {
          // toggle the pusher
          listening = 0;
          if(pusherServo.read() != SLIDER_PULL) {
            setPusher(false);
            setAirHard(false);
          } else {
            setPusher(true);
          }
        } else if(currentKey == 'C') {
          // press C -> toggle air
          listening = 0;
          if(airStatus == 1) {
            setAir(false);
          } else {
            if(pusherServo.read() == SLIDER_PUSH) {
              setAir(true);
            } else {
              setPusher(true);
            }
          }
        }
      }
    }
  }
}
