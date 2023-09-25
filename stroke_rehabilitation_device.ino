#include <Servo.h>      // necessary library to code servos
volatile Servo servo1;  // creates a motor object. Naming it servo1 by convention.

#define STOP_RESET_BUTTON_PIN 2      //RED BUTTON
#define INCREASE_SPEED_BUTTON_PIN 5  //YELLOW BUTTON
#define DECREASE_SPEED_BUTTON_PIN 6  //WHITE BUTTON
#define START_BUTTON_PIN 7           //GREEN BUTTON
#define ledPinGreen 9
#define ledPinBlue 10
#define ledPinRed 11
#define pinNumber 12

volatile bool is_system_running = false;

int RubberPin = A2;               
const int Resistor = 3300;         
const int sample_interval = 1000;  
int motor_speed = 94;   

float R_voltage;     // measured voltage drop value of the rubber wire
float R_resistance;  // calculated resistance value of the rubber wire
float sample_time;   // sample time the resistances are sampled at, increments by sample_interval every loop
int i = 0;           // counter for calculating sample time
double force;
double displacement;

//Global variables
bool stop_reset_button_pressed = false;
bool yellow_button_state = LOW;
bool white_button_state = LOW;
unsigned long cw_startTime = 0;
unsigned long ccw_startTime = 0;
volatile unsigned long cw_rotation_time = 0;
volatile unsigned long ccw_rotation_time = 0;
bool motor_is_rotating_cw = true;
volatile unsigned long totalTime = 0;  //initial total time is 0
unsigned long startTime = 0;

void setup() {
  Serial.begin(9600);                                                                   // initialize serial communication, 9600 is the baud rate
  servo1.attach(pinNumber);                                                             // assigns pinNumber as the servo motor's control signal pin
  Serial.print('\n');                                                                   // starts the program on a new line in the serial monitor
  Serial.println("Sample Time, Voltage(V), Resistance(Ω), Force(N), Displacement(m)");  // prints the column headings

  pinMode(START_BUTTON_PIN, INPUT);
  pinMode(STOP_RESET_BUTTON_PIN, INPUT);
  pinMode(INCREASE_SPEED_BUTTON_PIN, INPUT);
  pinMode(DECREASE_SPEED_BUTTON_PIN, INPUT);
  pinMode(ledPinRed, OUTPUT);
  pinMode(ledPinGreen, OUTPUT);
  pinMode(ledPinBlue, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(STOP_RESET_BUTTON_PIN), set_start_button_pressed_to_false, RISING);
  analogWrite(ledPinRed, 0);
  analogWrite(ledPinGreen, 255);
  analogWrite(ledPinBlue, 0);
  servo1.write(motor_speed);
}

void loop() {
  //read_and_calculate_resistance();
  bool increase_button_pressed = false;
  bool decrease_button_pressed = false;
  increase_button_pressed = digitalRead(INCREASE_SPEED_BUTTON_PIN);
  decrease_button_pressed = digitalRead(DECREASE_SPEED_BUTTON_PIN);
  delay(150);

  // Increase motor speed if button is pressed
  if (increase_button_pressed == HIGH) {
    motor_speed -= 2;
    if (motor_speed < 88) motor_speed = 88;  // limit the maximum speed to 88
  } else if (decrease_button_pressed == HIGH) {
    motor_speed += 2;
    if (motor_speed > 92) motor_speed = 92;  // limit the minimum speed to 92
  }

  // Change led based on speed
  if (motor_speed == 92) {
    analogWrite(ledPinRed, 0);
    analogWrite(ledPinGreen, 255);
    analogWrite(ledPinBlue, 0); 
  } else if (motor_speed == 90) {
    analogWrite(ledPinRed, 255);
    analogWrite(ledPinGreen, 0);
    analogWrite(ledPinBlue, 255);  
  } else if (motor_speed == 88) {
    analogWrite(ledPinRed, 255);
    analogWrite(ledPinGreen, 0);
    analogWrite(ledPinBlue, 0); 
  }

  is_system_running = digitalRead(START_BUTTON_PIN);
  while (is_system_running == true) {
    if (motor_speed == 94) motor_speed = 92;
    motor_protocol(2000);
    if (is_system_running == false) {
      move_mtr("stop", 94, 0);
    }
  }
}

void set_start_button_pressed_to_false() {
  if (motor_is_rotating_cw == true) {
    totalTime = millis() - startTime;  // if cw, add to the total time
  }
  if (motor_is_rotating_cw == false) {
    totalTime = 6000 - (millis() - startTime);   //if ccw, remove from total time
  }
  is_system_running = false;
  servo1.write(94);
}

// move the motor in the desired direction at a specifc speed
void move_mtr(String direction, int input_speed, int rotationSeconds) {
  if (direction == "cw") {
    servo1.write(input_speed);
  } else if (direction == "ccw") {
    servo1.write((94 - input_speed) + 94);
  } else if (direction == "stop") {
    servo1.write(94);
    return;
  }
  delay(rotationSeconds);
  servo1.write(94);
}

// function to start running the motor
void motor_protocol(int inter_cycle_delay) {
  servo1.attach(pinNumber);

  totalTime = 0;  
  motor_is_rotating_cw == true;
  startTime = millis();
  move_mtr("ccw", motor_speed, 2000);
  if (!is_system_running) {
    move_mtr("cw", motor_speed, totalTime);
    return; 
  }
  read_and_calculate_resistance();
  delay(inter_cycle_delay);

  motor_is_rotating_cw == false;
  move_mtr("cw", motor_speed, 2000);  //Rotate motor ccw for as long as it rotated cw
  if (!is_system_running) {
    move_mtr("cw", motor_speed, 6000 - totalTime);
    delay(inter_cycle_delay);
    return; 
  }
  delay(inter_cycle_delay);
}

void read_and_calculate_resistance() {
  // process the input values
  R_voltage = analogRead(RubberPin) * (5.0 / 1023.0);             
  R_resistance = (Resistor * R_voltage / (5 - R_voltage))-1400;  // voltage divider equation re-arranged to find R2, R2 = R1*Vout/(Vin - Vout), where R2 is the rubber wire
  sample_time = sample_interval * i++ / 1000;  // computes the sample time the sample is taken at (in seconds)
  force = exp((R_resistance - 945.68) / 206.59);
  displacement = (force - 0.5) / 132;

  // compute and output the sample time into rows on the serial monitor each time void loop() runs
  Serial.print(sample_time);
  Serial.print(',');  // separating the samples by a comma such that the row can later be copy+pasted into individual spreadsheet cells
  Serial.print(R_voltage);
  Serial.print(',');
  Serial.print(R_resistance);
  Serial.print(',');
  Serial.print(force);
  Serial.print(',');
  Serial.println(displacement);
}
