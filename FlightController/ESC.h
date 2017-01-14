#define PWM_FERQUENCY 4000

unsigned long last_pwm_pulse = 0;
unsigned long esc_pwm_timmer = 0;

unsigned long timer_channel_a = 0;
unsigned long timer_channel_b = 0; 
unsigned long timer_channel_c = 0;
unsigned long timer_channel_d = 0; 

//                 abcd
byte motors = B00000000;

long va,vb,vc,vd ;  // motors a,b,c and d outputs


void init_esc()
{
//  DDRD |= B00001000;                                           //Configure digital poort 3 as output
//  DDRD |= B00100000;                                           //Configure digital poort 5 as output
    DDRD |= B01000000;                                           //Configure digital poort 6 as output
    DDRB |= B00001110;                                           //Configure digital poort 9, 10, 11 as output.

  system_check |= INIT_ESC_ATTACHED;
}

void arm_esc()
{
  system_check |= INIT_ESC_ARMED;
}

void disarm_esc()
{
  system_check &= ~(INIT_ESC_ARMED);
}

void update_motors()
{
  while( (micros() - last_pwm_pulse) < PWM_FERQUENCY );
  last_pwm_pulse= micros(); 

//PORTD |= B00001000; // Set digital port 3 high
  PORTD |= B01000000; // Set digital port 6 high
  PORTB |= B00001110; // Set digital port 9,10,11 high

  timer_channel_a = last_pwm_pulse + va;
  timer_channel_b = last_pwm_pulse + vb;
  timer_channel_c = last_pwm_pulse + vc;
  timer_channel_d = last_pwm_pulse + vd;

  motors = B00001111;
  while( motors ) 
  {
      esc_pwm_timmer = micros();
      if( (motors & B00001000) && ( esc_pwm_timmer > timer_channel_a )){ PORTD &= B10111111; motors &= B00000111; }
      if( (motors & B00000100) && ( esc_pwm_timmer > timer_channel_b )){ PORTB &= B11110111; motors &= B00001011; }
      if( (motors & B00000010) && ( esc_pwm_timmer > timer_channel_c )){ PORTB &= B11111011; motors &= B00001101; }
      if( (motors & B00000001) && ( esc_pwm_timmer > timer_channel_d )){ PORTB &= B11111101; motors &= B00001110; }      
  }
}
