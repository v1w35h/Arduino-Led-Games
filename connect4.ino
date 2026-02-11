#include <FastLED.h>

// Matrix configuration
#define DATA_PIN        5
#define UP_PIN          12
#define DOWN_PIN        11  // DOWN also acts as drop button
#define LEFT_PIN        9
#define RIGHT_PIN       8

#define COLOR_ORDER     GRB
#define LED_TYPE        WS2812B
#define MATRIX_WIDTH    16
#define MATRIX_HEIGHT   16
#define NUM_LEDS       (MATRIX_WIDTH * MATRIX_HEIGHT)

CRGB leds[NUM_LEDS];

// Connect 4 variables
#define BOARD_WIDTH     7    // 7 columns for Connect 4
#define BOARD_HEIGHT    6    // 6 rows for Connect 4
#define CELL_SIZE       2    // 2x2 pixels per cell
#define BOARD_START_X   1    // Start at column 1 (leaving 1 column border)
#define BOARD_START_Y   3    // Start at row 3 (leaving room at bottom for pieces to stack)

int board[BOARD_WIDTH][BOARD_HEIGHT] = {0};  // 0=empty, 1=player1(yellow), 2=player2(red)
int currentPlayer = 1;  // Player 1 starts
int currentColumn = 3;  // Middle column (0-6)
int gameState = 0;  // 0=playing, 1=player1 wins, 2=player2 wins, 3=tie
int winner = 0;

// Button variables
bool leftPressed = false, rightPressed = false, downPressed = false;
bool lastLeftState = HIGH, lastRightState = HIGH, lastDownState = HIGH;

uint16_t XY(uint8_t x, uint8_t y) {
  // Serpentine layout
  if (y % 2 == 0) {
    return y * MATRIX_WIDTH + x;
  } else {
    return y * MATRIX_WIDTH + (MATRIX_WIDTH - 1 - x);
  }
}

void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
  
  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(DOWN_PIN, INPUT_PULLUP);
  pinMode(LEFT_PIN, INPUT_PULLUP);
  pinMode(RIGHT_PIN, INPUT_PULLUP);
  
  resetGame();
}

void loop() {
  if (gameState != 0) {
    gameOverAnimation();
    
    // Check for restart (press any button)
    if (digitalRead(UP_PIN) == LOW || digitalRead(DOWN_PIN) == LOW || 
        digitalRead(LEFT_PIN) == LOW || digitalRead(RIGHT_PIN) == LOW) {
      delay(500);
      resetGame();
    }
    return;
  }
  
  handleButtons();
  drawBoard();
  
  delay(20);
}

void handleButtons() {
  // Read current states
  bool currentLeftState = digitalRead(LEFT_PIN);
  bool currentRightState = digitalRead(RIGHT_PIN);
  bool currentDownState = digitalRead(DOWN_PIN);
  
  // LEFT button - move column selection left
  if (currentLeftState == LOW && lastLeftState == HIGH) {
    currentColumn--;
    if (currentColumn < 0) currentColumn = 0;
    leftPressed = true;
  }
  if (currentLeftState == HIGH && lastLeftState == LOW) leftPressed = false;
  lastLeftState = currentLeftState;
  
  // RIGHT button - move column selection right
  if (currentRightState == LOW && lastRightState == HIGH) {
    currentColumn++;
    if (currentColumn >= BOARD_WIDTH) currentColumn = BOARD_WIDTH - 1;
    rightPressed = true;
  }
  if (currentRightState == HIGH && lastRightState == LOW) rightPressed = false;
  lastRightState = currentRightState;
  
  // DOWN button - DROP PIECE
  if (currentDownState == LOW && lastDownState == HIGH) {
    dropPiece();
    downPressed = true;
  }
  if (currentDownState == HIGH && lastDownState == LOW) downPressed = false;
  lastDownState = currentDownState;
}

void dropPiece() {
  // Find the LOWEST EMPTY ROW from the BOTTOM (Connect 4 gravity!)
  int row = -1;
  for (int r = BOARD_HEIGHT - 1; r >= 0; r--) {  // Start from bottom
    if (board[currentColumn][r] == 0) {
      row = r;
      break;  // Stop at the first empty row from bottom
    }
  }
  
  if (row == -1) {
    // Column is full - visual feedback
    for (int i = 0; i < 3; i++) {
      int previewX = BOARD_START_X + currentColumn * CELL_SIZE;
      int previewY = BOARD_START_Y - 2;
      leds[XY(previewX, previewY)] = CRGB::Red;
      leds[XY(previewX + 1, previewY)] = CRGB::Red;
      FastLED.show();
      delay(100);
      leds[XY(previewX, previewY)] = CRGB::Black;
      leds[XY(previewX + 1, previewY)] = CRGB::Black;
      FastLED.show();
      delay(100);
    }
    return;
  }
  
  // Place piece
  board[currentColumn][row] = currentPlayer;
  
  // Animate piece falling to the BOTTOM
  animatePieceFall(currentColumn, row, currentPlayer);
  
  // Check for win
  if (checkWin(currentColumn, row, currentPlayer)) {
    gameState = currentPlayer;
    winner = currentPlayer;
  } 
  else if (isBoardFull()) {
    gameState = 3;  // Tie
  }
  else {
    currentPlayer = (currentPlayer == 1) ? 2 : 1;
  }
}

void animatePieceFall(int col, int finalRow, int player) {
  int screenX = BOARD_START_X + col * CELL_SIZE;
  int startY = BOARD_START_Y - 4;  // Start above the board
  int finalY = BOARD_START_Y + finalRow * CELL_SIZE;  // finalRow is from bottom!
  
  CRGB color = (player == 1) ? CRGB::Yellow : CRGB::Red;
  
  // Animate falling DOWN
  for (int y = startY; y <= finalY; y += 1) {
    // Clear previous position
    leds[XY(screenX, y - 1)] = CRGB::Black;
    leds[XY(screenX + 1, y - 1)] = CRGB::Black;
    
    // Draw piece at current position
    leds[XY(screenX, y)] = color;
    leds[XY(screenX + 1, y)] = color;
    
    FastLED.show();
    delay(30);
  }
  
  // Draw final 2x2 piece
  leds[XY(screenX, finalY)] = color;
  leds[XY(screenX + 1, finalY)] = color;
  leds[XY(screenX, finalY + 1)] = color;
  leds[XY(screenX + 1, finalY + 1)] = color;
  FastLED.show();
}

bool checkWin(int col, int row, int player) {
  // Check horizontal
  int count = 0;
  for (int c = 0; c < BOARD_WIDTH; c++) {
    if (board[c][row] == player) {
      count++;
      if (count >= 4) return true;
    } else {
      count = 0;
    }
  }
  
  // Check vertical
  count = 0;
  for (int r = 0; r < BOARD_HEIGHT; r++) {
    if (board[col][r] == player) {
      count++;
      if (count >= 4) return true;
    } else {
      count = 0;
    }
  }
  
  // Check diagonal (down-right)
  int startCol = col - min(col, row);
  int startRow = row - min(col, row);
  count = 0;
  while (startCol < BOARD_WIDTH && startRow < BOARD_HEIGHT) {
    if (board[startCol][startRow] == player) {
      count++;
      if (count >= 4) return true;
    } else {
      count = 0;
    }
    startCol++;
    startRow++;
  }
  
  // Check diagonal (up-right)
  startCol = col - min(col, BOARD_HEIGHT - 1 - row);
  startRow = row + min(col, BOARD_HEIGHT - 1 - row);
  count = 0;
  while (startCol < BOARD_WIDTH && startRow >= 0) {
    if (board[startCol][startRow] == player) {
      count++;
      if (count >= 4) return true;
    } else {
      count = 0;
    }
    startCol++;
    startRow--;
  }
  
  return false;
}

bool isBoardFull() {
  for (int c = 0; c < BOARD_WIDTH; c++) {
    for (int r = 0; r < BOARD_HEIGHT; r++) {
      if (board[c][r] == 0) return false;
    }
  }
  return true;
}

void drawBoard() {
  FastLED.clear();
  
  
  // Draw pieces - BOTTOM of board is row 5, TOP of board is row 0
  for (int c = 0; c < BOARD_WIDTH; c++) {
    for (int r = 0; r < BOARD_HEIGHT; r++) {
      if (board[c][r] != 0) {
        int screenX = BOARD_START_X + c * CELL_SIZE;
        int screenY = BOARD_START_Y + r * CELL_SIZE;  // r=5 is bottom, r=0 is top
        
        // Draw 2x2 piece
        CRGB color = (board[c][r] == 1) ? CRGB::Yellow : CRGB::Red;
        
        leds[XY(screenX, screenY)] = color;
        leds[XY(screenX + 1, screenY)] = color;
        leds[XY(screenX, screenY + 1)] = color;
        leds[XY(screenX + 1, screenY + 1)] = color;
      }
    }
  }
  
  // Draw current column selector (above the board)
  if (gameState == 0) {
    int previewY = BOARD_START_Y - 2;
    int previewX = BOARD_START_X + currentColumn * CELL_SIZE;
    
    // Blink every 300ms
    if ((millis() / 300) % 2 == 0) {
      CRGB previewColor = (currentPlayer == 1) ? CRGB::Yellow : CRGB::Red;
      
      leds[XY(previewX, previewY)] = previewColor;
      leds[XY(previewX + 1, previewY)] = previewColor;
    }
  }
  
  FastLED.show();
}

void gameOverAnimation() {
  CRGB winColor;
  if (winner == 1) winColor = CRGB::Yellow;
  else if (winner == 2) winColor = CRGB::Red;
  else winColor = CRGB::Blue;  // Tie game
  
  for (int i = 0; i < 5; i++) {
    FastLED.clear();
    FastLED.show();
    delay(200);
    
    fill_solid(leds, NUM_LEDS, winColor);
    FastLED.show();
    delay(200);
  }
}

void resetGame() {
  // Clear board
  for (int c = 0; c < BOARD_WIDTH; c++) {
    for (int r = 0; r < BOARD_HEIGHT; r++) {
      board[c][r] = 0;
    }
  }
  
  currentPlayer = 1;
  currentColumn = 3;
  gameState = 0;
  winner = 0;
  
  FastLED.clear();
  FastLED.show();
  delay(500);
}
