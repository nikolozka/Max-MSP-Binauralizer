/**
 * Streams calibrated sensors readings.
 *
 * @author Fabio Varesano - fvaresano at yahoo dot it
*/ 

/*
Development of this code has been supported by the Department of Computer Science,
Universita' degli Studi di Torino, Italy within the Piemonte Project
http://www.piemonte.di.unito.it/


This program is free software: you can redistribute it and/or modify
it under the terms of the version 3 GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/



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
#include "FreeIMU.h"
#include <Wire.h>
#include <SPI.h>

#define F_CPU = 16000000L;

const int arraySize = 11;
const int vals = 6;
float val[arraySize];

// Set the default object
FreeIMU my3IMU = FreeIMU();

void setup() { 
  Serial.begin(115200);
  
  pinMode(13, OUTPUT);

  Wire.begin();
  
  delay(500);
  my3IMU.init(0x68, true); // the parameter enable or disable fast mode
  delay(500);
}

void loop() {
  
  digitalWrite(13, HIGH);
  my3IMU.getValues(val);
  digitalWrite(13, LOW);
  Serial.flush();
   
  for(int i = 0; i < vals; i++){
    
    Serial.print(val[i], 4);    
    Serial.print(",");
  }   
  Serial.print('\n');
  delay(40);
  
}

