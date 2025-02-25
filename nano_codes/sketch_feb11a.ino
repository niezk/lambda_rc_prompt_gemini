#include "LiquidCrystal_I2C.h"

// Motor control pins
#define MOTORA1 2
#define MOTORB1 3
#define MOTORA2 4
#define MOTORB2 5
#define MOTORA3 6
#define MOTORB3 7
#define MOTORA4 8
#define MOTORB4 9


LiquidCrystal_I2C lcd(0x27, 16, 2);

void smile() {
lcd.clear();

byte image06[8] = {B11111, B00000, B00000, B11111, B11111, B11111, B11111, B11111};
byte image11[8] = {B11111, B00000, B00000, B11111, B11111, B11111, B11111, B11111};
byte image23[8] = {B00000, B00000, B00000, B00000, B11111, B01111, B00011, B00000};
byte image24[8] = {B00000, B00000, B00000, B00000, B11111, B11111, B11111, B00000};
byte image25[8] = {B00000, B00000, B00000, B00000, B11111, B11111, B11111, B00000};
byte image26[8] = {B00000, B00000, B00000, B00000, B11111, B11110, B11000, B00000};


lcd.createChar(0, image06);
lcd.createChar(1, image11);
lcd.createChar(2, image23);
lcd.createChar(3, image24);
lcd.createChar(4, image25);
lcd.createChar(5, image26);


lcd.setCursor(5, 0);
lcd.write(byte(0));
lcd.setCursor(10, 0);
lcd.write(byte(1));
lcd.setCursor(6, 1);
lcd.write(byte(2));
lcd.setCursor(7, 1);
lcd.write(byte(3));
lcd.setCursor(8, 1);
lcd.write(byte(4));
lcd.setCursor(9, 1);
lcd.write(byte(5));

}



void setup() {
  // Serial.begin(9600); // Initialize serial communication at 9600 baud rate for debugging
  Serial.begin(115200); // Initialize SoftwareSerial communication at 115200 baud rate for commands
  // Set motor control pins as outputs
  for (int i = 2; i <= 9; i++) {
    pinMode(i, OUTPUT);
  }

  // Set IR sensor pins as inputs
  pinMode(RIGHT, INPUT);
  pinMode(LEFT, INPUT);

  // LCD INITIALIZAITON
  lcd.init();
  lcd.backlight();
  lcd.clear();
  delay(4000);
  smile();
}

void loop() {

    if (Serial.available() > 0) {
      String command = Serial.readStringUntil('\n');
      Serial.print(command);
      command.trim();
      ParseAndExecuteCommand(command);
    }

}


void ParseAndExecuteCommand(String command) {
  // Split the command string by commas to handle multiple commands
  int commaIndex = command.indexOf(',');
  while (commaIndex != -1) {
    String subCommand = command.substring(0, commaIndex);
    subCommand.trim();  // Trim the subCommand in place
    ExecuteSingleCommand(subCommand);
    command = command.substring(commaIndex + 1);
    command.trim();  // Trim the remaining command string in place
    commaIndex = command.indexOf(',');
  }
  // Execute the last or single command
  command.trim();  // Trim the last command in place
  ExecuteSingleCommand(command);
}

void ExecuteSingleCommand(String command) {
  if (command == "forward") {
    MoveForward();
    delay(1000);
    Stop();
  } else if (command == "left") {
    Stop();
    delay(200);
    TurnLeft();
    delay(1000);
    Stop();
  } else if (command == "right") {
    Stop();
    delay(200);
    TurnRight();
    delay(1000);
    Stop();
  } else if (command == "stop") {
    delay(1000);
    Stop();
  } else if (command == "backward") {
    MoveBackward();
    delay(1000);
    Stop();
  } else if (command == "follow") {
    HumanFollowMode = true;
  } else if (command == "manual") {
    HumanFollowMode = false;
  } else {
    Serial.println("Unknown command: " + command);
  }
}


void MoveBackward() {
  // Implement motor control logic to move forward

  digitalWrite(MOTORA1, HIGH);
  digitalWrite(MOTORB1, LOW);
  digitalWrite(MOTORA2, HIGH);
  digitalWrite(MOTORB2, LOW);
  digitalWrite(MOTORA3, HIGH);
  digitalWrite(MOTORB3, LOW);
  digitalWrite(MOTORA4, HIGH);
  digitalWrite(MOTORB4, LOW);
}


void MoveForward() {
  // Implement motor control logic to move forward
  digitalWrite(MOTORA1, LOW);
  digitalWrite(MOTORB1, HIGH);
  digitalWrite(MOTORA2, LOW);
  digitalWrite(MOTORB2, HIGH);
  digitalWrite(MOTORA3, LOW);
  digitalWrite(MOTORB3, HIGH);
  digitalWrite(MOTORA4, LOW);
  digitalWrite(MOTORB4, HIGH);
}
void TurnLeft() {
  // Implement motor control logic to turn left
  
  digitalWrite(MOTORA1, LOW);
  digitalWrite(MOTORB1, HIGH);
  digitalWrite(MOTORA2, HIGH);
  digitalWrite(MOTORB2, LOW);
  digitalWrite(MOTORA3, LOW);
  digitalWrite(MOTORB3, HIGH);
  digitalWrite(MOTORA4, HIGH);
  digitalWrite(MOTORB4, LOW);
}

void TurnRight() {
  // Implement motor control logic to turn right
  
  digitalWrite(MOTORA1, HIGH);
  digitalWrite(MOTORB1, LOW);
  digitalWrite(MOTORA2, LOW);
  digitalWrite(MOTORB2, HIGH);
  digitalWrite(MOTORA3, HIGH);
  digitalWrite(MOTORB3, LOW);
  digitalWrite(MOTORA4, LOW);
  digitalWrite(MOTORB4, HIGH);
}

void Stop() {
  // Implement motor control logic to stop
  
  digitalWrite(MOTORA1, LOW);
  digitalWrite(MOTORB1, LOW);
  digitalWrite(MOTORA2, LOW);
  digitalWrite(MOTORB2, LOW);
  digitalWrite(MOTORA3, LOW);
  digitalWrite(MOTORB3, LOW);
  digitalWrite(MOTORA4, LOW);
  digitalWrite(MOTORB4, LOW);
}
