/*
 * ============================================================
 *  Pin Connections
 *
 *  nRF24L01 (SPI):
 *    VCC  → 3.3V        GND  → GND
 *    CE   → PIN 8       CSN  → PIN 10
 *    MOSI → PIN 11      MISO → PIN 12
 *    SCK  → PIN 13
 *
 *  L298N Motor Driver:
 *    Right Motor:
 *      IN1 (Forward)  → PIN 2
 *      IN2 (Backward) → PIN 3
 *      ENA (Speed)    → PIN 5 (PWM)
 *
 *    Left Motor:
 *      IN3 (Forward)  → PIN 4
 *      IN4 (Backward) → PIN 7
 *      ENB (Speed)    → PIN 6 (PWM)
 * ============================================================
 */


#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

//================================
int R_Offset = 50; // 255 - R_Offset = Right Motor Speed

#define R_F 2 // Right Forward
#define R_B 3
#define R_S 5

#define L_F 4
#define L_B 7
#define L_S 6 // Left Speed [0,255]

//===================================

#define CE 8
#define CSN 10

//===================================

RF24 radio(CE, CSN); 
const byte address[6] = "00001";

struct Data_Package {
  float zAxis = 0;
  float yAxis = 0;
};

Data_Package myData;

void setup() {

  Serial.begin(9600);

  pinMode(R_F, OUTPUT);
  pinMode(R_B, OUTPUT);
  pinMode(L_F, OUTPUT);
  pinMode(L_B, OUTPUT);

  pinMode(R_S, OUTPUT);   
  pinMode(L_S, OUTPUT);   

  // DO NOT TOUCH!!
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MAX);
  radio.startListening();
}

void loop() {

  if (radio.available()) {

    radio.read(&myData, sizeof(Data_Package));

    Serial.print("Z: ");
    Serial.print(myData.zAxis);
    Serial.print("   Y: ");
    Serial.println(myData.yAxis);

  }

  // // else {
  // //   Serial.println("No data");
    
  //  }
  // xAxis = roll  (tilt left/right)
  // yAxis = pitch (tilt forward/backward)

  if      (myData.yAxis >  30)  moveForward(255);
  else if (myData.yAxis < -30)  moveBackward(255);
  else if (myData.zAxis >  30)  moveRight(255 * 0.75);
  else if (myData.zAxis < -30)  moveLeft(255 * 0.75);
  else                          stopMotors();

}

// ==========================
// Movement Functions
// ==========================

void moveForward(int speed) {
    digitalWrite(R_F, HIGH);
    digitalWrite(R_B, LOW);

    digitalWrite(L_F, HIGH);
    digitalWrite(L_B, LOW);

    analogWrite(R_S, speed - R_Offset);
    analogWrite(L_S, speed);
}

void moveRight(int speed) {
    digitalWrite(R_F, LOW);
    digitalWrite(R_B, LOW);

    digitalWrite(L_F, HIGH);
    digitalWrite(L_B, LOW);

    analogWrite(R_S, 0);
    analogWrite(L_S, speed);
}

void stopMotors() {
    digitalWrite(R_F, LOW);
    digitalWrite(R_B, LOW);

    digitalWrite(L_F, LOW);
    digitalWrite(L_B, LOW);

    analogWrite(R_S, 0);
    analogWrite(L_S, 0);
}

void moveBackward(int speed) {
    digitalWrite(R_F, LOW);
    digitalWrite(R_B, HIGH);

    digitalWrite(L_F, LOW);
    digitalWrite(L_B, HIGH);

    analogWrite(R_S, speed - R_Offset);
    analogWrite(L_S, speed);
}

void moveLeft(int speed) {
    digitalWrite(R_F, HIGH);
    digitalWrite(R_B, LOW);

    digitalWrite(L_F, LOW);
    digitalWrite(L_B, HIGH);

    analogWrite(R_S, speed - R_Offset);
    analogWrite(L_S, 0);
}
