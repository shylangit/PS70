#include <cstdlib>
#include <string>
#include <vector>
using namespace std;

void game_start_sequence();
void game_lost_sequence();
void game_win_sequence();

class Pair{ 
  private:
    int button_pin;
    int led_pin;
    int last_button_state;
    int current_button_state;
    int led_state;

  public:
    Pair(int button_pin_input, int led_pin_input){
      button_pin = button_pin_input;
      led_pin = led_pin_input;
      led_state = LOW;

      pinMode(button_pin, INPUT_PULLUP);
      pinMode(led_pin, OUTPUT);

      last_button_state = digitalRead(button_pin);
    }
    void resetLED(){
      digitalWrite(led_pin, LOW);
    }
    void toggleLED(){ 
      led_state = !led_state;
      digitalWrite(led_pin, led_state);
    }
    void updateLEDFromButton(){
      // LED on when button is pressed (LOW due to INPUT_PULLUP)
      current_button_state = digitalRead(button_pin);
      if (current_button_state == LOW){
        digitalWrite(led_pin, HIGH);
      } else {
        digitalWrite(led_pin, LOW);
      }
    }
    bool buttonPressed(){
      current_button_state = digitalRead(button_pin);
      if (last_button_state == HIGH && current_button_state == LOW){
        last_button_state = current_button_state;
        return true;
      }
      last_button_state = current_button_state;
      return false;
    }
};

// Initialize pair vector. 
Pair pair_1(D6, D5);
Pair pair_2(D7, D4);
Pair pair_3(D8, D2);
Pair pair_4(D9, D1);
Pair pair_5(D10, D0);
vector<Pair> pair_list = {pair_1, pair_2, pair_3, pair_4, pair_5};

void setup() {
  Serial.begin(115200);               
}

void loop() {
  unsigned int max_game_length = 5; // max sequence length
  unsigned int current_game_length = 0;
  vector<int> led_sequence; // containes a sequence of pair #'s by index
  Serial.println("Start Game!");
  game_start_sequence();

  while (current_game_length < max_game_length){ // 9 -> 10
    // GAME LOOP

    // Add a random LED # to the sequence
    led_sequence.push_back(random(5)); // [0,5)
    current_game_length++;
    Serial.println("Game Round " + String(led_sequence.size())); 

    // Make sure all lights are off from previous round
    for (int i = 0; i < pair_list.size(); i++)
      pair_list[i].resetLED();

    delay(1000);

    // Display the current LED sequence
    for (int i = 0; i < led_sequence.size(); i++){
      pair_list[led_sequence[i]].toggleLED();
      delay(1000);
      pair_list[led_sequence[i]].toggleLED();
      delay(1000);
    }

  // Read user inputted LED sequence
    int user_count = 0;
    while (user_count < led_sequence.size()){
      // USER INPUT WHILE LOOP

      for (int i = 0; i < pair_list.size(); i++){ // USER INPUT FOR LOOP
        pair_list[i].updateLEDFromButton(); 

        if (pair_list[i].buttonPressed()){ // BUTTON PRESSED
          Serial.println("BUTTON " + String(i) + " CLICKED");
          if (i != led_sequence[user_count]){
            // USER INPUT INCORRECT
            Serial.println("GAME OVER: YOU LOSE");
            user_count = led_sequence.size() + 1; // break out of user input while loop
            current_game_length = max_game_length + 1; // break out of game loop
            game_lost_sequence();
            break; // break out of user input for loop

          } else { // DEBUG: else if?
            // USER INPUT CORRECT
            user_count++;
            delay(300);
            if (user_count >= max_game_length){
              // USER WIN
              Serial.println("YOU WIN");
              game_win_sequence();
              // DEBUG: other breaks should be handled already
              break; // break out of user input for loop
            }
            break;
          }
        } // button pressed
      } // user input for loop
      delay(10);
    } // user input while loop
  } // game while loop
} // loop

void game_start_sequence(){
  /* LED SEQUENCE FOR GAME START */
  for (int i = 0; i < pair_list.size(); i++){
    pair_list[i].toggleLED();
    delay(1000);
    pair_list[i].toggleLED();
  }
}

void game_lost_sequence(){
  /* LED SEQUENCE FOR GAME LOST */
  // L in morse code with LED furthest right
  pair_list[0].toggleLED();
  delay(1000);
  pair_list[0].toggleLED();
  delay(1000);

  pair_list[0].toggleLED();
  delay(2000);
  pair_list[0].toggleLED();
  delay(1000);

  pair_list[0].toggleLED();
  delay(1000);
  pair_list[0].toggleLED();
  delay(1000);

  pair_list[0].toggleLED();
  delay(1000);
  pair_list[0].toggleLED();
  delay(2000);
}

void game_win_sequence(){
  /* LED SEQUENCE FOR GAME WIN */
  // turn individual LEDs on in sequence
  for (int i = 0; i < pair_list.size(); i++){ // ON
    pair_list[i].toggleLED();
    delay(500);
  }
  for (int i = 0; i < pair_list.size(); i++){ // OFF
    pair_list[i].toggleLED();
    delay(500);
  }

  // turn on and off all LEDs 3 times
  for (int i = 0; i < pair_list.size(); i++){ // ON
    pair_list[i].toggleLED();
  }
  delay(500);
  for (int i = 0; i < pair_list.size(); i++){ // OF
    pair_list[i].toggleLED();
  }
  delay(500);
  for (int i = 0; i < pair_list.size(); i++){ // ON
    pair_list[i].toggleLED();
  }
  delay(500);
  for (int i = 0; i < pair_list.size(); i++){ // OF
    pair_list[i].toggleLED();
  }
  delay(500);
  for (int i = 0; i < pair_list.size(); i++){ // ON
    pair_list[i].toggleLED();
  }
  delay(500);
  for (int i = 0; i < pair_list.size(); i++){ // OF
    pair_list[i].toggleLED();
  }
  delay(2000);
}
