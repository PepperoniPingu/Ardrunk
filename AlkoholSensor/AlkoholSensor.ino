// Ardrunk alcohol meter
//
// Can display promille in blood (BAC) or ppm in air
//
// Using HS135 sensor and SSD1306 OLED display
//
// PepperoniPingu 2022


#include <Wire.h> // Library for the I2C protocol. Used for the display
#include <Adafruit_SSD1306.h> // OLED screen library
#include <Adafruit_GFX.h> // Graphics library

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define SCREEN_ADDRESS 0x3C // I2C address of the OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire); // Define the display

#define HS135Pin A3 // Sensor pin
#define btn1Pin 3 // Button 1 pin
#define btn2Pin 2 // Button 2 pin

#define e 2.7182818284

#define programStates 2 // Number of states the program can be in, like different apps
enum ProgramState { BACPromi, PPM }; // The different states/apps
ProgramState programState = BACPromi; // Starting state

float sensorValue;  // Variable to store sensor value
float tareOffset; // If tared, the value to disregard is stored here
bool tareState; // If tared or not

unsigned long lastReading; // Time when sensor was read last. Used to make a delay between each measurement
unsigned long lastButtonPress; // Time when a button was pressed last. Used to debounce the buttons

bool pressedButtons [2]; // The states of the buttons
bool lastPressedButtons[2]; // The last states of the buttons. Used to detect when the button state has changed. 


void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS); // Initialize the screen

  // Set up the buttons as inputs with pullups
  pinMode(btn1Pin, INPUT_PULLUP); 
  pinMode(btn2Pin, INPUT_PULLUP);

  // Display that the sensor is warming up
  display.clearDisplay();
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(2,22);             // Start at top-left corner
  display.println("Warming up"); // Print to the screen buffer
  display.display(); // Display the screen buffer
  
  delay(5000); // Allow the MQ3 to warm up
}


// Main loop
void loop() {
  readButtons(); // Update the button states

  changeProgramState(); // Change the program state depending on the button states

  // Decide which state/app to run
  switch(programState) {
    case BACPromi:
      BACPromiLoop();
      break;

    case PPM:
      PPMLoop();
      break;

    default:
      BACPromiLoop();
      break;
  }
}


// Routine for displaying BAC
void BACPromiLoop() {

  // Read the sensor every 500 ms
  if(millis() - lastReading > 500) {
    sensorValue = analogRead(HS135Pin); // Read sensor
    
    sensorValue = (sensorValue/1023)*5; // Convert value to voltage
    sensorValue = voltageToPPM(sensorValue); // Convert voltage to ppm
    sensorValue = PPMToBACPromi(sensorValue); // Convert to BAC
    
    lastReading = millis(); // Save when last reading was done
    }

  // Tare or un-tare if button 2 was pressed
  if(pressedButtons[1] == HIGH && lastPressedButtons[1] != pressedButtons[1]) {
    tareState = !tareState;
    if(tareState) {
      tareOffset = sensorValue;
    } else tareOffset = 0;
  }
  
  display.clearDisplay(); // Clear the display buffer

  display.setTextSize(3);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.print(sensorValue - tareOffset > 0? sensorValue - tareOffset : 0); // Print the BAC. If less than 1, print 0
  display.setTextSize(2);  
  display.setCursor(100, 10); // Print the unit
  display.println("%%");

  display.display();
}


// Routine for displaying ppm in the air
void PPMLoop() {

  // Read the sensor every 500 ms
  if(millis() - lastReading > 500) {
    sensorValue = analogRead(HS135Pin); // Read sensor
    
    sensorValue = (sensorValue/1023)*5; // Convert value to voltage
    sensorValue = voltageToPPM(sensorValue); // Convert voltage to ppm
    
    lastReading = millis(); // Save when last reading was done
    }

  // Tare or un-tare if button 2 was pressed
  if(pressedButtons[1] == HIGH && lastPressedButtons[1] != pressedButtons[1]) {
    tareState = !tareState;
    if(tareState) {
      tareOffset = sensorValue;
    } else tareOffset = 0;
  }
  
  display.clearDisplay(); // Clear the display buffer

  display.setTextSize(4);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.print(round(sensorValue - tareOffset) > 0? round(sensorValue - tareOffset) : 0); // Print the ppm. If less than 1, print 0
  display.setTextSize(2);  
  display.setCursor(90, 14);
  display.println("ppm"); // Print the unit

  display.display();
}



// Function for reading and debouncing buttons
void readButtons() {

  // Save last button states
  lastPressedButtons[0] = pressedButtons[0];
  lastPressedButtons[1] = pressedButtons[1];

  // Read buttons
  if (millis() - lastButtonPress > 20) {
    if(digitalRead(btn1Pin) == LOW) {
      pressedButtons[0] = HIGH;
      lastButtonPress = millis();
    } else pressedButtons[0] = LOW;
    if(digitalRead(btn2Pin) == LOW) {
      pressedButtons[1] = HIGH;
      lastButtonPress = millis();
    } else pressedButtons[1] = LOW;
  }
}

// Function for switching program state/app
void changeProgramState() {
  if(pressedButtons[0] == HIGH  && pressedButtons[0] != lastPressedButtons[0]) {
    if(programState < programStates - 1) {
      programState = programState + 1;
    } else {
      programState = 0;
    }

    tareOffset = 0;
  }
}


// Convert voltage between 0v-5v to PPM in the air
float voltageToPPM(float volts) {
  float PPM = -62517.681/(1+(-233.21972)*pow(e, -1.16923373*volts)); // Function decided through doing regression on the sensors (HS135) datasheet. 
  return PPM;
}

// Convert PPM in the air to promille in the blood
float PPMToBACPromi(float PPM) { 
  float BACPromi = PPM / 21; // For every 21 promille in the blood there is 1 ppm in the breath. Standard value agreed upon. Actual  
  return BACPromi;
}
