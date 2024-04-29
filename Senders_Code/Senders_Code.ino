// Hexapod Remote Control Code
// Coded by 
// Kinley Penjor
// 2024

#include <SPI.h> 
#include "RF24.h" // establishing the radion connection and communication 
#include <Wire.h> // Include the Wire library for I2C communication
#include <Adafruit_GFX.h> // Include the Adafruit graphics library
#include <Adafruit_SSD1306.h> // Include the Adafruit SSD1306 OLED library
#include <Bounce2.h> //include the bounce2 library

#define OLED_WIDTH 128 // OLED display width,  in pixels
#define OLED_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// https://arduinogetstarted.com/tutorials/arduino-joystick
// Defining the pins for the Joystick1, Joystick2, Linearpotentiometer1, linerpotentiometer2
const int  joy1xpin = A1;
const int  joy1ypin = A0;
const int  joy2xpin = A2;
const int  joy2ypin = A3;
const int  linearPotentiometer1pin = A6;
const int  linearPotentiometer2pin = A7;

// Digital pin button
const int sw1pin = 3;
const int sw2pin = 2;

// Define OLED pins
const int oledSdaPin = A4; // Connect OLED SDA pin to A6
const int oledSclPin = A5; // Connect OLED SCL pin to A7

// Display timer disappear setup
unsigned long displayStartTime = 0; // Variable to store the start time of displaying message
const unsigned long displayDuration = 5000; // Display message duration in milliseconds
unsigned long displayscreenontime = 0; // variable to store the display data after each click

// Create instances of the Bounce class for each button
Bounce joy1button = Bounce();
Bounce joy2button = Bounce();

// Pin codings
RF24 radio(9, 10);
byte node_A_address[11] = "HexaRemote";
byte node_B_address[11] = "HexaPRobot";

// Counter initial value for the buttons 1
int gait_count = 0;
int prev_gait = 0;
int current_gait = -1;
bool gait_switch = true;

// Main setupFunction
void setup() {
  Serial.begin(9600);
  Serial.println("Starting Remote Control...");

  // Initialize OLED
  if(!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.openWritingPipe(node_B_address);
  radio.openReadingPipe(1, node_A_address);
  radio.startListening();

  custom_message_display("Establishing Radio Connection...");
  delay(2000);
  
  // For joystick  switch
  joy1button.attach(sw1pin, INPUT_PULLUP); // Joystick 1 button connected to pin 2
  joy1button.interval(5); // Debounce interval in ms
  
  joy2button.attach(sw2pin, INPUT_PULLUP); // Joystick 2 button connected to pin 3
  joy2button.interval(5); // Debounce interval in ms

  // Clear the display buffer.
  oled.clearDisplay();

  // Display the eyes while starting the screen
  display_screen();
}

// Main Looping function
void loop() {
  joy1button.update();
  joy2button.update();

  int joy1x = analogRead(joy1xpin);
  int joy1y = analogRead(joy1ypin);
  int joy1sw = digitalRead(sw1pin);
  int joy2x = analogRead(joy2xpin);
  int joy2y = analogRead(joy2ypin);
  int joy2sw = digitalRead(sw2pin);
  int liPo1 = analogRead(linearPotentiometer1pin);
  int liPo2 = analogRead(linearPotentiometer2pin);    

  // handle the buttong press events
  handleButtonActions();
  // set_gait(gait_count);
  
  // to clear the display output and to save some energy clear any screen output on time like while switching various gates or such
  if (millis() - displayStartTime >= displayDuration) {
    clear_oled_display();
  }

  // note that botht the joy1 and lipo 1 have reversed values, maximum place shows min values while coding make sure to encode that aswell
  sendData(gait_count, joy1x, joy1y, joy2x, joy2y, liPo1, liPo2);
}

// counter for joystick value increment and decrement the values
void handleButtonActions() {
  // Increment count if joystick 1 button is pressed and count is less than 5
  if (joy2button.fallingEdge() && gait_count <= 5) {
    gait_count++; 
    display_message();

  }

  // Decrement count if joystick 2 button is pressed and count is greater than 0
  if (joy1button.fallingEdge() && gait_count > 0) {
    gait_count--;  
    display_message();
  }

  if (gait_count > 5){
    gait_count = 0;
    prev_gait = 0;
    current_gait = -1;
    display_message();
  }
}

// Display values and changes on change upon changing the gaits
void display_message(){
  oled.clearDisplay(); // Clear the display buffer
  oled.setTextSize(3.5);          
  oled.setTextColor(WHITE);     
  oled.setCursor(0, 10);
  
  if (gait_count == 0){
    oled.println("Tripod");
  }
  else if (gait_count == 1){
    oled.println("Wave");
  }
  else if (gait_count == 2){
    oled.println("Ripple");
  }
  else if (gait_count == 3){
    oled.println("Bi");
  }
  else if (gait_count == 4){
    oled.println("Quad");
  }
  else if (gait_count == 5){
    oled.println("Hop");
  }
  else {
    oled.println("Out of Bound...");
  }
  oled.display();        
  displayStartTime = millis();  
}


// displaying the custom message that you want to display
void custom_message_display(String message){
  oled.clearDisplay(); // Clear the display buffer
  oled.setTextSize(1.5);          
  oled.setTextColor(WHITE);     
  oled.setCursor(0, 10);
  oled.println(message);
  oled.display();        
  displayStartTime = millis();
}

// clearing the old display screen
void clear_oled_display(){
  oled.clearDisplay(); // Clear the display buffer
  oled.display(); // Refresh the display to clear the content
}
// Start the Transferring of data form remote to the location destined to reach
void sendData(int gait_count, int joy1x, int joy1y, int joy2x, int joy2y, int liPo1, int liPo2) {
  radio.stopListening();

  // Prepare data packet
  char data_packet[48]; // Allocate space for data packet
  sprintf(data_packet, "%d,%d,%d,%d,%d,%d,%d",gait_count, joy1x, joy1y, joy2x, joy2y, liPo1, liPo2);

  // Send data packet
  if (!radio.write(&data_packet, sizeof(data_packet))) {
    Serial.println(F("failed to write message"));
  }

  radio.startListening();
}

// Robot eyes movement
void display_screen(){
  displayStartTime = millis();  
  // draw something
  oled.clearDisplay(); 
  oled.setTextSize(1.5);          // text size
  oled.setTextColor(WHITE);     // text color
  oled.setCursor(0, 10);        // position to display
  oled.println("Hello there..."); // text to display
  oled.display();        
  delay(1000);

  oled.clearDisplay();
  oled.setTextSize(1.5);        
  oled.setTextColor(WHITE);     
  oled.setCursor(0, 10);        
  oled.println("I am Your Hexapod!!"); 
  oled.display();        
  delay(2000);


  oled.clearDisplay();
  oled.drawCircle(30, 35, 20, WHITE);
  oled.drawCircle(90, 35, 20, WHITE);
  oled.display();
  delay(500);  

  oled.clearDisplay();
  oled.fillCircle(30, 35, 20, WHITE);
  oled.fillCircle(90, 35, 20, WHITE);
  oled.display();
  delay(100);

  oled.clearDisplay();
  oled.drawCircle(30, 25, 20, WHITE);
  oled.drawCircle(90, 25, 20, WHITE);
  oled.display();
  delay(500);   

  oled.clearDisplay();
  oled.drawCircle(30, 35, 20, WHITE);
  oled.drawCircle(90, 35, 20, WHITE);
  oled.display();
  delay(500);  
  
  oled.clearDisplay();
  oled.fillCircle(30, 35, 20, WHITE);
  oled.fillCircle(90, 35, 20, WHITE);
  oled.display();
  delay(100);

  oled.clearDisplay();
  oled.drawCircle(30, 35, 20, WHITE);
  oled.drawCircle(90, 35, 20, WHITE);
  oled.display();
  delay(500);
}

// // Radion chat test between the two nodes
// void sendReceiveMessage() {
//   radio.stopListening();

//   Serial.println(F("Now sending Hello to node B"));

//   char msg_to_B[20] = "Hello from node_A";

//   unsigned long start_time = micros();
//   if (!radio.write(&msg_to_B, sizeof(msg_to_B))) {
//     Serial.println(F("failed to write message"));
//   }

//   radio.startListening();

//   unsigned long started_waiting_at = micros();
//   boolean timeout = false;
//   while (!radio.available()) {
//     if (micros() - started_waiting_at > 2000000) {
//       timeout = true;
//       break;
//     }
//   }

//   if (timeout) {
//     Serial.println(F("Failed, response timed out."));
//   } 
//   else {
//     char msg_from_B[20];
//     radio.read(&msg_from_B, sizeof(msg_from_B));
//     unsigned long end_time = micros();
//     Serial.print(F("Sent '"));
//     Serial.print(msg_to_B);
//     Serial.print(F("', Got response '"));
//     Serial.print(msg_from_B);
//     Serial.print(F("', Round-trip delay "));
//     Serial.print(end_time - start_time);
//     Serial.println(F(" microseconds"));
//   }
// }
