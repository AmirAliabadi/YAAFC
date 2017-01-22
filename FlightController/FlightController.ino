//#define DEBUG

#include <EEPROM.h>             //Include the EEPROM.h library so we can store information onto the EEPROM
#include <Wire.h>

#define DEBUG
#include "FlightController.h"
#include "PPM.h"
#include "ESC.h"
#include "MPU.h"

// input values from receiver
// ranges from 1000 to 2000 (ms)
int throttle_input = 0;
int pitch_input = 0;
int roll_input = 0;
int yaw_input = 0;

// using a center stick throttle
// throttle value accumulates or decays bases on throttle_input
// throttle is a value of 1000 to 2000
int throttle = MIN_ESC_SIGNAL;

#include "PID.h"

void setup() {
#ifdef DEBUG
  Serial.begin(57600);
#endif
  Serial.begin(57600);
  
  DDRB |= B00110000;                                           //ports 12 and 13 as output.

  system_check = INIT_CLEARED;

/*
  EEPROM.get(0, eeprom_data);
  if(eeprom_data.id[0] != 'A' && eeprom_data.id[1] != 'A') {
#ifdef DEBUG
  Serial.println("Please run Calibration");
#endif
    while(1) {
      digitalWrite(13, !digitalRead(13));
      delay(10);
      digitalWrite(13, !digitalRead(13));
      delay(100);      
    }
  } 
*/  

  Wire.begin();
  Wire.setClock(400000L);   // i2c at 400k Hz

  attachInterrupt(digitalPinToInterrupt(3), ppmRising, RISING);  // PPM input setup

  while( !ppm_sync ) ; // wait for ppm sync
  wait_for_initial_inputs(); // wait for all stick to be neutral

  init_mpu();
  calibrate_gyro();
  init_esc();
  init_pid();

  gyro_lpf = true;
}

unsigned int guesture_count = 0;
float throttle_input_gain = 0.0;
float f_throttle = MIN_ESC_SIGNAL;
void loop() {

  pitch_input    = ppm_channels[1] ;  // Read ppm channel 1
  roll_input     = ppm_channels[2] ;  // Read ppm channel 2
  throttle_input = ppm_channels[3] ;  // Read ppm channel 3
  yaw_input      = ppm_channels[4] ;  // Read ppm channel 4

  // 20us of deadband
  if( pitch_input >= 1490 && pitch_input <= 1510 ) pitch_input = 1500;
  if( roll_input >= 1490 && roll_input <= 1510 ) roll_input = 1500;  
  if( throttle_input >= 1490 && throttle_input <= 1510 ) throttle_input = 1500;  
  if( yaw_input >= 1490 && yaw_input <= 1510 ) yaw_input = 1500;  

  ////////////////////////////////////////////////////////////////////
  // Simple Arm/Disarm.  Hold throttle at lowest position for 500ms
  if( throttle <= MIN_ESC_CUTOFF ) {
    // Look for ESC Arm/Disarm gestures
    if( throttle_input < 1050 ) {
        guesture_count ++;
    } else {
      guesture_count = 0;
    }

    if( guesture_count >= 500 ) {
      if( system_check & INIT_ESC_ARMED ) {
        disarm_esc();
      } else {
        throttle = 0;
        pid_reset();
        gyro[0] = 0; 
        gyro[1] = 0; 
        gyro[2] = 0;
        arm_esc();      
      }    
      guesture_count = 0;
    }
  }
  // Simple Arm/Disarm. 
  ////////////////////////////////////////////////////////////////////

  // adjust so 1500 = Zero input
  throttle_input = throttle_input - 1500 ;
  pitch_input    = pitch_input - 1500 ;
  roll_input     = roll_input - 1500 ;
  yaw_input      = yaw_input - 1500 ;   

  digitalWrite(12,HIGH);
                                    // 250Hz with no i2cdevlib and no DMP  
                                    // With i2cdevlib and DMP enabled this is about 100 Hz
  read_mpu_process();               // Just the gyro read: 300uS 
                                    // 500us read with lpf and offsets
                                    // With Accel reads, 800ms
  digitalWrite(12,LOW);

  throttle_input_gain = throttle_input / 600.0;

  if( system_check & INIT_ESC_ARMED ) {

    throttle = ( f_throttle += throttle_input_gain );

    if( f_throttle > MAX_ESC_SIGNAL ) f_throttle = MAX_ESC_SIGNAL;
    if( f_throttle < MIN_ESC_CUTOFF ) f_throttle = MIN_ESC_CUTOFF;    

    if( throttle > MAX_ESC_SIGNAL ) throttle = MAX_ESC_SIGNAL;
    if( throttle < MIN_ESC_CUTOFF ) throttle = MIN_ESC_CUTOFF;

    // DO PID CALCUATIONS
    do_pid_compute();   // 200us

    // READ BATTERY LEVEL
    // TODO:

    // DO MOTOR MIX ALGORITHM : X Setup
    va = throttle - pitch_pid_rate_out + roll_pid_rate_out - yaw_pid_rate_out; // front right - CCW
    vb = throttle + pitch_pid_rate_out + roll_pid_rate_out + yaw_pid_rate_out; // front left  -  CW
    vc = throttle + pitch_pid_rate_out - roll_pid_rate_out - yaw_pid_rate_out; // back left   - CCW
    vd = throttle - pitch_pid_rate_out - roll_pid_rate_out + yaw_pid_rate_out; // back right  -  CW

    // Serial.print( va ); Serial.print( "\t" ); Serial.println( vb );
    // Serial.print( vc ); Serial.print( "\t" ); Serial.println( vd ); 
    // Serial.print( va ); Serial.print( "\t" ); Serial.println( vd ); 
    // Serial.print( vb ); Serial.print( "\t" ); Serial.println( vc ); 

    if( va < MIN_ESC_CUTOFF ) va = MIN_ESC_CUTOFF;
    if( vb < MIN_ESC_CUTOFF ) vb = MIN_ESC_CUTOFF;
    if( vc < MIN_ESC_CUTOFF ) vc = MIN_ESC_CUTOFF;
    if( vd < MIN_ESC_CUTOFF ) vd = MIN_ESC_CUTOFF;

    if( va > MAX_ESC_SIGNAL ) va = MAX_ESC_SIGNAL;
    if( vb > MAX_ESC_SIGNAL ) vb = MAX_ESC_SIGNAL;
    if( vc > MAX_ESC_SIGNAL ) vc = MAX_ESC_SIGNAL;
    if( vd > MAX_ESC_SIGNAL ) vd = MAX_ESC_SIGNAL;
    
  } else {
    
    pid_reset();
    
    va = MIN_ESC_SIGNAL;
    vb = MIN_ESC_SIGNAL;
    vc = MIN_ESC_SIGNAL;
    vd = MIN_ESC_SIGNAL;
        
  }

  update_motors();

}

