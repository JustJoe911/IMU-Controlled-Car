/*
 * ============================================================
 *  Pin Connections
 *
 *  MPU6050 (I2C):
 *    VCC → 3.3V or 5V   GND → GND
 *    SDA → GPIO 21      SCL → GPIO 22
 *    AD0 → GND  (addr = 0x68)
 *
 *  nRF24L01 (SPI):
 *    VCC  → 3.3V        GND  → GND
 *    CE   → GPIO 4      CSN  → GPIO 5
 *    MOSI → GPIO 23     MISO → GPIO 19
 *    SCK  → GPIO 18
 *
 *  LED (Calibration Indicator):
 *    LED  → GPIO 2
 * ============================================================
 */


#include<nRF24L01.h>
#include<RF24.h>
#include<SPI.h>

RF24 radio(4, 5); // CE, CSN any digital pins 

const byte address[6] = "00001";

struct Data_Package {
  float zAxis; // right and left
  float yAxis; // forward and backward
};

Data_Package myData;




//=============================================================
#include <Wire.h> 
#include <math.h>

#define MPU_ADDR  0x68
#define ALPHA     0.95f      // Complementary filter weight (0–1)
#define CALIB_N   2000       // Calibration samples

// Offsets (filled by calibrate())
float offGx, offGy, offGz;
float offAx, offAy, offAz;

// Angles
float roll  = 0;
float pitch = 0;
float yaw   = 0;

unsigned long prevMicros;

// ── I2C write ────────────────────────────────────────────────
void mpuWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg); Wire.write(val);
  Wire.endTransmission();
}

// ── Read all 6 axes in one burst ─────────────────────────────
void readRaw(int16_t &ax, int16_t &ay, int16_t &az,
             int16_t &gx, int16_t &gy, int16_t &gz) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);                        // ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);

  ax = Wire.read() << 8 | Wire.read();
  ay = Wire.read() << 8 | Wire.read();
  az = Wire.read() << 8 | Wire.read();
  Wire.read(); Wire.read();                // skip temperature
  gx = Wire.read() << 8 | Wire.read();
  gy = Wire.read() << 8 | Wire.read();
  gz = Wire.read() << 8 | Wire.read();
}

// ── Calibration ──────────────────────────────────────────────
void calibrate() {
  digitalWrite(2, HIGH);
  Serial.println(F("Calibrating... keep sensor flat & still."));

  long sAx=0,sAy=0,sAz=0,sGx=0,sGy=0,sGz=0;
  int16_t ax,ay,az,gx,gy,gz;

  for (int i = 0; i < CALIB_N; i++) {
    readRaw(ax,ay,az,gx,gy,gz);
    sAx+=ax; sAy+=ay; sAz+=az;
    sGx+=gx; sGy+=gy; sGz+=gz;
    delay(2);
  }

  offAx = sAx / (float)CALIB_N;
  offAy = sAy / (float)CALIB_N;
  offAz = sAz / (float)CALIB_N - 16384.0f;  // remove 1g from Z
  offGx = sGx / (float)CALIB_N;
  offGy = sGy / (float)CALIB_N;
  offGz = sGz / (float)CALIB_N;

  Serial.println(F("Done.\n"));
  digitalWrite(2, LOW);
}

// ── Setup ────────────────────────────────────────────────────
void setup() {
  pinMode(2, OUTPUT);
  Serial.begin(115200);
  Wire.begin(21, 22, 400000);

  mpuWrite(0x6B, 0x00);   // wake up
  mpuWrite(0x1B, 0x00);   // gyro  ±250 °/s  → scale = 131
  mpuWrite(0x1C, 0x00);   // accel ±2 g       → scale = 16384

  calibrate();

  prevMicros = micros();
  Serial.println(F("Roll\tPitch\tYaw"));

   radio.begin();
  //  radio.setAutoAck(false);
   radio.setRetries(1, 5);
   radio.openWritingPipe(address);
   radio.setPALevel(RF24_PA_MIN);
   radio.stopListening();
   radio.openWritingPipe(address);
   radio.setPALevel(RF24_PA_MIN);
   radio.stopListening();
}

// ── Loop ─────────────────────────────────────────────────────
void loop() {
  // ── delta time
  unsigned long now = micros();
  float dt = (now - prevMicros) * 1e-6f;
  prevMicros = now;

  // ── read
  int16_t ax, ay, az, gx, gy, gz;
  readRaw(ax, ay, az, gx, gy, gz);

  // ── remove offsets & scale
  float Ax = (ax - offAx) / 16384.0f;   // g
  float Ay = (ay - offAy) / 16384.0f;
  float Az = (az - offAz) / 16384.0f;
  float Gx = (gx - offGx) / 131.0f;    // °/s
  float Gy = (gy - offGy) / 131.0f;
  float Gz = (gz - offGz) / 131.0f;

  // ── accel-only angles
  float accelRoll  =  atan2(Ay, Az)                    * 57.2958f;
  float accelPitch = atan2(Ax, sqrt(Ay*Ay + Az*Az))   * 57.2958f;

  // ── complementary filter (roll & pitch)
  roll  = ALPHA * (roll  + Gx * dt) + (1.0f - ALPHA) * accelRoll;
  pitch = ALPHA * (pitch + Gy * dt) + (1.0f - ALPHA) * accelPitch;

  // ── gyro integration (yaw – drifts slowly without magnetometer)
  yaw += Gz * dt;

// ── keep roll in -180..180
  if (roll >  180.0f) roll -= 360.0f;
  if (roll < -180.0f) roll += 360.0f;

  // ── keep pitch in -180..180
  if (pitch >  180.0f) pitch -= 360.0f;
  if (pitch < -180.0f) pitch += 360.0f;


  // ── keep yaw in -180..180
  if (yaw >  180.0f) yaw -= 360.0f;
  if (yaw < -180.0f) yaw += 360.0f;

  // ── output (tab-separated → works in Serial Plotter too)
  Serial.print(roll,  2); Serial.print('\t');
  Serial.print(pitch, 2); Serial.print('\t');
  Serial.println(yaw, 2);

  myData.zAxis = yaw;
  myData.yAxis = pitch;
  
  radio.write(&myData, sizeof(Data_Package)); 
  

  delay(50);
}
