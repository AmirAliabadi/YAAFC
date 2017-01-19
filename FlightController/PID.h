/* 
 *  YMFC PID values
 *  yaw: 4.0, 0.02, 0.0
 *  pitch/roll : 1.04, 0.05, 15
 */

float y_pid_gains[3]  = {5.00, 0.00,  0.0};
float p_pid_gains[3]  = {1.04, 0.05,  15.0};
float r_pid_gains[3]  = {1.04, 0.05,  15.0};

float y_pid_term[3]   = {0,0,0};
float p_pid_term[3]   = {0,0,0};
float r_pid_term[3]   = {0,0,0};

float y_pid_rate_out   = 0.0;
float p_pid_rate_out   = 0.0;
float r_pid_rate_out   = 0.0;

float y_last_error    = 0.0;
float p_last_error    = 0.0;
float r_last_error    = 0.0;

void init_pid()
{
  system_check |= INIT_PID_ON ;
}

void pid_reset() 
{
  y_pid_rate_out   = 0.0;
  p_pid_rate_out   = 0.0;
  r_pid_rate_out   = 0.0;

  y_last_error    = 0.0;
  p_last_error    = 0.0;
  r_last_error    = 0.0;

  for( byte b =0; b<3; b++ ) {
    y_pid_term[b]   = 0;
    p_pid_term[b]   = 0;
    r_pid_term[b]   = 0;  
  }
  
}

float pid_error;
void do_pid_compute()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // COMPUTE YAW PID
    pid_error = yaw_input - gyro[2] ;
    y_pid_term[0] = y_pid_gains[0] * pid_error;                     // pTerm;
    y_pid_term[2] = y_pid_gains[2] * ( pid_error - y_last_error );  // dTerm = dGain * (current error - last_error)
    y_last_error = pid_error;                                       // update yaw last_error for next time

    y_pid_rate_out  = y_pid_term[0] + y_pid_term[1] + y_pid_term[2] ;

    if( y_pid_rate_out >  400.0 ) y_pid_rate_out = 400.0;
    if( y_pid_rate_out < -400.0 ) y_pid_rate_out = -400.0;  


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // COMPUTE PITCH PID
    pid_error = pitch_input - gyro[0] ; 
    p_pid_term[0] = p_pid_gains[0] * pid_error;                     // pTerm;    
    p_pid_term[2] = p_pid_gains[2] * ( pid_error - p_last_error );  // dTerm = dGain * (current error - last_error)
    p_last_error = pid_error;                                       // update pitch last_error for next time
    
    p_pid_term[1] += p_pid_gains[1] * pid_error;                    // integrate the iTerm
    if( p_pid_term[1] >  400.0 ) p_pid_term[1] =  400.0;
    if( p_pid_term[1] < -400.0 ) p_pid_term[1] = -400.0;    
    
    p_pid_rate_out  = p_pid_term[0] + p_pid_term[1] + p_pid_term[2] ;

    if( p_pid_rate_out >  400.0 ) p_pid_rate_out =  400.0;
    if( p_pid_rate_out < -400.0 ) p_pid_rate_out = -400.0;    
    
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // COMPUTE ROLL PID
    pid_error = roll_input - gyro[1] ;     
    r_pid_term[0] = r_pid_gains[0] * pid_error;                     // pTerm;    
    r_pid_term[2] = r_pid_gains[2] * ( pid_error - r_last_error );  // dTerm = dGain * (current error - last_error)    
    r_last_error = pid_error;                                       // update roll last_error for next time   

    r_pid_term[1] += r_pid_gains[1] * pid_error;                    //  integrate the iTerm
    if( r_pid_term[1] >  400.0 ) r_pid_term[1] =  400.0;
    if( r_pid_term[1] < -400.0 ) r_pid_term[1] = -400.0;    
    
    r_pid_rate_out  = r_pid_term[0] + r_pid_term[1] + r_pid_term[2] ; 

    if( r_pid_rate_out >  400.0 ) r_pid_rate_out =  400.0;
    if( r_pid_rate_out < -400.0 ) r_pid_rate_out = -400.0;  

}
