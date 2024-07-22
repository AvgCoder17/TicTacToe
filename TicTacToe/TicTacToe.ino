#include "LedControl.h"
#include "IRremote.h"
#include "SevSegShift.h"

//MAX7219 Driver
LedControl lc = LedControl(12,10,11,1);

//4 digit 7 Segment display with 74HC595 IC
const int clk = 9;
const int latch = 8;
const int data = 7;
SevSegShift sevsegshift(data, clk, latch, 1, true);

//Player scores
int red_score = 0;
int green_score = 0;

//IR Receiver
const int receiver = 13;
uint32_t last_decodedRawData = 0;
IRrecv irrecv(receiver);

//Player Turn (0 == Red, 1 == Green)
 int player_turn = 0;
//Keeps track of which player started the game
int turn_start = 0;
int button_pushed = -1;
bool is_win = false;

//Keeps track of available moves
int move_count = 0;
bool moves[9] = {false};

//TicTacToe board to keep track of the game state
int board[3][3];
int row;
int col;

void setup() {
  //Configuring Timer Interrupt with 450 Hz
  //Timer Interrupt will constantly refresh the display of 4 digits
  cli();
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  OCR1A = 555;// = (16*10^6) / (64*450) - 1 
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 64 prescaler
  TCCR1B |= (1 << CS11) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();

  lc.shutdown(0, false);
  lc.clearDisplay(0);

  //Configuring 4 digit 7 segment display
  byte num_digits = 4;
  byte digit_pins[] = {6, 5, 4, 3};
  byte segment_pins[] = {0, 1, 2, 3, 4, 5 ,6, 7};
  bool resistor_on_segments = false;
  byte hardwareConfig = COMMON_CATHODE;
  bool update_with_delays = false;
  bool leading_zeros = true;
  bool disable_dec_point = false;

  sevsegshift.begin(hardwareConfig, num_digits, digit_pins, segment_pins,
  resistor_on_segments, update_with_delays, leading_zeros, disable_dec_point);
  sevsegshift.setBrightness(80);
  update_display();

  irrecv.enableIRIn();
  Serial.begin(9600);

  reset_board();
}

void loop() {
  Serial.println("Getting Move");
  get_move(); 
  if (moves[button_pushed - 1] == false) {
    move_count++;
    Serial.println("New Move");
    Serial.print("Move #");
    Serial.println(move_count);

    moves[button_pushed - 1] = true;
    light_led();
    
    if (move_count == 9) {
      Serial.println("Draw");
      reset_game(is_win);
    }
    is_win = check_win();
    swap_turn(is_win);
  }
  //Resets button_pushed for next turn
  button_pushed = -1;
}
//Loops until a valid move that hasn't been played is given
void get_move() {
  while(button_pushed == -1) {
    if(irrecv.decode()) {
      translate_IR();
      irrecv.resume();
    }
  }
  return;
}
//Translates IR code from button input
void translate_IR() {
  // Check if it is a repeat IR code 
  if (irrecv.decodedIRData.flags) {
    //set the current decodedRawData to the last decodedRawData 
    irrecv.decodedIRData.decodedRawData = last_decodedRawData;
    Serial.println("REPEAT!");
  } 
  else {
    switch(irrecv.decodedIRData.decodedRawData) {
      case 0xF30CFF00: button_pushed = 1; break;
      case 0xE718FF00: button_pushed = 2; break;
      case 0xA15EFF00: button_pushed = 3; break;
      case 0xF708FF00: button_pushed = 4; break;
      case 0xE31CFF00: button_pushed = 5; break;
      case 0xA55AFF00: button_pushed = 6; break;
      case 0xBD42FF00: button_pushed = 7; break;
      case 0xAD52FF00: button_pushed = 8; break;
      case 0xB54AFF00: button_pushed = 9; break;
      default: button_pushed = -1; break;
    }
    if (button_pushed != -1) {
      Serial.print(button_pushed);
      Serial.println(" was pushed");
      if (moves[button_pushed - 1]) {
        Serial.println("Move has been played");
        button_pushed = -1;
        return;
      }
    }
    else {
      Serial.println("Invalid Move, Press keys 1-9");
      return;
    }
  }
  last_decodedRawData = irrecv.decodedIRData.decodedRawData;
  delay(500);
  return;
}
//Lights up the led in the led matrix based on button input and player's turn
//Updates row and col for check_win() algorithm
void light_led() {
  if(player_turn) {
    switch(button_pushed) {
      case 1: 
        lc.setLed(0, 0, 1, 1); 
        board[0][0] = player_turn; 
        row = 0, col = 0;
        break;
      case 2: 
        lc.setLed(0, 0, 3, 1); 
        board[0][1] = player_turn; 
        row = 0, col = 1;
        break;
      case 3: 
        lc.setLed(0, 0, 5, 1); 
        board[0][2] = player_turn; 
        row = 0, col = 2;
        break;
      case 4: 
        lc.setLed(0, 1, 1, 1); 
        board[1][0] = player_turn; 
        row = 1, col = 0;
        break;
      case 5: 
        lc.setLed(0, 1, 3, 1); 
        board[1][1] = player_turn; 
        row = 1, col = 1;
        break;
      case 6: 
        lc.setLed(0, 1, 5, 1); 
        board[1][2] = player_turn; 
        row = 1, col = 2;
        break;
      case 7: 
        lc.setLed(0, 2, 1, 1); 
        board[2][0] = player_turn; 
        row = 2, col = 0;
        break;
      case 8: 
        lc.setLed(0, 2, 3, 1); 
        board[2][1] = player_turn; 
        row = 2, col = 1;
        break;
      case 9: 
        lc.setLed(0, 2, 5, 1); 
        board[2][2] = player_turn; 
        row = 2, col = 2;
        break;
    }
  }
  else {
    switch(button_pushed) {
      case 1: 
        lc.setLed(0, 0, 0, 1); 
        board[0][0] = player_turn; 
        row = 0, col = 0;
        break;
      case 2: 
        lc.setLed(0, 0, 2, 1); 
        board[0][1] = player_turn; 
        row = 0, col = 1;
        break;
      case 3: 
        lc.setLed(0, 0, 4, 1); 
        board[0][2] = player_turn; 
        row = 0, col = 2;
        break;
      case 4: 
        lc.setLed(0, 1, 0, 1); 
        board[1][0] = player_turn; 
        row = 1, col = 0;
        break;
      case 5: 
        lc.setLed(0, 1, 2, 1); 
        board[1][1] = player_turn; 
        row = 1, col = 1;
        break;
      case 6: 
        lc.setLed(0, 1, 4, 1); 
        board[1][2] = player_turn; 
        row = 1, col = 2;
        break;
      case 7: 
        lc.setLed(0, 2, 0, 1); 
        board[2][0] = player_turn; 
        row = 2, col = 0;
        break;
      case 8: 
        lc.setLed(0, 2, 2, 1); 
        board[2][1] = player_turn; 
        row = 2, col = 1;
        break;
      case 9: 
        lc.setLed(0, 2, 4, 1); 
        board[2][2] = player_turn; 
        row = 2, col = 2;
        break;
    }
  }
  return;
}
//Checks if the last played move won the game for player's turn
bool check_win() {
  //Check for win across row
  for (int i = 0; i < 3; i++) {
    if (board[i][col] != player_turn) {
      break;
    }
    if (i == 2) {
      winning_msg();
      return true;
    }
  }
  //Check for win across column
  for (int i = 0; i < 3; i++) {
    if (board[row][i] != player_turn) {
      break;
    }
    if (i == 2) {
      winning_msg();
      return true;
    }
  }
  //Check for win on main diagonal
  if (row == col) {
    for (int i = 0; i < 3; i++) {
      if (board[i][i] != player_turn) {
        break;
      }
      if (i == 2) {
        winning_msg();
        return true;
      }
    }
  }
  //Check for win across anti-diagonal
  if ((row + col) == 2) {
    for (int i = 0; i < 3; i++) {
      if (board[i][2 - i] != player_turn) {
        break;
      }
      if (i == 2) {
        winning_msg();
        return true;
      }
    }
  }
  return false;
}
//Prints out winning message 
void winning_msg() 
{
  Serial.print("Win for ");
  if (player_turn)
  {
    Serial.println("green");
  }
  else
  {
    Serial.println("red");
  }
  return;
}
//Swaps player turns
//Resets game if someone has won and starts game with opposite player
void swap_turn(bool game_over) {
  if (game_over)
  {
    reset_game(is_win);
    if (turn_start) {
      turn_start = 0; 
      player_turn = 0;
      return;
    }
    else {
      turn_start = 1;
      player_turn = 1;
      return;
    }
  }
  if (player_turn) {
    player_turn = 0;
  }
  else {
    player_turn = 1;
  }
  return;
} 
//Updates red and green score
//2 points for win, 1 point for draw
void update_score(bool is_win) {
  if (is_win) {
    if (player_turn) {
      green_score += 2;
    }
    else {
      red_score += 2;
    }
  }
  else {
    red_score += 1;
    green_score += 1;
  }
  //red and green score resets to 0 if greater than 99
  if (red_score > 99) {
    red_score = 0;
  }
  if (green_score > 99) {
    green_score = 0;
  }
}
//Updates 4 digit 7 segment display with red and green scores
//First 2 digits represents red, last 2 digits represents green
void update_display() {
  int display_num = (red_score * 100) + green_score;
  Serial.print("Displaying ");
  Serial.println(display_num);
  sevsegshift.setNumber(display_num);
}

//Updates the score and resets everything for the next game
void reset_game(bool is_win) {
  update_score(is_win);
  update_display();
  Serial.println("Clearing Board for new game");
  print_board();
  reset_board();
  lc.clearDisplay(0);
  move_count = 0;
  for (int i = 0; i < 9; i++) {
    moves[i] = false;
  }
}
//Prints TicTacToe board
void print_board() {
  //Prints game state
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      Serial.print(board[i][j]);
      Serial.print(" ");
    }
    Serial.println("");
  }
  return;
}
//Resets each value to 1
void reset_board() {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      board[i][j] = -1;
    }
  }
  return;
}
//Constantly refreshes display
ISR(TIMER1_COMPA_vect){
  sevsegshift.refreshDisplay();
}