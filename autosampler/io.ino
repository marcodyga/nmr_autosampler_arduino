// Input and Output routines.

void badInputWarning() {
  // helper function for the part reading the numpad input.
  numpadKeysPressed[0] = -1;
  numpadKeysPressed[1] = -1;
  listening = 0;
  writeLEDs(1,1,1);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("     Warning    ");
  lcd.setCursor(0,1);
  lcd.print("  Bad input! :( ");
  LastDisplayString = "";
  delay(50);
  buzz(150);
  delay(50);
  buzz(150);
  delay(50);
  buzz(200);
  delay(2000);
}

int getNextDigit() {
  // Helper function for reading numbers from input.
  i = 0;
  do {
    input = Serial.read();
    i++;
    if(i > 3000) {
      break;
    }
    delayMicroseconds(1);
  } while(input == -1);
  input -= 48;
  if(input >= 0 and input < 10) {
    return input;
  } else {
    return -1;
  }
}

int getPositionNumber() {
  digit1 = getNextDigit();
  digit2 = getNextDigit();
  if(digit2 == -1) {
    // for the case that there was a number with one digit
    i = digit1;
  } else {
    // when a 2 digit number was given
    i = 10*digit1 + digit2;
  }
  if(i <= 40 and i > 0) {
    return i;
  } else {
    return -1;
  }
}

/*
   There are in total 5 values which should be saved in EEPROM. They will be saved at the following positions.

   Pos. | Value
   -----+----------------------------------------------------------------------
      0 | Motor calibration (POS1)
    100 | Pushing position of Pusher (PUSHER_PUSH)
    110 | Fully retracted position of pusher (PUSHER_PULL)
    200 | Position of Air Servo for connecting air and spectrometer (AIR_PUSH)
    210 | Position of Air Servo for venting air (AIR_VENT)

   Keep in mind that an int spans 2 positions!

*/

void getCalFromEEPROM() {
  // Reads the values from EEPROM according to the table above.
  // the position of Holder 1 !
  POS1 = readIntFromEEPROM(0);
  // the positions for start and end points of the Pusher.
  PUSHER_PUSH = readIntFromEEPROM(100);
  PUSHER_PULL = readIntFromEEPROM(110);
  // the positions of the air servo for venting and for pushing the NMR tube.
  AIR_PUSH = readIntFromEEPROM(200);
  AIR_VENT = readIntFromEEPROM(210);
}

void writeIntIntoEEPROM(int address, int number) { 
  // https://roboticsbackend.com/arduino-store-int-into-eeprom/
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

int readIntFromEEPROM(int address) {
  // https://roboticsbackend.com/arduino-store-int-into-eeprom/
  byte byte1 = EEPROM.read(address);
  byte byte2 = EEPROM.read(address + 1);

  return (byte1 << 8) + byte2;
}

void writeLEDs(int red, int yellow, int green) {
  if(red == 1) {
    digitalWrite(redLEDPin, HIGH);
  } else if(red == 2) {
    blinkred++;
    if(blinkred > 0) {
      digitalWrite(redLEDPin, HIGH);
      if(blinkred > blink_freq) {
        blinkred = -blink_freq;
      }
    } else {
      digitalWrite(redLEDPin, LOW);
    }
  } else {
    digitalWrite(redLEDPin, LOW);
  }
  if(yellow == 1) {
    digitalWrite(yellowLEDPin, HIGH);
  } else if(yellow == 2) {
    blinkyellow++;
    if(blinkyellow > 0) {
      digitalWrite(yellowLEDPin, HIGH);
      if(blinkyellow > blink_freq) {
        blinkyellow = -blink_freq;
      }
    } else {
      digitalWrite(yellowLEDPin, LOW);
    }
  } else {
    digitalWrite(yellowLEDPin, LOW);
  }
  if(green == 1) {
    digitalWrite(greenLEDPin, HIGH);
  } else if(green == 2) {
    blinkgreen++;
    if(blinkgreen > 0) {
      digitalWrite(greenLEDPin, HIGH);
      if(blinkgreen > blink_freq) {
        blinkgreen = -blink_freq;
      }
    } else {
      digitalWrite(greenLEDPin, LOW);
    }
  } else {
    digitalWrite(greenLEDPin, LOW);
  }
}

void writeBuzzer(int mode) {
  if(mode == 1) {
    digitalWrite(buzzerPin, HIGH);
  } else if(mode == 2) {
    blinkbuzzer++;
    if(blinkbuzzer > 0) {
      digitalWrite(buzzerPin, HIGH);
      if(blinkbuzzer > blink_freq_buzzer) {
        blinkbuzzer = -blink_freq_buzzer;
      }
    } else {
      digitalWrite(buzzerPin, LOW);
    }
  } else {
    digitalWrite(buzzerPin, LOW);
  }
}

void buzz(int ms) {
  digitalWrite(buzzerPin, HIGH);
  delay(ms);
  digitalWrite(buzzerPin, LOW);
}

void writeLCD() {
  // this must be called repeatedly in the loop, it will update the LCD, whenever
  // necessary.  
  if(!lockLCD) {
    if(listening == 1) {
      // show input until now, if a number is going to be entered via the numpad
      CurrentDisplayString = F("Go to holder #: ");
      if(numpadKeysPressed[0] != -1) {
        CurrentDisplayString += String(numpadKeysPressed[0]);
      }
      if(numpadKeysPressed[1] != -1) {
        CurrentDisplayString += String(numpadKeysPressed[1]);
      }
      CurrentDisplayString += "_";
    } else if(runMotor == true or isRunning == true) {
      CurrentDisplayString = F("    Motor is        running!    ");
    } else if(unexpectedError == 4 or unexpectedError == 5) {
      if(lcd_ticks < 1000) {
        CurrentDisplayString = F("   Error when       pushing!    ");
      } else {
        CurrentDisplayString = F("   (Press D to       unlock)    ");
      }
    } else if(unexpectedError == 6) {
      if(lcd_ticks < 1000) {
        CurrentDisplayString = F("Remove sample at   Holder 32!   ");;
      } else {
        CurrentDisplayString = F(" Press D to un- lock afterwards.");
      }
    } else if(unexpectedError == 7) {
      if(lcd_ticks < 1000) {
        CurrentDisplayString = F("Error: Could not eject sample!  ");
      } else {
        CurrentDisplayString = F("   (Press D to       unlock)    ");;
      }
    } else if(unexpectedError == 8) {
      if(lcd_ticks < 1000) {
        CurrentDisplayString = F("Error: Could notdetect sample in");
      } else {
        CurrentDisplayString =  String(F("Holder ")) + String(lastHolder) + "!" + String(F(" (D to unlock)  "));
      }
    } else if(unexpectedError == 9) {
      if(lcd_ticks < 1000) {
        CurrentDisplayString = F(" Unknown error   from outside!  ");
      } else {
        CurrentDisplayString =  F("   (Press D to  ");
        CurrentDisplayString += F("     unlock)    ");
      }
    } else if(errorcode == '2' and listening != 4) {
      CurrentDisplayString = F("Pusher is stuck!Please check... ");
    } else if(listening == 2) {
      CurrentDisplayString = F(" Press 1 to in- sert, 2 to eject");
    } else if(listening == 3) {
      CurrentDisplayString = F("Press * to run  a homing.       ");;
    } else if(listening == 4) {
      CurrentDisplayString =  String(F("RotorCalibration"));
      CurrentDisplayString += "St:" + String(calib_stepsize);
      for(i=0; i<(4-String(calib_stepsize).length()); i++) {
        CurrentDisplayString += " ";
      }
      CurrentDisplayString += "Pos:";
      CurrentDisplayString += String(myStepper.currentPosition());
    } else if(listening == 5) {
      CurrentDisplayString =  String(F("PusherServo cal."));
      CurrentDisplayString += "St:" + String(calib_stepsize);
      for(i=0; i<(4-String(calib_stepsize).length()); i++) {
        CurrentDisplayString += " ";
      }
      CurrentDisplayString += "Pos:";
      CurrentDisplayString += String(pusherServo.read());
    } else if(listening == 6) {
      CurrentDisplayString =  String(F("AirServo calibr."));
      CurrentDisplayString += "St:" + String(calib_stepsize);
      for(i=0; i<(4-String(calib_stepsize).length()); i++) {
        CurrentDisplayString += " ";
      }
      CurrentDisplayString += "Pos:";
      CurrentDisplayString += String(airServo.read());
    } else if(listening == 7) {
      CurrentDisplayString =  F("Press * to starta full test.    ");
    } else if(runMeasurement != -1) {
      // if that is not the case, but a measurement is running, display, which sample is currently measured
      CurrentDisplayString =  F("Measuring sample");
      if(runMeasurement != 0) {
        CurrentDisplayString += String(F("at Holder ")) + String(runMeasurement);
      }
    } else {
      //CurrentDisplayString = String(readIntFromEEPROM(0)) + "." + String(readIntFromEEPROM(100)) + "." + String(readIntFromEEPROM(110)) + "." 
      //   + String(readIntFromEEPROM(200)) + "." + String(readIntFromEEPROM(210));  // debugging calibration values
      CurrentDisplayString =  F(" NMR-Killer     ");
      CurrentDisplayString += F("                ");
      if(lcd_ticks == 0) {
        lcd.setCursor(1,1);
        lcd.print("/");
      } else if(lcd_ticks == 500) {
        lcd.setCursor(1,1);
        lcd.print("-");
      } else if(lcd_ticks == 1000) {
        lcd.setCursor(1,1);
        lcd.print("`");
      } else if(lcd_ticks == 1500) {
        lcd.setCursor(1,1);
        lcd.print("|");
      }
    }
    if(CurrentDisplayString != LastDisplayString) {
      // update LCD
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(CurrentDisplayString.substring(0,16));
      lcd.setCursor(0,1);
      lcd.print(CurrentDisplayString.substring(16,32));
      LastDisplayString = CurrentDisplayString;
    }
  }
  lcd_ticks++;
  if(lcd_ticks > 2000) {
    lcd_ticks = 0;
  }
}

void customWriteLCD(String message = "") {
  // writes a non-standard message to LCD (e.g. for debug purposes)
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(message.substring(0,16));
  lcd.setCursor(0,1);
  lcd.print(message.substring(16,32));
  LastDisplayString = message;
}
