// Control of the Motor of the Rotor.

void rotate() {
  /* 
    This function is responsible for turning the rotor, it should be called
    repeatedly by the main loop, or anytime movement of the rotor is required.
    Rotor movement should ONLY be done via this function, as we have all the
    safety checks in here!
    Control where to move by setting the variable "target".
    As a side effect, sets the "cP" global variable to the current position.
  */
  if(digitalRead(anschlagBridgePin) == 0 and unexpectedError <= 3) {
    // check for Anschlag, so it can't turn if the pusher is not in the right position.
    // if an unexpected error has occured, the motor should not move either.
    if(wasStuck == false) {
      // only run the motor if it wasn't stuck before.
      if(pusherServo.read() == PUSHER_PULL) {
        // check here for Pusher, if it is extended, the motor shouldn't move.
        // movement function should be blocking !
        cP = myStepper.currentPosition();
        myStepper.enableOutputs();
        myStepper.moveTo(target);
        writeLEDs(0,1,0);
        writeLCD();
        isRunning = myStepper.run();
        // send the error code to the COM port.
        if(send_next <= 0) {
          Serial.write('1');
          send_next = 100;
        } else {
          send_next--;
        }
      } else {
        myStepper.disableOutputs();
      }
    } else {
      // if the motor was stuck before, then just reset everything
      runMotor = false;
      wasStuck = false;
      myStepper.disableOutputs();
    }
  } else {
    wasStuck = true;
    myStepper.disableOutputs();
  }
}

bool homing() {
  bool failed = false;
  writeLEDs(0,1,0);
  buzz(100);
  delay(100);
  buzz(180);
  delay(2000);
  setPusher(false);
  // determine in which direction the homing should start.
  target = 0;
  checkDirection();
  i = 0;
  // to start turning, target and current position must be apart a certain amount of steps.
  // let's give it 100*stepsize which equals 2 full revolution, this should be more than enough
  // for a homing.
  myStepper.enableOutputs();
  if(abs(target - myStepper.currentPosition()) < 100*stepsize) {
    if((target - myStepper.currentPosition()) > 0) {
      target = myStepper.currentPosition() + 100*stepsize;
    } else {
      target = myStepper.currentPosition() - 100*stepsize;
    }
  }
  int point1 = 0;
  int point2 = 0;
  bool foundPoint1 = false;
  
  if(digitalRead(homingBridgePin) != 0) {
    // case if we start at the position where the flag is inside of the homing bridge
    point1 = myStepper.currentPosition();
    point2 = myStepper.currentPosition();
  } else {
    // if not, lets run at full speed till we find the flag
    do {
      if(digitalRead(anschlagBridgePin) == 0 and unexpectedError <= 3) {
        rotate();
        i = digitalRead(homingBridgePin);
        // a rough imprecise "pre-measurement"
        if(!foundPoint1) {
          if(i != 0) {
            point1 = myStepper.currentPosition();
            foundPoint1 = true;
          }
        } else {
          if(i == 0) {
            point2 = myStepper.currentPosition();
            break;
          }
        }
      } else {
        delay(500);
        myStepper.disableOutputs();
        failed = true;
        break;
      }
    } while(isRunning);
  }
  if(!failed) {
    // go back to the position where the homing bridge's signal was first detected
    target = (point1 + point2) / 2;
    do {
      rotate();
    } while(isRunning);
    // precise actual measurement is now
    // check if we can see the flag
    if(digitalRead(homingBridgePin) != 0) {
      // lets go
      // slow down the rotor for this operation
      myStepper.setSpeed(0.6*stepsize);
      cP = myStepper.currentPosition();
      // turn towards positive position till we are clear of the flag
      do {
        myStepper.runSpeed();
        i = digitalRead(homingBridgePin);
      } while(i!=0 or abs(myStepper.currentPosition()-cP)<stepsize/4);
      point1 = myStepper.currentPosition();
      // go back to middle position
      do {
        rotate();
      } while(isRunning);
      // turn towards negative position till we are clear of the flag
      delay(200);
      myStepper.setSpeed(-0.6*stepsize);
      cP = myStepper.currentPosition();
      do {
        myStepper.runSpeed();
        i = digitalRead(homingBridgePin);
      } while(i!=0 or abs(myStepper.currentPosition()-cP)<stepsize/4);
      point2 = myStepper.currentPosition();
      // the zero-point position should be the middle between point 2 and point 1, however, currently we are at point2
      // thus, we set the current position as half the distance between point 1 and point 2
      myStepper.setCurrentPosition((point2-point1)/2);
      // reset speed.
      myStepper.setSpeed(standardSpeed);
      myStepper.disableOutputs();
    } else {
      // the flag is not at the promised position, this should never really happen
      // if you run into this error consider slowing down the rotor
      failed = true;
      buzz(2000);
    }
  }
  return failed;
}

void gotoPos(short targetPos) {
  // Position formula: Pos. 1 is #POS1
  // so POS1-c*(t-1) will give the target value.
  // with c being stepsize * 50 / number_of_positions
  target = POS1 - (stepsize*50/32)*(targetPos-1);
  // check if it is faster to go the other way around
  checkDirection();
  runMotor = true;
}

void checkDirection() {
  // Helper function which will move the current position by one rotation foward or backward,
  // if it will bring the position in global variable "target" closer to the current position.
  // In effect, this will change the movement direction of the rotor, if it is faster to go
  // the other way around.
  int altTarget = target - 50*stepsize;
  cP = myStepper.currentPosition();
  if(abs(altTarget-cP) < abs(target-cP)) {
    myStepper.setCurrentPosition(cP + 50*stepsize);
  }
  altTarget = target + 50*stepsize;
  if(abs(altTarget-cP) < abs(target-cP)) {
    myStepper.setCurrentPosition(cP - 50*stepsize);
  }
}

void calibration() {
  // In calibration, you should try to get to the position #1. The position number displayed
  // on the screen must then be added into the main code of the autosampler into the 
  // variable "POS1".
  bool failed = homing();
  listening = 4;
}
