
#include <SPI.h>
#include <ADXL362.h>

ADXL362 xl;

int16_t temp;
int16_t XValue, YValue, ZValue, Temperature;
int16_t last_XValue, last_YValue, last_ZValue, last_Temperature;
float ax , ay, az;
void setup(){
  
  Serial.begin(9600);
  xl.begin(10);                   // Setup SPI protocol, issue device soft reset
  xl.beginMeasure();              // Switch ADXL362 to measure mode  
	
  ax = ay = az = 0.;
}

void loop(){
    
  // read all three axis in burst to ensure all measurements correspond to same sample time
  xl.readXYZTData(XValue, YValue, ZValue, Temperature);  
  ax = ( ax + ( float ) XValue / 100. ) * 0.5;
  ay = ( ay + ( float ) YValue / 100. ) * 0.5;
  az = ( az + ( float ) ZValue / 100. ) * 0.5;
  float g = sqrt( ax*ax + ay*ay + az*az );
  /*
  float roll = asin( ay / g );
  float pitch = acos( az / ( g * cos( asin( ay / g ) ) ) );
  */
  float pitch = ( atan2( -ay, az ) * 180. ) / M_PI;
  float roll = ( atan2(ax, sqrt( ay*ay + az*az ) ) * 180. ) / M_PI;
  
  // output data
  Serial.print("[euler,");
  Serial.print(0);
  Serial.print(",");
  Serial.print(roll);
  Serial.print(",");
  Serial.print(pitch);
  Serial.println("]");
  
  // check for commands
  if (Serial.available() > 0) {
    int command = Serial.read();
    switch ( command ) {
      case 'i' :
        Serial.println("[id,controller]");
        break;
    }
  }
  delay(10); //default delay(10);
}

