#define MPU6050_RA_PWR_MGMT_1       0x6B

#define MPU6050_RA_SMPLRT_DIV       0x19

#define MPU6050_RA_CONFIG           0x1A
#define MPU6050_RA_GYRO_CONFIG      0x1B
#define MPU6050_RA_ACCEL_CONFIG     0x1C

#define MPU6050_RA_I2C_MST_CTRL     0x24

#define MPU6050_GYRO_FS_250         0x00
#define MPU6050_GYRO_FS_500         0x01
#define MPU6050_GYRO_FS_1000        0x02
#define MPU6050_GYRO_FS_2000        0x03

#define MPU6050_ACCEL_FS_2          0x00
#define MPU6050_ACCEL_FS_4          0x01
#define MPU6050_ACCEL_FS_8          0x02
#define MPU6050_ACCEL_FS_16         0x03

#define MPU6050_RA_ACCEL_XOUT_H     0x3B
#define MPU6050_RA_ACCEL_XOUT_L     0x3C
#define MPU6050_RA_ACCEL_YOUT_H     0x3D
#define MPU6050_RA_ACCEL_YOUT_L     0x3E
#define MPU6050_RA_ACCEL_ZOUT_H     0x3F
#define MPU6050_RA_ACCEL_ZOUT_L     0x40

#define MPU6050_RA_TEMP_OUT_H       0x41
#define MPU6050_RA_TEMP_OUT_L       0x42

#define MPU6050_RA_GYRO_XOUT_H      0x43
#define MPU6050_RA_GYRO_XOUT_L      0x44
#define MPU6050_RA_GYRO_YOUT_H      0x45
#define MPU6050_RA_GYRO_YOUT_L      0x46
#define MPU6050_RA_GYRO_ZOUT_H      0x47
#define MPU6050_RA_GYRO_ZOUT_L      0x48

#define MPU6050_RA_FIFO_EN          0x23

int  pit_inverse = -1;
int  rol_inverse = -1;
int  yaw_inverse = -1;

double gyro_read_x, gyro_read_y, gyro_read_z;
double gyro_pitch = 0.0;
double gyro_roll  = 0.0;
double gyro_yaw   = 0.0;

double accl_read_x, accl_read_y, accl_read_z;

double tmpr_read;

float x_angle = 0.0;
float y_angle = 0.0;
double pitch_angle = 0.0;
double roll_angle = 0.0;

double gyro_offsets_x = 0.0;
double gyro_offsets_y = 0.0;
double gyro_offsets_z = 0.0;

double pitch_angle_offset = 0.0;
double roll_angle_offset  = 0.0;

// kalmanFilter
//double kalAngleX = 0.0;
//double kalAngleY = 0.0;

int mpu_address = 0x68;

/* MPU 6050 Orientation:

front left          front right  (CCW)
(CW)
           +------+
           |( )  .|--- Int
           |     .|
           |     .|--- sda
           |     .|--- scl
           |( )  .|--- gnd
           +------+--- vcc

back left          back right (CW)
(CCW)

Roll Right  = - Gryo
Roll Left   = + Gyro
Pitch Up    = - Gyro
Pitch Down  = + Gyro
Yaw Right   = + Gyro
Yaw Left    = - Gyro
*/


void init_mpu() {
#ifdef DEBUG
    Serial.println( "init_mpu" );
#endif   
  
    system_check &= ~(INIT_MPU_ENABLED | INIT_MPU_STABLE);

    Wire.beginTransmission(mpu_address);                        
    Wire.write(0x6B);                           // PWR_MGMT_1 register 
    Wire.write(B00001000);                      // Internal 8mhz Clock, Temperature Disabled, Sleep Mode disable.  Bit 7 will cause a reset
    Wire.endTransmission();                                      
        
    Wire.beginTransmission(mpu_address);              
    Wire.write(0x1B);                            // Set MPU6050_RA_GYRO_CONFIG 
    Wire.write(B00001000);                       // Gyro Full Scale FS_SEL,  +/- 500 */S  : 012=unused, 34=FS_SEL, 567=Self_Test         
    Wire.endTransmission();                      // FS_SEL : 0 = +/-250 */s, 1 = +/- 500 */s, 2 = +/- 1000 */s, 3 = +/- 2000 */s      
// FS_SEL Full Scale Range LSB Sensitivity
// 0 ±  250 °/s 131  LSB/°/s 
// 1 ±  500 °/s 65.5 LSB/°/s
// 2 ± 1000 °/s 32.8 LSB/°/s
// 3 ± 2000 °/s 16.4 LSB/°/s       

    Wire.beginTransmission(mpu_address);          
    Wire.write(0x1C);                            // Set MPU6050_RA_ACCEL_CONFIG 
    Wire.write(B00001000);                       // Accelerometer Full Scale - AFS_SEL, +/- 4g   : 012=unused, 34=AFS_SEL, 567=Self_Test    
    Wire.endTransmission();                      // AFS_SEL : 0 = +/- 2g, 1 = +/- 4g, 2 = +/- 8g, 3 = +/- 16g 
// AFS_SEL Full Scale Range
// 0 ± 2g
// 1 ± 4g
// 2 ± 8g
// 3 ± 16g    

    Wire.beginTransmission(mpu_address);          
    Wire.write(0x23);                            // MPU6050_RA_FIFO_EN
    Wire.write(B00000000);                       // disable fifo buffer for slv0, slv1, slv2, ACCEL, Zgyro, Ygyro, Xgyro, Temp
    Wire.endTransmission();                           

    Wire.beginTransmission(mpu_address);          
    Wire.write(0x1A);                           // MPU6050_RA_CONFIG 
  //Wire.write(B00000000);                      // 0 No DLPF, No External Sync
  //Wire.write(B00000001);                      // 1 DLPF, Accel @ 184hz and Gyro @ 188hz. No External Sync
  //Wire.write(B00000010);                      // 2 DLPF, Accel @  94hz and Gyro @  98hz. No External Sync
    Wire.write(B00000011);                      // 3 DLPF, Accel @  44hz and Gyro @  42hz. No External Sync
  //Wire.write(B00000100);                      // 4 DLPF, Accel @  21hz and Gyro @  20hz. No External Sync
  //Wire.write(B00000101);                      // 5 DLPF, Accel @  10hz and Gyro @  10hz. No External Sync    
  //Wire.write(B00000110);                      // 6 DLPF, Accel @   5hz and Gyro @   5hz. No External Sync
    Wire.endTransmission();      

    delay(100);

    system_check |= INIT_MPU_ENABLED;            
}


// just gyro only reads: 520us
// just accelerometer reads: 408us
// gyro + accelerometer + temp as a single reads: 607us
// gyro+accel raw reads with LPF : 784us
void read_mpu_process() {
  Wire.beginTransmission(mpu_address);          
  Wire.write(0x3B);                             //Start reading from register MPU6050_RA_ACCEL_XOUT_H 0x3B 
  Wire.endTransmission();                       
  Wire.requestFrom(mpu_address, 14);             //Request 6 bytes from the gyro

  // there is 4uS of time here.
  while(Wire.available() < 14);                 //Wait until the 14 bytes are received.
  
  accl_read_x = (Wire.read()<<8|Wire.read());    //Add the low and high byte to the acc_x variable.
  accl_read_y = (Wire.read()<<8|Wire.read());    //Add the low and high byte to the acc_y variable.
  accl_read_z = (Wire.read()<<8|Wire.read());    //Add the low and high byte to the acc_z variable.
  tmpr_read   = (Wire.read()<<8|Wire.read());    //Add the low and high byte to the temperature variable.
  gyro_read_x = (Wire.read()<<8|Wire.read());    //Read high and low part of the gryo_x data.
  gyro_read_y = (Wire.read()<<8|Wire.read());    //Read high and low part of the gryo_y data.
  gyro_read_z = (Wire.read()<<8|Wire.read());    //Read high and low part of the gryo_z data.

  // Apply offset to gyro_read.  gyro_offsets defaults to zero
  // and is only <> 0.0 once calibration is done.
  gyro_read_x  = (gyro_read_x  - gyro_offsets_x) ; 
  gyro_read_y  = (gyro_read_y  - gyro_offsets_y) ; 
  gyro_read_z  = (gyro_read_z  - gyro_offsets_z) ; 

  //accl_read[0] *= pit_inverse;
  //accl_read[1] *= rol_inverse;
  //accl_read[2] *= yaw_inverse;

  if( system_check & INIT_ESC_ARMED ) {
    // convert to degrees/sec, with LPF 
    // not sure why but this helps even when the 6050 has the DLPF enabled.
    gyro_pitch  = (gyro_pitch * 0.8)  + (( (gyro_read_x * pit_inverse) / 65.5) * 0.2); 
    gyro_roll   = (gyro_roll  * 0.8)  + (( (gyro_read_y * rol_inverse) / 65.5) * 0.2); 
    gyro_yaw    = (gyro_yaw   * 0.8)  + (( (gyro_read_z * yaw_inverse) / 65.5) * 0.2); 
  } else {
    // convert to degrees/sec, no LPF
    gyro_pitch  = ((gyro_read_x * pit_inverse) / 65.5);
    gyro_roll   = ((gyro_read_y * rol_inverse) / 65.5);
    gyro_yaw    = ((gyro_read_z * yaw_inverse) / 65.5);      
  }  

// roll right, gryo increase
// pitch up, gyro increase
// yaw right, gyro increase

  //Serial.println(gyro_pitch);
  //Serial.println(gyro_roll);
  //Serial.println(gyro_yaw);
  
}

// 544us : complementary filter
// Kalman was something like 1.2ms
void mpu_conversion_process() {

  /// ------------------------------------------------------------------------------------------
  // PITCH Restricted, calculate angles from accel data
  //x_angle = atan2( accl_read[1], accl_read[2] ) * RAD_TO_DEG;
  x_angle = atan(  accl_read_y / sqrt( (accl_read_x * accl_read_x) + (accl_read_z * accl_read_z) ) ) * RAD_TO_DEG;
  y_angle = atan( -accl_read_x / sqrt( (accl_read_y * accl_read_y) + (accl_read_z * accl_read_z) ) ) * RAD_TO_DEG;

  //x_angle = 57.295*atan((float) accl_read[1]/ sqrt(pow((float)accl_read[2],2)+pow((float)accl_read[0],2)));
  //y_angle = 57.295*atan((float)-accl_read[0]/ sqrt(pow((float)accl_read[2],2)+pow((float)accl_read[1],2))); 

  // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees  
  if ( x_angle < -90 || x_angle > 90 ) {
    pitch_angle = x_angle;
    //gyroXangle = roll;
  }  

  /// ------------------------------------------------------------------------------------------
  /// use a comlementary filter to compute a more reliable x/y angle by using gyro data + accel data
  pitch_angle = 0.93 * (pitch_angle + gyro_pitch * dt) + 0.07 * x_angle; // Calculate the pitch_angle using a complementary filter
  roll_angle  = 0.93 * (roll_angle  + gyro_roll  * dt) + 0.07 * y_angle;  

  pitch_angle = (pitch_angle - pitch_angle_offset) ;// * compAngleX_Invert;
  roll_angle  = (roll_angle  - roll_angle_offset ) ;// * compAngleY_Invert;

  //kalAngleX = kalmanX.getAngle(x_angle, gyro[0], dt);
  //kalAngleY = kalmanY.getAngle(y_angle, gyro[1], dt);     
}


/*
void mpu_conversion_process() {

// ACCEL_XOUT = ((ACCEL_XOUT_H<<8)|ACCEL_XOUT_L);
// ACCEL_YOUT = ((ACCEL_YOUT_H<<8)|ACCEL_YOUT_L);
// ACCEL_ZOUT = ((ACCEL_ZOUT_H<<8)|ACCEL_ZOUT_L);
// if(ACCEL_XOUT>32767) ACCEL_XOUT = ACCEL_XOUT-65536;
// if(ACCEL_YOUT>32767) ACCEL_YOUT = ACCEL_YOUT-65536;
// if(ACCEL_ZOUT>32767) ACCEL_ZOUT = ACCEL_ZOUT-65536;
//
// if(accl_read[0]>32767) accl_read[0] = accl_read[0]-65536;
// if(accl_read[1]>32767) accl_read[1] = accl_read[1]-65536;
// if(accl_read[2]>32767) accl_read[2] = accl_read[2]-65536;
//////////////////////////////////////////

//#define SYS_FREQ 40000000
//#define PB_DIV 8
//#define PRESCALE 256
//#define T1_TICK (SYS_FREQ/PB_DIV/PRESCALE/100)
//#define dt .01                                                                                                                                                                      
//#define g 8192

//#define gyro_x_sensitivity 131 //66.5 Dead on at last check
//#define gyro_y_sensitivity 131 //72.7 Dead on at last check
//#define gyro_z_sensitivity 131
  //GYRO_XRATE = (float)GYRO_XOUT/gyro_x_sensitivity;
  //GYRO_YRATE = (float)GYRO_YOUT/gyro_y_sensitivity;
  //GYRO_ZRATE = (float)GYRO_ZOUT/gyro_z_sensitivity;
  
  //GYRO_XANGLE += GYRO_XRATE*dt;
  //GYRO_YANGLE += GYRO_YRATE*dt;
  //GYRO_ZANGLE += GYRO_ZRATE*dt;

  if( system_check & INIT_ESC_ARMED ) {
    // convert to degrees/sec
    // apply low pass filter, not sure why but this helps even when the 6050 has the DLPF enabled.
    gyro[0] = (gyro[0] * 0.8) + ((gyro_read[0] / 65.5) * 0.2); 
    gyro[1] = (gyro[1] * 0.8) + ((gyro_read[1] / 65.5) * 0.2); 
    gyro[2] = (gyro[2] * 0.8) + ((gyro_read[2] / 65.5) * 0.2); 
  } else {
    // convert to degrees/sec
    gyro[0] = (gyro_read[0] / 65.5);
    gyro[1] = (gyro_read[1] / 65.5);
    gyro[2] = (gyro_read[2] / 65.5);      
  }

  //ACCEL_XANGLE = 57.295*atan((float)ACCEL_YOUT/ sqrt(pow((float)ACCEL_ZOUT,2)+pow((float)ACCEL_XOUT,2)));
  //ACCEL_YANGLE = 57.295*atan((float)-ACCEL_XOUT/ sqrt(pow((float)ACCEL_ZOUT,2)+pow((float)ACCEL_YOUT,2)));
  // this adds 500us of overhead!!!
//  x_angle = 57.295*atan((float)accl_read[1]/ sqrt(pow((float)accl_read[2],2)+pow((float)accl_read[0],2)));
//  y_angle = 57.295*atan((float)-accl_read[0]/ sqrt(pow((float)accl_read[2],2)+pow((float)accl_read[1],2)));  
  // this adds 500us of overhead!!!
  
}
*/


// gyro offsets: -21.06  -10.04   45.00
// gyro offsets: -22.36   -9.63   45.33
void calibrate_gyro() {
#ifdef DEBUG
    Serial.println( "calibrate_gyro" );
#endif
  
  double temp_gyro_offsets_x = 0.0;
  double temp_gyro_offsets_y = 0.0;
  double temp_gyro_offsets_z = 0.0;
 
  for(int i=0; i<2000; i++ ) {
    read_mpu_process();
    temp_gyro_offsets_x += gyro_read_x;
    temp_gyro_offsets_y += gyro_read_y;
    temp_gyro_offsets_z += gyro_read_z;
    delay(1);
  }
  
  gyro_offsets_x = temp_gyro_offsets_x / 2000.0;
  gyro_offsets_y = temp_gyro_offsets_y / 2000.0;
  gyro_offsets_z = temp_gyro_offsets_z / 2000.0;
  
}

// compAngle Offsets: 0.07 -0.01
// compAngle Offsets: 0.07 -0.01
void calibrate_accl() {
#ifdef DEBUG
    Serial.println( "calibrate_accl" );
#endif

//  pitch_angle_offset = 0.07;
//  roll_angle_offset = -0.01;
//  return;

  // delay(10000); // wait for things to stabalize
  for( int i=0; i<15000; i++ ) {
    read_mpu_process();
  }

  // take a reading
  read_mpu_process();
  mpu_conversion_process();     

  // use that as the complementary angle offset
  // best plan here is to do this once and save it in the eeprom
  pitch_angle_offset = pitch_angle;
  roll_angle_offset = roll_angle;    
 
}

void calibrate() {
  calibrate_gyro();
  calibrate_accl();

  system_check |= INIT_MPU_STABLE;
#ifdef DEBUG
    Serial.print( "gyro offsets: " );
    Serial.print(  gyro_offsets_x );
    Serial.print(  "\t" );
    Serial.print(  gyro_offsets_y );
    Serial.print(  "\t" );
    Serial.println(  gyro_offsets_z );

    Serial.print( "compAngle Offsets: " );
    Serial.print( pitch_angle_offset );
    Serial.print( "\t" );
    Serial.println( roll_angle_offset );    
    
    Serial.println( "calibrate done" );

    delay(5000);
#endif
}
