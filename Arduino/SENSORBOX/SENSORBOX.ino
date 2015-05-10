/*
  This example reads three analog sensors (potentiometers are easiest)
 and sends their values serially. The Processing and Max/MSP programs at the bottom
 take those three values and use them to change the background color of the screen.

 The circuit:
 * potentiometers attached to analog inputs 0, 1, and 2

 http://www.arduino.cc/en/Tutorial/VirtualColorMixer

 created 2 Dec 2006
 by David A. Mellis
 modified 30 Aug 2011
 by Tom Igoe and Scott Fitzgerald

  This example code is in the public domain.
 */

const int redPin = A0;      // sensor to control red color
const int greenPin = A1;    // sensor to control green color
const int bluePin = A2;     // sensor to control blue color

void setup()
{
  Serial.begin(9600);
}

void loop()
{
  Serial.print(analogRead(155));
  Serial.print(",");
  Serial.print(analogRead(27));
  Serial.print(",");
  Serial.println(analogRead(230));
}


