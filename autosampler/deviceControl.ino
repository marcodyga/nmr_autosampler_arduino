// Control of the additional parts of the device
// (Pusher and air valves)

void setPusher(bool push) {
  int starting_status = pusherServo.read();
  bool pushed = false;
  writeLEDs(0,1,0);
  if(push == true) {
    if(starting_status != PUSHER_PUSH) {
      pusherServo.write(PUSHER_PUSH);
      pushed = true;
    }
  } else {
    if(starting_status != PUSHER_PULL) {
      pusherServo.write(PUSHER_PULL);
      pushed = true;
    }
  }
  if(pushed == true) {
    // check here if the Anschlag pin registers that the pusher goes through...
    // if it doesn't we will create unexpected error #4 (if it didn't open properly)
    // or error #5 (if it didn't close properly).
    l=0;
    while(digitalRead(anschlagBridgePin) == 0) {
      delayMicroseconds(1);
      if(l++ > 500000) {
        writeLEDs(2,0,0);
        if(push == false) {
          unexpectedError = 5;
        } else {
          unexpectedError = 4;
        }
        buzz(1000);
        break;
      }
    }
    l=0;
    while(digitalRead(anschlagBridgePin) == 1) {
      delayMicroseconds(1);
      if(l++ > 500000) {
        writeLEDs(2,0,0);
        if(push == false) {
          unexpectedError = 5;
        } else {
          unexpectedError = 4;
        }
        buzz(1000);
        break;
      }
    }
    if(starting_status == PUSHER_PUSH and (unexpectedError == 4 or unexpectedError == 5)) {
      pusherServo.write(PUSHER_PUSH);
    }
    pushed = false;
  }
  
}

void setAir(bool air) {
  // Slowly opens or closes air.
  int additionalTime = 0;
  writeLEDs(0,1,0);
  if(air == true) {
    // Open quickly at first, then decrease the rate.
    airServo.write(AIR_VENT);
    delay(200);
    digitalWrite(relaisAirPin,LOW);
    if(AIR_VENT > AIR_PUSH) {
      for(j=AIR_VENT; j>=AIR_PUSH; j--) {
        airServo.write(j);
        additionalTime = AIR_VENT - j;
        delay(airServoStepTime+(airServoMultiplier*additionalTime)/100);
      }
    } else {
      for(j=AIR_VENT; j<=AIR_PUSH; j++) {
        airServo.write(j);
        additionalTime = j - AIR_VENT;
        delay(airServoStepTime+(airServoMultiplier*additionalTime)/100);
      }
    }
    delay(1000);
    airStatus = 1;
  } else {
    // Close slowly at first, then increase the rate.
    airServo.write(AIR_PUSH);
    delay(200);
    if(AIR_VENT > AIR_PUSH) {
      for(j=AIR_PUSH; j<=AIR_VENT; j++) {
        airServo.write(j);
        additionalTime = AIR_VENT - j;
        delay(airServoStepTime+(airServoMultiplier*additionalTime)/100);
      }
    } else {
      for(j=AIR_PUSH; j>=AIR_VENT; j--) {
        airServo.write(j);
        additionalTime = j - AIR_VENT;
        delay(airServoStepTime+(airServoMultiplier*additionalTime)/100);
      }
    }
    digitalWrite(relaisAirPin,HIGH);
    airStatus = 0;
    delay(800);
  }
}

void setAirHard(bool air) {
  writeLEDs(0,1,0);
  if(air == true) {
    airServo.write(AIR_PUSH);
    digitalWrite(relaisAirPin,LOW);
    airStatus = 1;
  } else {
    airServo.write(AIR_VENT);
    digitalWrite(relaisAirPin,HIGH);
    airStatus = 0;
  }
}

void pusherCalibration() {
  listening = 5;
}

void airCalibration() {
  listening = 6;
}
