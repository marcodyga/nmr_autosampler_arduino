/*

  Main Arduino program for the Autosampler.
 
*/

// AccelStepper library can be gotten here: http://www.airspayce.com/mikem/arduino/AccelStepper/index.html

#include <AccelStepper.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <Keypad_I2C.h>
#include <EEPROM.h>

// Calibration values are stored in Arduino's EEPROM (see the io.ino for details).
// After calibration of the motor, add here the position of Holder 1 !
int POS1;
// After calibration of the Pusher, add here the positions for start and end points.
short PUSHER_PUSH;
short PUSHER_PULL;
// After calibration of the Air servo, add there the positions of the servo for venting and for pushing the NMR tube.
short AIR_PUSH;
short AIR_VENT;

int input = -1;
bool runMotor = false;  // turn this on as soon as you want to start turning the rotor; in the main loop this is caught and the rotation is then initialised.
bool isRunning = false; // this variable determines if the rotation is finished. as soon as movement has started, the autosampler shouldn't do anything else until it is finished.
int runMeasurement = -1;
int i = 0;
int j = 0;
long l = 0;
int target = 0;
int cP = 0; // helper variable usually used for the current position, and if anything is calculated based on it.
int digit1 = -1;
int digit2 = -1;
bool wasStuck = false;
char errorcode = '0';
int unexpectedError = 0;
int blinkred = 0;
int blinkyellow = 100;
int blinkgreen = -100;
int blink_freq = 200;  // this is the frequency of the blinking of a LED, in loop cycles * 1/2.
int blinkbuzzer = 0;
int blink_freq_buzzer = 300; // frequency of buzzer blinking
int lastKey = -1;
short listening = 0; // this controls "listening" mode, used for manual control of the position, and for calibration.
int numpadKeysPressed[2] = {-1, -1};
int airStatus = 0;
int calib_stepsize = 1;
int send_next = 100; // amount of milliseconds until the next message with the errorcode is sent to the COM-port.
int lcd_ticks = 0;
short lastHolder = 0;

// Explanation: stepsize is the inverse "step size" (on the stepper driver board), multiplied by 4. 
// For example, if 1/16-steps are configured, this value should be 64 (or 128 if motor can do 400 steps per revolution).
// The number of steps per revolution is always 50*stepsize
int stepsize = 64; 

// Standard speed
int standardSpeed = 4*stepsize;
// speeds for acceleration modification
float maximumSpeed = 4*stepsize;
float acceleration = 3.5*stepsize;

// Speed of the transition from venting to blowing out the NMR tube. (lower is faster)
// Defined by: 
// Total step time: T; StepTime = S; Multiplier: M; Progress of the transition: P.
// => T = S + (M * P)/100
int airServoStepTime = 10;
int airServoMultiplier = 190;

//Pin definitions
const short stepPin = 6;
const short dirPin = 7;
const short enablePin = 8;
const short relaisAirPin = 12; // air valve
const short airDirPin = 5;  // for the control of air valve via stepper motor shield.
const short airStepPin = 4;
const short homingBridgePin = 9;
const short anschlagBridgePin = 10;
const short galgenPin = 11;
const short pusherPin = 2;
const short redLEDPin = A1;
const short yellowLEDPin = A0;
const short greenLEDPin = 13;
const short buzzerPin = A2;
const short airServoPin = 3; // the pressure regulating servo

AccelStepper myStepper = AccelStepper(AccelStepper::DRIVER, stepPin, dirPin);

// LCD display stuff
#define lcdAddress    0x27  // Define I2C Address for controller
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
String LastDisplayString = "";
String CurrentDisplayString = "";

// Keypad stuff
#define keypadAddress 0x20                          // I2C Address of the PCF8574 breakout board for the keypad. Can be set using the jumpers on the board, don't ask how.
const byte keypadRows = 4;
const byte keypadCols = 4;
char keypadLayout[keypadRows][keypadCols] = {
  {'1','2','3','A'}, 
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte keypadRowPins[keypadRows] = {7,6,5,4};
byte keypadColPins[keypadCols] = {3,2,1,0};
Keypad_I2C i2cKeypad(makeKeymap(keypadLayout), keypadRowPins, keypadColPins, keypadRows, keypadCols, keypadAddress); 
char currentKey = 'Z';

// Servo
Servo pusherServo;
Servo airServo;

// function prototypes BS
bool homing();
void gotoPos(short targetPos);
void calibration();
void customWriteLCD(String message);

void setup() {
  // get infos on calibration
  getCalFromEEPROM();
  
  // setup all pins
  // Servo
  pinMode(pusherPin,OUTPUT);
  pinMode(airServoPin, OUTPUT);
  pusherServo.write(PUSHER_PULL);
  pusherServo.attach(pusherPin);
  airServo.write(AIR_VENT);
  airServo.attach(airServoPin);
  // LCD
  lcd.begin (16,2);  // initialize the lcd
  // keypad
  i2cKeypad.begin();
  // Switch on the backlight
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);
  // Stepper
  myStepper.setEnablePin(enablePin);
  myStepper.setPinsInverted(false, false, true);
  myStepper.disableOutputs();
  myStepper.setMaxSpeed(maximumSpeed);
  myStepper.setSpeed(standardSpeed);
  myStepper.setAcceleration(acceleration);
  // Light barrier 1 (homing)
  pinMode(homingBridgePin,INPUT);
  // Light barrier 2 (Anschlag)
  pinMode(anschlagBridgePin,INPUT);
  // Light barrier 3 (Galgen)
  pinMode(galgenPin,INPUT);
  // Relais
  pinMode(relaisAirPin,OUTPUT);
  // LEDs
  pinMode(redLEDPin,OUTPUT);
  pinMode(yellowLEDPin,OUTPUT);
  pinMode(greenLEDPin,OUTPUT);
  writeLEDs(0,0,0);
  // buzzer
  pinMode(buzzerPin,OUTPUT);
  // deactivate Relais, to prevent overheating
  digitalWrite(relaisAirPin,HIGH);
  // For air valve control via motor shield
  pinMode(airDirPin, OUTPUT);
  pinMode(airStepPin, OUTPUT);
  digitalWrite(airDirPin, HIGH);
  digitalWrite(airStepPin, HIGH);
  
  Serial.begin(9600);
}

void loop() {
  // This part deals with input from the numpad.
  numpad_proc();
  // This part deals with input from the COM port.
  comport_proc();

  // This part controls the movement of the motor.
  if(runMotor) {
    isRunning = true;
    while(isRunning) {
      rotate();
    }
    delay(500); // delay so the rotor doesn't overshoot the target position due to inertia.
    runMotor = false;
    myStepper.disableOutputs();
  }

  // Return status information
  // Error codes:
  // 0 = everything is good
  // 1 = motor, pusher or air valve is currently moving
  // 2 = pusher is stuck, check the device
  // 3 = Measurement is running at the moment
  // 4 = Pusher did not properly open 
  // 5 = Pusher did not properly close
  // 6 = A NMR tube was detected in the spectrometer when trying to start the measurement. It should be removed from Holder 32.
  // 7 = When trying to return a sample, no NMR tube could be detected.
  // 8 = When trying to start a measurement, no sample could be detected.
  // 9 = An error was raised from outside (by the Autosampler table or by the user).
  errorcode = '0';
  if(runMeasurement != -1) {
    errorcode = '3';
  }
  if(runMotor == true) {
    errorcode = '1';
  }
  if(digitalRead(anschlagBridgePin) != 0) {
    errorcode = '2';
  }
  if(unexpectedError > 3) {
    String errorstring = String(unexpectedError);
    errorcode = char(errorstring[0]);
  }
  // send the error code to the COM port.
  if(send_next <= 0) {
    Serial.write(errorcode);
    send_next = 100;
  } else {
    send_next--;
  }

  // LED control
  // green LED on: ready to accept samples
  // yellow LED on: motor running
  // yellow and green LEDs blinking: calibration running
  // yellow LED blinking: measurement running
  // red LED on: Device is stuck
  // red LED blinking: unexpected error has occured
  if(errorcode == '2') {
    writeLEDs(1,0,0);
  } else if(unexpectedError > 3) {
    writeLEDs(2,0,0);
  } else if(runMotor == true) {
    writeLEDs(0,1,0);
  } else if(listening == 4 or listening == 5) {
    writeLEDs(0,2,2);
  } else if(errorcode == '3') {
    writeLEDs(0,2,1);
  } else {
    writeLEDs(0,0,1);
  }

  // Buzzer control
  if(errorcode == '2') {
    writeBuzzer(2);
  } else {
    writeBuzzer(0);
  }

  // LCD control
  writeLCD();

  delay(1);
}


void measureSample(int sample, bool inQueue=false) {
  /*
    Starts measurement of sample.
    Arguments:
    sample (int)   - the holder number of the sample to be measured
    inQueue (bool) - if the measurement is inside of a queue. If false, then holder 32
                     will be checked for a sample inside of the spectrometer, and a homing
                     will be performed; if true, it is assumed that the function is called 
                     inside of a queue, and that returnSample has been called directly 
                     before, so no homing and no check are necessary.
  */
  Serial.write("1");
  writeLEDs(0,1,0);
  // first check if a measurement is already running, if it does, don't start measuring a new one.
  if(runMeasurement == -1 and unexpectedError <= 3) {
    bool failed = false;
    bool removed = false;
    if(inQueue == false) {
      lastHolder = sample;
      failed = homing();
      if(failed == false) {
        // before starting the measurement, check if there is a NMR tube in the slot by blowing it to the top
        // of position 32 with pressurized air.
        gotoPos(32);
        while(runMotor == true) {
          loop();
        }
        Serial.write("1");
        writeLEDs(0,1,0);
        delay(500);
        removed = removeSample();
      }
    }
    if(removed == true) {
      unexpectedError = 6;
    } else if(failed == false) {
      gotoPos(sample);
      while(runMotor == true) {
        loop();
        Serial.write("1");
        writeLEDs(0,1,0);
      }
      delay(500);
      writeLEDs(0,2,0);
      bool inserted = insertSample();
      if(inserted) {
        runMeasurement = sample;
      }
    }
  }
}

bool insertSample() {
  // Insert a sample. Returns true if sample was inserted, false if no sample was detected.
  setAirHard(true);
  setPusher(true);
  delay(1000);
  // now check if the sample is actually there and if it can be lifted to the top with the compressed air.
  bool removed = galgenCheck();
  // if the sample could not be detected by the light bridge, cancel the stuff and return error 8.
  if(removed == false) {
    setAirHard(false);
    setPusher(false);
    unexpectedError = 8;
    return false;
  } else {
    // now gently insert the sample into the spectrometer.
    setAir(false);
    // use the pusher to check if the sample is stuck. If it is, the setPusher function will raise an unexpected Error.
    delay(500);
    setPusher(false);
    delay(1000);
    if(unexpectedError == 0) {
      return true;
    }
  }
}

void returnSample(int sample) {
  bool failed = true;
  bool removed = false;
  Serial.write("1");
  writeLEDs(0,1,0);
  if(unexpectedError <= 3) {
    setPusher(false);
    failed = homing();
    if(failed==false) {
      gotoPos(sample);
      if(cP != target) { // cP and target are set in the gotoPos() function.
        setPusher(false);
        while(runMotor == true) {
          loop();
        }
        Serial.write("1");
        writeLEDs(0,1,0);
        delay(500);
      }
      writeLEDs(0,2,0);
      removed = removeSample();
      // if there is no sample which could be removed, raise error 7.
      if(removed == false) {
        unexpectedError = 7;
      }
      runMeasurement = -1;
    }
  }
}

bool removeSample() {
  // helper function to remove a sample and check if it correctly gets removed.
  bool removed = false;
  setPusher(true);
  if(unexpectedError == 0) {
    delay(100);
    setAir(true);
    delay(800);
    removed = galgenCheck();
    setPusher(false);
    delay(200);
    setAirHard(false);
  }
  return removed;
}

bool galgenCheck() {
  // helper function to check if a sample is removed from the spectrometer.
  bool removed = false;
  int top = 0;
  int bottom = 0;
  int timeout = 1000;
  while (timeout > 0) {
    timeout--;
    delay(1);
    if(digitalRead(galgenPin) == 0) {
      bottom++;
    } else {
      top++;
    }
  }
  if(top > 600) {
    removed = true;
  }
  return removed;
}

void test() {
  for(int n = 1; n <= 25; n++) {
    /*int pos = n*8;
    while(pos>32) {
      pos -= 31;
    }*/
    int pos = 31;
    if(n%4 == 0) {
      pos = 1;
    } else if(n%4 == 1) {
      pos = 9;
    } else if(n%4 == 2) {
      pos = 17;
    } else if(n%4 == 3) {
      pos = 25;
    }
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Testing...      ");
    lcd.setCursor(0,1);
    lcd.print("n="+String(n)+"; pos="+String(pos));
    measureSample(pos);
    //delay(2000);
    returnSample(pos);
    delay(2000);
    if(unexpectedError > 3) {
      break;
    }
  }
}
