#include <ADXL345.h>
#include <bma180.h>
#include <HMC58X3.h>
#include <ITG3200.h>
#include <MS561101BA.h>
#include <I2Cdev.h>
#include <MPU60X0.h>
#include <EEPROM.h>

//#define DEBUG
#include "DebugUtils.h"
#include "CommunicationUtils.h"
#include "FreeIMU.h"
#include <Wire.h>
#include <SPI.h>

const int quatSize = 4;
float q[quatSize];

// Set the FreeIMU object
FreeIMU my3IMU = FreeIMU();

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  pinMode(13, OUTPUT);
  
  delay(5);
  my3IMU.init();
  delay(5);
}


void loop() { 
  //digitalWrite(13, HIGH);
  //my3IMU.getQ(q);
  //digitalWrite(13, LOW);
  //for(int i=0; i<quatSize; i++){
    //Serial.print(q[i], 4);
    //Serial.print(",  ");
  //}
  //Serial.print('\n');
  //delay(1000);
}

