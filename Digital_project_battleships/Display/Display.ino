#include <LedControl.h>
#include <pitches.h>
// Pins for MAX7219
#define DIN_PIN 12
#define CS_PIN 11
#define CLK_PIN 10

#define CURSOR_MOVE_NOTE NOTE_C4
#define SAVE_STATE_NOTE NOTE_G4
#define SHIP_FOUND_NOTE NOTE_A4
#define NO_SHIP_FOUND_NOTE NOTE_E4

// Pins for push buttons
const int leftButton = 2;
const int rightButton = 3;
const int upButton = 4;
const int downButton = 5;
const int selectButton = 6;
const int sendButton = 7;
// Piezo buzzer pin
const int buzzerPin = 8;
// Number of LED matrices connected in series
#define NUM_DEVICES 1

// Create a LedControl object
LedControl lc = LedControl(DIN_PIN, CLK_PIN, CS_PIN, NUM_DEVICES);

// Dimensions of the LED matrix
const int numRows = 8;
const int numCols = 8;

// 2D array to store the state of each LED (0 for off, 1 for on)
bool ledState[numRows][numCols] = {0};
bool hiddenShips[numRows][numCols] = {0};
bool revealedState[numRows][numCols] = {0};

bool drawingPhase = true;
bool attackPhase = false;

bool win = false;
// Current position of the cursor
int currentRow = 4;
int currentCol = 4;
bool cursorVisible = false;  // Flag to toggle cursor visibility
unsigned long lastBlinkMillis = 0;  // Variable to store the last time cursor blinked

void setup() {
  Serial.begin(9600);

  // Initialize the MAX7219
  lc.shutdown(0, false);       // Wake up display
  lc.setIntensity(0, 8);       // Set brightness level (0 is min, 15 is max)
  lc.clearDisplay(0);           // Clear display buffer
  
  // Set push button pins as inputs with pull-up resistors
  pinMode(leftButton, INPUT);
  pinMode(rightButton, INPUT);
  pinMode(upButton, INPUT);
  pinMode(downButton, INPUT);
  pinMode(selectButton, INPUT);
  pinMode(sendButton, INPUT);
}

void updateDisplay() {
  // Update display based on stored LED state
  for (int row = 0; row < numRows; row++) {
    for (int col = 0; col < numCols; col++) {
      // Set the LED state
      if (drawingPhase == true && attackPhase == false){
        lc.setLed(0, row, col, ledState[row][col]);
      }
      else{
        if (hiddenShips[row][col] == 1 && revealedState[row][col]) {
        // Ship found and revealed, light up the LED
        lc.setLed(0, row, col, 1);

      } else {
        // Either no ship or not revealed, keep it off
        lc.setLed(0, row, col, 0);
      }
      }
    }
  }

  // Toggle cursor visibility every 500 milliseconds
  if (millis() - lastBlinkMillis >= 250) {
    cursorVisible = !cursorVisible;
    lastBlinkMillis = millis();
  }

  // Show or hide the cursor at the current position
  lc.setLed(0, currentRow, currentCol, cursorVisible);
}

void sendBooleanArray(bool arr[8][8]) {
  Serial.print('A');
  for (int i = 0; i < numRows; i++) {
    for (int j = 0; j < numCols; j++) {
      Serial.write(arr[i][j] ? '1' : '0');
    }
  }
  Serial.println(); // End of the array
}

void receiveBooleanArray(bool arr[8][8]) {
  while (Serial.read() != 'B') {}  // Wait for start marker
  for (int i = 0; i < numRows; i++) {
    for (int j = 0; j < numCols; j++) {
      while (!Serial.available()) {} // Wait until data is available
      char receivedByte = Serial.read();
      arr[i][j] = (receivedByte == '1') ? true : false;
    }
  }
  attackPhase = true;
}

void printBooleanArray(bool arr[8][8]) {
  Serial.print('B');
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      Serial.print(arr[i][j] ? "1 " : "0 "); // Print 1 for true, 0 for false
    }
    Serial.println(); // Move to the next line after printing each row
  }
  Serial.println(); // End of the array
}

const int BLINK_INTERVAL = 500; // Blink interval in milliseconds
const int BLINK_LED_ON = 1;
const int BLINK_LED_OFF = 0;

bool missedBlinkState[numRows][numCols] = {false}; // Array to track blink state for missed locations
bool blinkLedState[numRows][numCols] = {false}; // Array to track LED state for blinking
bool control = true;

void revealShip() {
  // Check if there is a ship at the current cursor position
  if (hiddenShips[currentRow][currentCol] == 1) {
    // Ship found, mark it as revealed
    revealedState[currentRow][currentCol] = true;
    // Light up the LED
    lc.setLed(0, currentRow, currentCol, 1);
    Serial.println("Ship found at position: " + String(currentRow) + ", " + String(currentCol));
    // Play a musical note for ship found
    tone(buzzerPin, SHIP_FOUND_NOTE, 200); // Play the A4 note for 200ms
    
  } else {
    // No ship found, mark it as missed and start blinking
    missedBlinkState[currentRow][currentCol] = true;
    Serial.println("No ship found at position: " + String(currentRow) + ", " + String(currentCol));
    // Play a musical note for no ship found
    tone(buzzerPin, NO_SHIP_FOUND_NOTE, 200);
    control = false;
  }
}

void updateBlinking() {
  // Iterate through all missed locations
  for (int row = 0; row < numRows; row++) {
    for (int col = 0; col < numCols; col++) {
      if (missedBlinkState[row][col]) {
        // Toggle LED state
        blinkLedState[row][col] = !blinkLedState[row][col];
        lc.setLed(0, row, col, blinkLedState[row][col] ? BLINK_LED_ON : BLINK_LED_OFF);
        delay(2);
      }
    }
  }
}

void moveLeft() {
  if (currentCol > 0) {
    currentCol--; // Move left
    tone(buzzerPin, CURSOR_MOVE_NOTE, 100); // Play the C4 note for 100ms
    updateDisplay();
  }
}

void moveRight() {
  if (currentCol < numCols - 1) {
    currentCol++; // Move right
    tone(buzzerPin, CURSOR_MOVE_NOTE, 100); // Play the C4 note for 100ms
    updateDisplay();
  }
}

void moveUp() {
  if (currentRow > 0) {
    currentRow--; // Move up
    tone(buzzerPin, CURSOR_MOVE_NOTE, 100); // Play the C4 note for 100ms
    updateDisplay();
  }
}

void moveDown() {
  if (currentRow < numRows - 1) {
    currentRow++; // Move down
    tone(buzzerPin, CURSOR_MOVE_NOTE, 100); // Play the C4 note for 100ms
    updateDisplay();
  }
}

void saveState(bool arr[8][8]) {
  // Save the state of the LED at the current position
  arr[currentRow][currentCol] = !arr[currentRow][currentCol];
  tone(buzzerPin, SAVE_STATE_NOTE, 200); // Play the G4 note for 200ms
}

void draw(){
  if (digitalRead(leftButton) == HIGH) {
    moveLeft();
    delay(200); // Adjust delay as needed
  }
  
  // Check if right button is pressed
  if (digitalRead(rightButton) == HIGH) {
    moveRight();
    delay(200); // Adjust delay as needed
  }

  // Check if up button is pressed
  if (digitalRead(upButton) == HIGH) {
    moveUp();
    delay(200); // Adjust delay as needed
  }

  // Check if down button is pressed
  if (digitalRead(downButton) == HIGH) {
    moveDown();
    delay(200); // Adjust delay as needed
  }
  
  // Check if selectButton button is pressed
  if (digitalRead(selectButton) == HIGH) {
    saveState(ledState);
    delay(200);
  }

  if (digitalRead(sendButton) == HIGH) {
    // Serial.println("Right button pressed");
    sendBooleanArray(ledState);
    drawingPhase = false;
    cursorVisible = false;
  }
  // Update the display periodically
  updateDisplay();
}

void attack(){
  control = true;
  while(control){
  if (digitalRead(leftButton) == HIGH) {
    moveLeft();
    delay(200); // Adjust delay as needed
  }
  
  // Check if right button is pressed
  if (digitalRead(rightButton) == HIGH) {
    moveRight();
    delay(200); // Adjust delay as needed
  }

  // Check if up button is pressed
  if (digitalRead(upButton) == HIGH) {
    moveUp();
    delay(200); // Adjust delay as needed
  }

  // Check if down button is pressed
  if (digitalRead(downButton) == HIGH) {
    moveDown();
    delay(200); // Adjust delay as needed
  }

  // Check if select button is pressed
  if (digitalRead(selectButton) == HIGH) {
    revealShip();
    delay(200); // Adjust delay as needed
  }
  // Check for win
  if (checkAllShipsLocated()) {
    Serial.println("Congratulations! You've located all the ships. You win!");
    tone(buzzerPin, NOTE_C6, 500);
    attackPhase = false;
    control = false;
    win = true;
  }
  updateDisplay();
  updateBlinking();
  }
  // Update the display periodically
}

bool checkAllShipsLocated() {
  for (int i = 0; i < numRows; i++) {
    for (int j = 0; j < numCols; j++) {
      if (hiddenShips[i][j] == 1 && !revealedState[i][j]) {
        return false;  // Return false if any ship is not located
      }
    }
  }
  return true;  // Return true if all ships are located
}

void sendChar() {
  Serial.print('x');
}

void loop() {
  // Check if left button is pressed
  while(drawingPhase == true){
    draw();
  }
  receiveBooleanArray(hiddenShips);
  printBooleanArray(hiddenShips);
  lc.clearDisplay(0);

  while(attackPhase==true){
    attack();
    if(attackPhase){
      sendChar();
      while(Serial.read() != 'y'){}
        delay(10);
    }
    
  }
int winArray[numRows][numCols] = {
  {1, 1, 1, 1, 1, 1, 1, 1},
  {1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 1, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 1, 0, 0, 0},
  {0, 0, 0, 1, 1, 0, 0, 0},
  {0, 1, 1, 0, 0, 0, 0, 0},
  {1, 0, 0, 0, 0, 0, 0, 0},
  {1, 1, 1, 1, 1, 1, 1, 1}
};

    lc.clearDisplay(0);
    if(win){
      for (int row = 0; row < numRows; row++) {
        for (int col = 0; col < numCols; col++) {
          lc.setLed(0, row, col, winArray[row][col]);
        }
      }
    }
    while(true){}
}