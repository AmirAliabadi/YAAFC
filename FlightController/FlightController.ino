//#define DEBUG

#include <EEPROM.h>             //Include the EEPROM.h library so we can store information onto the EEPROM
#include <Wire.h>
//#include <Kalman.h> twice as slow as complementary filer

#define DEBUG
//#define UNIT_TEST_MODE

#include "FlightController.h"

byte aux_1_index, aux_2_index, aux_3_index, aux_4_index;

// input values from receiver
// ranges from 1000 to 2000 (ms)
int throttle_setpoint = 0;
int pitch_setpoint    = 0;
int roll_setpoint     = 0;
int yaw_setpoint      = 0;
int aux_1;
int aux_2;
int aux_3;
int aux_4;

// using a center stick throttle
// throttle value accumulates or decays bases on throttle_input
// throttle is a value of 1000 to 2000
int throttle = MIN_ESC_SIGNAL;

double dt = 0.0;
unsigned long  timer = 0;

#include "PPM.h"
#include "MPU.h"
#include "PID.h"
#include "ESC.h"


void setup() {
#ifdef DEBUG
  Serial.begin(57600);
#endif

  system_check = INIT_CLEARED;
      
  DDRB |= B00110000;                                           //ports 12 and 13 as output.

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

  ppm_channels[0] = 0;  // sync channel
  ppm_channels[1] = 1500;
  ppm_channels[2] = 1500;
  ppm_channels[3] = 1500;
  ppm_channels[4] = 1500;
  ppm_channels[5] = 1500;
  ppm_channels[6] = 1500;
  ppm_channels[7] = 1500;
  ppm_channels[8] = 1500;

  attachInterrupt(digitalPinToInterrupt(3), ppmRising, RISING);  // PPM input setup
#ifdef DEBUG
    delay(50);
    if( !ppm_sync ) Serial.println("Power on Transmitter");
#endif
  while( !ppm_sync ) {
    delay(10);        // wait for ppm sync
  }
  wait_for_initial_inputs();  // wait for all stick to be neutral

  init_mpu();
  calibrate();
  init_esc();
  init_pid();

  timer = micros();

#ifdef DEBUG
    Serial.println( "init_comleted" );
#endif
}

unsigned int guesture_count = 0;
float throttle_input_gain = 0.0;
float f_throttle = MIN_ESC_SIGNAL;

int c_trim_ticks = 0;
int c_trim = 0;

int yaw_trim = 0;

boolean calibartion_mode = false; // no pids, throttle max is 2000us
void loop() {
  digitalWrite(12,!digitalRead(12));  

  dt = (double)(micros() - timer) / 1000000; // Calculate delta time
  timer = micros();

  cli(); roll_setpoint       = ppm_channels[1] ;  sei(); // Read ppm channel 1
  cli(); pitch_setpoint      = ppm_channels[2] ;  sei(); // Read ppm channel 2
  cli(); throttle_setpoint   = ppm_channels[3] ;  sei(); // Read ppm channel 3
  cli(); yaw_setpoint        = ppm_channels[4] ;  sei(); // Read ppm channel 4
  cli(); aux_1               = ppm_channels[5] ;  sei(); // Read ppm channel 5
  cli(); aux_2               = ppm_channels[6] ;  sei(); // Read ppm channel 6
  cli(); aux_3               = ppm_channels[7] ;  sei(); // Read ppm channel 7
  cli(); aux_4               = ppm_channels[8] ;  sei(); // Read ppm channel 8

  // 20us of deadband
  if( pitch_setpoint    >= 1490 && pitch_setpoint     <= 1510 ) pitch_setpoint    = 1500;
  if( roll_setpoint     >= 1490 && roll_setpoint      <= 1510 ) roll_setpoint     = 1500;  
  if( throttle_setpoint >= 1490 && throttle_setpoint  <= 1510 ) throttle_setpoint = 1500;  
  if( yaw_setpoint      >= 1490 && yaw_setpoint       <= 1510 ) yaw_setpoint      = 1500;  
  if( aux_4             >= 1390 && aux_4              <= 1610 ) aux_4             = 1500; 

  if( aux_1 < 1150 ) aux_1_index = 0;
  else if( aux_1 > 1850 ) aux_1_index = 2;
  else aux_1 = aux_1_index = 1;

  if( aux_2 < 1150 ) aux_2_index = 0;
  else if( aux_2 > 1850 ) aux_2_index = 2;
  else aux_2_index = 1;    

  if( aux_3 < 1150 ) aux_3_index = 0;
  else if( aux_3 > 1850 ) aux_3_index = 2;
  else aux_3_index = 1;  


  ////////////////////////////////////////////////////////////////
  // PID Tuning via AUX channels

  ////////////////////////////////////////////////////////////////
  // use trim wheel input to trim the yaw 
  if( aux_4 != 1500 ) c_trim_ticks += 1;
  else {
    c_trim_ticks = 0;
    c_trim = 0;
  }
  
  if( c_trim_ticks > 100 ) {
    if( aux_4 < 1500) c_trim -= 1;
    else if( aux_4 > 1500 ) c_trim += 1;
  }
  //
  ////////////////////////////////////////////////////////////////

  if( aux_3_index == 2 ) {
    if( c_trim_ticks > 100 ) {
      if ( aux_1_index == 0 && aux_2_index == 0 ) {
          if( c_trim_ticks > 100 ) yaw_trim += c_trim ;
          if( yaw_trim >  150 )  yaw_trim =  150;
          if( yaw_trim < -150 )  yaw_trim = -150;
#ifdef DEBUG        
          Serial.print("yaw trim"); Serial.print("\t"); Serial.println(yaw_trim);   
#endif
        } else 
      if ( aux_1_index == 0 && aux_2_index == 1 ) {
#ifdef DEBUG      
          Serial.print("roll trim"); Serial.print("\t"); Serial.println(c_trim);   
#endif        
        } else 
      if ( aux_1_index == 0 && aux_2_index == 2 ) {
#ifdef DEBUG      
          Serial.print("pitch trim"); Serial.print("\t"); Serial.println(c_trim);  
#endif                  
      } else 
      if ( aux_1_index == 1 && aux_2_index == 0 ) {
          if( c_trim_ticks > 100 ) attitude_pTerm += c_trim/100.0;
          if( attitude_pTerm > 5 )  attitude_pTerm = 5;
          if( attitude_pTerm < 0 )  attitude_pTerm = 0;
#ifdef DEBUG         
          Serial.print("attitude pid: pTerm"); Serial.print("\t"); Serial.println(attitude_pTerm);  
#endif                  
        } else 
      if ( aux_1_index == 1 && aux_2_index == 1 ) {
#ifdef DEBUG      
          Serial.print("attitude pid: iTerm"); Serial.print("\t"); Serial.println(c_trim); 
#endif
        } else 
      if ( aux_1_index == 1 && aux_2_index == 2 ) {
#ifdef DEBUG      
          Serial.print("attitude pid: dTerm"); Serial.print("\t"); Serial.println(c_trim);  
#endif
      } else 
      if ( aux_1_index == 2 && aux_2_index == 0 ) {
          if( c_trim_ticks > 100 ) rate_pid_gains[0] += c_trim/100.0;
          if(  rate_pid_gains[0] > 5 )   rate_pid_gains[0] = 5;
          if(  rate_pid_gains[0] < 0 )   rate_pid_gains[0] = 0;
#ifdef DEBUG        
          Serial.print("rate pid: pTerm"); Serial.print("\t"); Serial.println(rate_pid_gains[0]);            
#endif        
      } else 
      if ( aux_1_index == 2 && aux_2_index == 1 ) {
          if( c_trim_ticks > 100 ) rate_pid_gains[1] += c_trim/100.0;
          if(  rate_pid_gains[1] > 5 )   rate_pid_gains[1] = 5;
          if(  rate_pid_gains[1] < 0 )   rate_pid_gains[1] = 0;
#ifdef DEBUG        
          Serial.print("rate pid: iTerm"); Serial.print("\t"); Serial.println(rate_pid_gains[1]);            
#endif        
      } else 
      if ( aux_1_index == 2 && aux_2_index == 2 ) {
          if( c_trim_ticks > 100 ) rate_pid_gains[2] += c_trim/100.0;
          if(  rate_pid_gains[2] > 20 )  rate_pid_gains[2] = 20;
          if(  rate_pid_gains[2] < 0 )   rate_pid_gains[2] = 0;
#ifdef DEBUG        
          Serial.print("rate pid: dTerm"); Serial.print("\t"); Serial.println(rate_pid_gains[2]);     
#endif               
      }   
    } 
  } else {
    // back to defaults
    reset_to_defaults();
  }

  if( c_trim_ticks > 100 ) {
    c_trim_ticks = 0; 
  }  
  //
  ////////////////////////////////////////////////////////////////
  //
  ////////////////////////////////////////////////////////////////

  if( throttle <= MIN_ESC_CUTOFF ) {
    // Look for ESC Arm/Disarm gestures
    if( throttle_setpoint < 1050 && yaw_setpoint < 1050 && !(system_check & INIT_ESC_ARMED) ) {
        throttle = 0;
        pid_reset();
        gyro_pitch = 0; 
        gyro_roll = 0; 
        gyro_yaw = 0;
        arm_esc();         
    } else if ( throttle_setpoint < 1050 && yaw_setpoint > 1850 && (system_check & INIT_ESC_ARMED) ) {
        disarm_esc();      
    }
  }

  // safety/testing
  // only yaw when throttle is nuteral
  if( throttle_setpoint != 1500 ) {
    yaw_setpoint = 1500;
  }  
  // safety/testing

  // adjust so 1500 = Zero input
  throttle_setpoint = (throttle_setpoint - 1500) ;
  pitch_setpoint    = (pitch_setpoint    - 1500) * -1.0; // inverted signal from TX
  roll_setpoint     = (roll_setpoint     - 1500) ;
  yaw_setpoint      = (yaw_setpoint      - 1500) ;   

  digitalWrite(12,HIGH);

  // read_mpu_process();      // 784us : gyro+accel raw reads with LPF : moved the MPU read process to the ESC PWM 1000us idle time
  mpu_conversion_process();   // 544us : convert to degress/sec and calculate angles using complinetary filter.
  
  digitalWrite(12,LOW);

  if( throttle_setpoint <= -470 ) throttle = 0;

  throttle_input_gain = throttle_setpoint / 600.0;

  if( calibartion_mode ) {
    
    throttle = (int)( f_throttle += throttle_input_gain );

    if( f_throttle > MAX_ESC_SIGNAL ) f_throttle = MAX_ESC_SIGNAL;
    if( f_throttle < MIN_ESC_SIGNAL ) f_throttle = MIN_ESC_SIGNAL;    

    if( throttle > MAX_ESC_SIGNAL ) throttle = MAX_ESC_SIGNAL;
    if( throttle < MIN_ESC_SIGNAL ) throttle = MIN_ESC_SIGNAL;    

    va = throttle ; // front right - CCW
    vb = throttle ; // front left  -  CW
    vc = throttle ; // back left   - CCW
    vd = throttle ; // back right  -  CW   

#ifdef DEBUG
    Serial.print("Calibration Mode: ");
    Serial.print( va ); Serial.print( "\t" ); Serial.print( vb );
    Serial.print( "\t" );
    Serial.print( vc ); Serial.print( "\t" ); Serial.println( vd );
#endif
    
  } else  if( system_check & INIT_ESC_ARMED ) {

    throttle = (int)( f_throttle += throttle_input_gain );

    if( f_throttle > MAX_ESC_SIGNAL ) f_throttle = MAX_ESC_SIGNAL;
    if( f_throttle < MIN_ESC_CUTOFF ) f_throttle = MIN_ESC_CUTOFF;    

    if( throttle > MAX_ESC_SIGNAL ) throttle = MAX_ESC_SIGNAL;
    if( throttle < MIN_ESC_CUTOFF ) throttle = MIN_ESC_CUTOFF;

    // DO PID CALCUATIONS
    do_pid_compute();   // 200us

    // READ BATTERY LEVEL
    // TODO:

    if (throttle > 1800) throttle = 1800;     // leave room for the PID controllers

    // DO MOTOR MIX ALGORITHM : X Setup
    va = throttle - pitch_pid_rate_out + roll_pid_rate_out - yaw_pid_rate_out; // front right - CCW
    vb = throttle + pitch_pid_rate_out + roll_pid_rate_out + yaw_pid_rate_out; // front left  -  CW
    vc = throttle + pitch_pid_rate_out - roll_pid_rate_out - yaw_pid_rate_out; // back left   - CCW
    vd = throttle - pitch_pid_rate_out - roll_pid_rate_out + yaw_pid_rate_out; // back right  -  CW

    va += yaw_trim;
    vc += yaw_trim;
    vb -= yaw_trim;
    vd -= yaw_trim;

#ifdef DEBUG
    Serial.println( attitude_pTerm * (roll_angle - roll_setpoint/15.0) );

    // pitch test
    // Serial.print( va ); Serial.print( "\t" ); Serial.println( vd );    // stick pitch input - nose up, should increase, gryo up should decrease
    // Serial.print( vb ); Serial.print( "\t" ); Serial.println( vc );    // stick pitch input - nose up, should decrease, gryo up should increase

    // roll test
    // Serial.print( va ); Serial.print( "\t" ); Serial.println( vb );  // stick roll input - right down, should decrease, gyro right should increase
    // Serial.print( vc ); Serial.print( "\t" ); Serial.println( vd );  // stick roll input - right down, should increase, gyro right should decrease

    // yaw test
    // Serial.print( va ); Serial.print( "\t" ); Serial.println( vc );     // stick yaw right - increase  , hard left increase
    // Serial.print( vb ); Serial.print( "\t" ); Serial.println( vd );     // stick yaw left - decrease  
#endif

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

#ifdef DEBUG
  // Serial.println( accl_read[2] );

  //Serial.print( pitch_angle ); Serial.print( "\t" ); Serial.println( roll_angle  );
  //Serial.print( pitch_angle ); Serial.print( "\t" ); Serial.println( gyro_pitch  );

/*
  Serial.print( gyro_pitch - ((pitch_angle - pitch_setpoint/15.0) * .5 ) );
  Serial.print( "\t" ); 
  Serial.println( gyro_roll - ((roll_angle - roll_setpoint/15.0) * .5 ) );
*/  

/*
  Serial.print( va ); Serial.print( "\t" ); Serial.print( vb );     
  //Serial.println("");
  Serial.print( "\t" ); 
  Serial.print( vc ); Serial.print( "\t" ); Serial.println( vd ); 
*/

/*
  Serial.print( roll_setpoint ); Serial.print("\t");
  Serial.print( pitch_setpoint ); Serial.print("\t");
  Serial.print( throttle_setpoint ); Serial.print("\t");
  Serial.print( yaw_setpoint ); Serial.print("\t");
  Serial.print( aux_1 ); Serial.print("\t");
  Serial.print( aux_2 ); Serial.print("\t");
  Serial.print( aux_3 ); Serial.print("\t");
  Serial.println( aux_4 ); 

  Serial.print( aux_3 );
  Serial.print( "\t" );
  Serial.print( f_yaw_trim );
  Serial.print( "\t" );
  Serial.println( yaw_trim );  
*/  
#endif

#ifdef DEBUG
 // Serial.print( gyro_read_z );
 // Serial.print( "\t" );
 // Serial.print( accl_read_z );
 // Serial.print( "\t" );

/* 
  Serial.print( gyro_yaw );
  Serial.print( "\t" );
  Serial.print( yaw_setpoint );
  Serial.print( "\t" );
  Serial.print( va ); Serial.print( "\t" ); Serial.print( vc );     // stick yaw right - increase  , hard left increase
  Serial.print( "\t" ); 
  Serial.print( vb ); Serial.print( "\t" ); Serial.println( vd );     // stick yaw left - decrease    
*/  
 
#endif
  
  update_motors();

}

