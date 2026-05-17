#include <SPI.h>
#include <SD.h>
#include <BMP388_DEV.h>                           // Include the BMP388_DEV.h library
#include <Timer.h>
#include <MemoryUsage.h>

#define LM_PIN 3                                 // Declaring a pin for triggering flight Mode
#define ST_PIN 4                                 // Declaring a pin to use for status LED
#define EJ_PIN 5                                 // Declaring a pin for signaling to the ejection charge. When it goes HIGH, charge fires.

#define PRE_LAUNCH 6                             // Defining some values to use for the flightMode variable.
#define ON_GROUND 7                             
#define ASCENDING 8
#define DESCENDING 9

float temperature, pressure, altitude;            // Create the temperature, pressure and altitude variables.
BMP388_DEV bmp388;                                // Object for the bmp388 temp, pressure, and altitude sensor.
File myFile;                                      // *** Considering making these variables local where possible *** 
const int chipSelect = 10;                        // I2C chip ID for SD card module.
bool onGround, apogeeReached = false;             // bools for whether the rocket is on the ground, or has reached apogee.
float currentAlt, prevAlt1, prevAlt2, prevAlt3, diff, startingAlt, apogeeAlt, apogeeTime = 0;   // Bunch of floats for various things.
int numMeasurements, buttonState = 0;             // int for number of measurements for detectApogee function, and buttonState for the state of the push button.
int flightMode = PRE_LAUNCH;                       //flightMode is the status of the vehicle, using the previously defined values.
Timer timer;                                      // Timer object to record flight time, time of apogee etc.


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
}

  //Serial.print("Initializing SD card...");

  if (!SD.begin(chipSelect)) {
    //Serial.println("initialization failed!");
    while (1);
  }
  pinMode(EJ_PIN, OUTPUT);                        // Initialize ejection charge pin, and set it to low.
  digitalWrite(EJ_PIN, LOW);
  pinMode(ST_PIN, OUTPUT);                        // Initialize status LED pin, and set it to low.
  digitalWrite(ST_PIN, LOW);
  pinMode(LM_PIN, INPUT);                      
  bmp388.begin();                                 // Default initialisation, place the BMP388 into SLEEP_MODE 
  bmp388.setTimeStandby(TIME_STANDBY_80MS);       // Set the standby time to 80ms
  bmp388.startNormalConversion();                 // Start BMP388 continuous conversion in NORMAL_MODE 
  //Serial.println("\ninitialization done.");

  //Serial.println("\nAll systems go. Press button to enter launch mode.");
  digitalWrite(ST_PIN, HIGH);   //Blink LED twice to indicate successful initialization
  delay(250);
  digitalWrite(ST_PIN, LOW);
  delay(250);
  digitalWrite(ST_PIN, HIGH);
  delay(250);
  digitalWrite(ST_PIN, LOW);
  delay(250);
  delay(100);
}

void loop() {
  buttonState = digitalRead(LM_PIN);
  if(digitalRead(LM_PIN) == HIGH && flightMode == PRE_LAUNCH)
  {
    //Serial.print("\nEntering launch mode. Beginning telemetry.");
    if(SD.exists("data.csv")) {
      SD.remove("data.csv");        //Delete prev flight data to make it so only most recent flight data is stored
      //Serial.println("Deleted previous flight data.");
    }
    myFile = SD.open("data.csv", FILE_WRITE);
    if (myFile) {
      myFile.println("Begin flight data.");
      myFile.println("Alt (m),Time (s)");
    }
    else {
    //Serial.println("error opening data.csv");
    }

    //flightStatus = "Entering launch mode";
    flightMode = ON_GROUND;               // Update flightMode to PRE_LAUNCH state.
    numMeasurements = 0;                  // Reset numMeasurements in case this is not the first flight since the computer has been powered on.
    digitalWrite(ST_PIN, HIGH);   //Flash LED 3 times to indicate beginning of flight, and keep it steady on during flight
    delay(1000);
    digitalWrite(ST_PIN, LOW);
    delay(1000);
    digitalWrite(ST_PIN, HIGH);
    delay(1000);
    digitalWrite(ST_PIN, LOW);
    delay(1000);
    digitalWrite(ST_PIN, HIGH);
    delay(1000);
    digitalWrite(ST_PIN, LOW);
    delay(1000);
    digitalWrite(ST_PIN, HIGH);
    timer.start();                // Start the timer.
    if (bmp388.getMeasurements(temperature, pressure, altitude)){           //Taking preflight measurements. Probably add temp and stuff later.
      startingAlt = altitude;
      altitude = altitude - startingAlt;
    }    
}

  if(flightMode != PRE_LAUNCH) {             // As long as we're not in PRE_LAUNCH mode, take measurements with the BMP388.
      if (bmp388.getMeasurements(temperature, pressure, altitude)) {  // Check if the measurement is complete
        altitude = altitude - startingAlt;
        detectApogee();  
      }

      if (myFile) {
        myFile.print(altitude);
        myFile.print(",");
        myFile.println(timer.read()/1000.0);
        //Serial.println("Wrote altitude to SD card."); 
      }

      if((apogeeReached && onGround && flightMode == DESCENDING) || (digitalRead(LM_PIN) == HIGH && flightMode != PRE_LAUNCH)) {  // Seeing if we landed after a flight. If so, return to preflight state.                                                                                                                     
        flightMode = PRE_LAUNCH;                                                                                                  // The second condition checks to see if the button has been pressed to cancel the flight.
        //Serial.print("\nFlight has landed!");
        apogeeReached = false;
        digitalWrite(ST_PIN, LOW);
        digitalWrite(EJ_PIN, LOW);
        myFile.println("Flight ended.");
        timer.stop();
        myFile.print("Time of flight (s):,");       // Print some data to the .csv file on the sdcard.
        myFile.println(timer.read()/1000.0);
        myFile.print("Apogee (m):,");
        myFile.println(apogeeAlt);
        myFile.print("Time of apogee (s):,");
        myFile.println(apogeeTime);
        myFile.close();
        delay(500);
      }
    }
  delay(80);                                    // Delay to sync up with BMP388 delay time.
}


void detectApogee(){
  prevAlt3 = prevAlt2;                          // Update last 4 altitude measurements. Used for apogee detection.
  prevAlt2 = prevAlt1;
  prevAlt1 = currentAlt;
  currentAlt = altitude;
  diff = 0;
  numMeasurements++;
  if(numMeasurements < 4) return;               // Need to not update flight status until we get at least 4 altitude measurements.

  diff += currentAlt - prevAlt1;
  diff += prevAlt1 - prevAlt2;
  diff += prevAlt2 - prevAlt3;
  diff = diff/3;                               // Take average of difference between the last 4 altitude measurements to find out what's going on with the vehicle.
  if(diff < -0.25) {
    onGround = false;
    //flightStatus = "Descending";
    digitalWrite(EJ_PIN, LOW);
    //apogeeReached = true;
    flightMode = DESCENDING;                          // Not sure if this is necessary
  }
  else if (diff > 0.25) {
    onGround = false;
    //flightStatus = "Ascending";
    digitalWrite(EJ_PIN, LOW);
    flightMode = ASCENDING;
  }
  else if (diff > -0.1 && diff < 0.1) {           // Apogee detection, and landing detection in the else if.
    if(flightMode == ASCENDING) {
      apogeeReached = true;               //Detecting apogee if average difference between last 4 altitude measurements is between -0.1m and 0.1m.
      //flightStatus = "At apogee";
      digitalWrite(EJ_PIN, HIGH);         //Fires ejection charge.
      //myFile.print(",Apogee reached and ejection charge fired.\n");
      //Serial.println("\nApogee reached, and ejection charge fired.");
      if(currentAlt > apogeeAlt){
        apogeeAlt = currentAlt;           // Getting apogee measurement.
        apogeeTime = timer.read()/1000.0;
      }
    }
    else {       // If the vehicle stops moving after it's been descending, then it has landed.
      //flightStatus = "onGround";
      onGround = true;
      digitalWrite(EJ_PIN, LOW);
    }
  }
}
