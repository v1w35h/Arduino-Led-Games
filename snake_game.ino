#include <FastLED.h>

// Matrix configuration
#define DATA_PIN        5         // Data pin
#define UP_PIN          12
#define DOWN_PIN        11
#define LEFT_PIN        9
#define RIGHT_PIN       8

#define COLOR_ORDER     GRB       // Color order
#define LED_TYPE        WS2812B   // LED chipset
#define MATRIX_WIDTH    16
#define MATRIX_HEIGHT   16
#define NUM_LEDS       (MATRIX_WIDTH * MATRIX_HEIGHT)

CRGB leds[NUM_LEDS];

// Button variables
bool UpPressed = false, DownPressed = false, LPressed = false, RPressed = false;
bool lastUpState = HIGH, lastDownState = HIGH, lastLState = HIGH, lastRState = HIGH;

// Snake game variables
#define MAX_SNAKE_LENGTH 100  // Maximum snake length
int snakeX[MAX_SNAKE_LENGTH];
int snakeY[MAX_SNAKE_LENGTH];
int snakeLength = 3;  // Start with 3 segments
int direction = 3;    // 0=up, 1=down, 2=left, 3=right (start moving right)
int nextDirection = 3;
unsigned long lastMoveTime = 0;
int moveDelay = 300;  // ms between moves

// Food variables
int foodX, foodY;

// Game state
bool gameOver = false;
int score = 0;

uint16_t XY(uint8_t x, uint8_t y) {
  // Assuming serpentine layout (snake pattern)
  if (y % 2 == 0) {
    return y * MATRIX_WIDTH + x;
  } else {
    return y * MATRIX_WIDTH + (MATRIX_WIDTH - 1 - x);
  }
}

// Alternative straight layout
// uint16_t XY(uint8_t x, uint8_t y) {
//   return y * MATRIX_WIDTH + x;
// }

void setup() {
  // Initialize LEDs
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(100);
  
  // Initialize buttons
  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(DOWN_PIN, INPUT_PULLUP);
  pinMode(LEFT_PIN, INPUT_PULLUP);
  pinMode(RIGHT_PIN, INPUT_PULLUP);
  
  // Initialize random seed
  randomSeed(analogRead(0));
  
  // Initialize snake game
  initGame();
}

void loop() {
  if (gameOver) {
    // Snake turns red and stays red
    drawGameOverSnake();
    
    // Check for restart
    if (checkForRestart()) {
      initGame();
    }
    return;
  }
  
  // Handle button input
  handleButtons();
  
  // Move snake at regular intervals
  unsigned long currentTime = millis();
  if (currentTime - lastMoveTime >= moveDelay) {
    moveSnake();
    lastMoveTime = currentTime;
  }
  
  // Draw everything
  drawGame();
  
  delay(10);
}

void initGame() {
  // Clear everything
  FastLED.clear();
  gameOver = false;
  score = 0;
  snakeLength = 3;
  direction = 3;  // Start moving right
  nextDirection = 3;
  moveDelay = 300;
  
  // Initialize snake - start safely inside the matrix (not on edges)
  int startX = MATRIX_WIDTH / 2;
  int startY = MATRIX_HEIGHT / 2;
  
  // Make sure snake starts away from edges
  if (startX < 1) startX = 1;
  if (startX >= MATRIX_WIDTH - 1) startX = MATRIX_WIDTH - 2;
  if (startY < 1) startY = 1;
  if (startY >= MATRIX_HEIGHT - 1) startY = MATRIX_HEIGHT - 2;
  
  snakeX[0] = startX;
  snakeY[0] = startY;
  snakeX[1] = startX - 1;
  snakeY[1] = startY;
  snakeX[2] = startX - 2;
  snakeY[2] = startY;
  
  // Generate first food
  generateFood();
  
  lastMoveTime = millis();
}

void handleButtons() {
  // Read button states
  bool currentUpState = digitalRead(UP_PIN);
  bool currentDownState = digitalRead(DOWN_PIN);
  bool currentLState = digitalRead(LEFT_PIN);
  bool currentRState = digitalRead(RIGHT_PIN);
  
  // UP button - can't reverse from down to up
  if (currentUpState == LOW && lastUpState == HIGH) {
    if (direction != 1) nextDirection = 0;
    UpPressed = true;
  }
  if (currentUpState == HIGH && lastUpState == LOW && UpPressed) {
    UpPressed = false;
  }
  
  // DOWN button - can't reverse from up to down
  if (currentDownState == LOW && lastDownState == HIGH) {
    if (direction != 0) nextDirection = 1;
    DownPressed = true;
  }
  if (currentDownState == HIGH && lastDownState == LOW && DownPressed) {
    DownPressed = false;
  }
  
  // LEFT button - can't reverse from right to left
  if (currentLState == LOW && lastLState == HIGH) {
    if (direction != 3) nextDirection = 2;  // LEFT = 2
    LPressed = true;
  }
  if (currentLState == HIGH && lastLState == LOW && LPressed) {
    LPressed = false;
  }
  
  // RIGHT button - can't reverse from left to right
  if (currentRState == LOW && lastRState == HIGH) {
    if (direction != 2) nextDirection = 3;  // RIGHT = 3
    RPressed = true;
  }
  if (currentRState == HIGH && lastRState == LOW && RPressed) {
    RPressed = false;
  }
  
  // Update direction
  direction = nextDirection;
  
  // Save states
  lastUpState = currentUpState;
  lastDownState = currentDownState;
  lastLState = currentLState;
  lastRState = currentRState;
}

void moveSnake() {
  // Save current head position
  int oldHeadX = snakeX[0];
  int oldHeadY = snakeY[0];
  
  // Calculate new head position based on direction
  int newHeadX = oldHeadX;
  int newHeadY = oldHeadY;
  
  switch (direction) {
    case 0: // UP
      newHeadY--;
      break;
    case 1: // DOWN
      newHeadY++;
      break;
    case 2: // LEFT  <-- FIXED: LEFT should DECREASE X
      newHeadX--;
      break;
    case 3: // RIGHT <-- FIXED: RIGHT should INCREASE X
      newHeadX++;
      break;
  }
  
  // Check for wall collision (edges of matrix)
  if (newHeadX < 0 || newHeadX >= MATRIX_WIDTH ||
      newHeadY < 0 || newHeadY >= MATRIX_HEIGHT) {
    gameOver = true;
    return;
  }
  
  // Check for collision with self
  for (int i = 0; i < snakeLength; i++) {
    if (snakeX[i] == newHeadX && snakeY[i] == newHeadY) {
      gameOver = true;
      return;
    }
  }
  
  // Move snake body
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }
  
  // Update head position
  snakeX[0] = newHeadX;
  snakeY[0] = newHeadY;
  
  // Check if food is eaten
  if (newHeadX == foodX && newHeadY == foodY) {
    // Increase snake length
    if (snakeLength < MAX_SNAKE_LENGTH) {
      snakeLength++;
      // New segment appears at tail position
      snakeX[snakeLength - 1] = snakeX[snakeLength - 2];
      snakeY[snakeLength - 1] = snakeY[snakeLength - 2];
    }
    
    // Increase score
    score++;
    
    // Generate new food
    generateFood();
    
    // Speed up slightly
    if (moveDelay > 100) {
      moveDelay -= 5;
    }
  }
}

void generateFood() {
  bool foodOnSnake;
  
  do {
    foodOnSnake = false;
    
    // Generate food anywhere on the matrix (not on edges is safer)
    // Using 1 to WIDTH-2 and 1 to HEIGHT-2 to avoid spawning on edges
    foodX = random(1, MATRIX_WIDTH - 1);
    foodY = random(1, MATRIX_HEIGHT - 1);
    
    // Check if food is on snake
    for (int i = 0; i < snakeLength; i++) {
      if (snakeX[i] == foodX && snakeY[i] == foodY) {
        foodOnSnake = true;
        break;
      }
    }
    
  } while (foodOnSnake);
}

void drawGame() {
  // Clear display
  FastLED.clear();
  
  // Draw snake - ALL WHITE (head and body)
  for (int i = 0; i < snakeLength; i++) {
    leds[XY(snakeX[i], snakeY[i])] = CRGB::White;
  }
  
  // Draw food - Red
  leds[XY(foodX, foodY)] = CRGB::Red;
  
  FastLED.show();
}

void drawGameOverSnake() {
  // Snake turns red (no flashing, just turns red)
  FastLED.clear();
  
  // Draw the entire snake in RED
  for (int i = 0; i < snakeLength; i++) {
    leds[XY(snakeX[i], snakeY[i])] = CRGB::Red;
  }
  
  // Also draw food (if it was on screen when game ended)
  leds[XY(foodX, foodY)] = CRGB::Red;
  
  FastLED.show();
  
  // Just wait and check for restart (no animation)
  delay(100);
}

bool checkForRestart() {
  // Check if any button is pressed to restart
  // Check for a longer press to avoid accidental restarts
  unsigned long buttonPressTime = 0;
  bool buttonWasPressed = false;
  
  for (int i = 0; i < 100; i++) {  // Check for about 2 seconds
    bool anyButtonPressed = (digitalRead(UP_PIN) == LOW || 
                            digitalRead(DOWN_PIN) == LOW ||
                            digitalRead(LEFT_PIN) == LOW || 
                            digitalRead(RIGHT_PIN) == LOW);
    
    if (anyButtonPressed) {
      if (!buttonWasPressed) {
        buttonWasPressed = true;
        buttonPressTime = millis();
      } else if (millis() - buttonPressTime > 500) {
        // Button held for 500ms - restart
        delay(200);  // Debounce
        return true;
      }
    } else {
      buttonWasPressed = false;
    }
    
    delay(20);
  }
  
  return false;
}
