#include <stdlib.h>
#include <iostream.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <string.h>


short control = 1;  // global variable used by timer_handler()

// variables from lab1:
// stores the values that are sent to the D/A at both
// transitions of the square wave
unsigned char loByte1, hiByte1;
unsigned char loByte2, hiByte2;



extern "C"
{
   // ASM function that outputs voltage to D/A
   void hw_out(unsigned int port, unsigned char val);
}



// this is called whenever there is a SIGALRM
void timer_handler()
{
   if (control == 1) {
      hw_out( 0x304,loByte1 );  // output the 1st user-specified voltage
      hw_out( 0x305,hiByte1 );  // if control is +1
   }

   else {
      hw_out( 0x304,loByte2 );  // output the 2nd user-specified voltage
      hw_out( 0x305,hiByte2 );  // if control is -1
   }

   // output the other voltage the next time there is a SIGALRM
   control=control*(-1);
}



void main()
{
   float voltage1, voltage2;           // user-specified voltages
   unsigned short digitalV1,digitalV2; // voltage values after normalization
   float user_frequency;               // user-specified frequency

   // used by clock_getres() to get the resolution of the clock
   struct timespec clock_resolution;

   float res_period;     // the minimum clk period supported by the machine
   float res_frequency;  // the maximum clk freq supported by the machine


   // sec and nsec components of the square wave period / 2
   // our timer will go off at both transitions of the square wave, so
   // we can change the output voltage
   int user_period_sec;
   unsigned long user_period_nsec;


   struct sigaction my_action;   // timer_handler() is called for SIGALRM's


   // used by the timer created, and initialized for the timer handler
   timer_t timer_id;
   struct itimerspec timer_spec, old_timer_spec;
   timer_create(CLOCK_REALTIME, NULL, &timer_id);

   // initialize the D/A, used in lab1
   hw_out( 0x309, 0 ); // disable int & DMA
   hw_out( 0x308, 0 ); // clear trigger flip flop for interrupt
   hw_out( 0x302, 0 ); // select analog input channel


   // print out our fancy welcome screen!
   cout << "********************************\n"
        << "********************************\n"
        << "**                            **\n"
        << "**   CS 331 Lab 2: Group 26   **\n"
        << "**                            **\n"
        << "********************************\n"
        << "********************************\n\n";

   cout << "Enter Voltage #1 (-5V to +5V, other to Quit): ";
   // make sure user enters some kind of number for voltage1
   while( !(cin >> voltage1) )
      return -1;

   // make sure voltage1 is within the acceptable bounds
   if ( (voltage1<-5.0)||(voltage1>5.0) )
      return -1;


   cout << "Enter Voltage #2 (-5V to +5V, other to Quit): ";
   // make sure user enters some kind of number for voltage2
   while( !(cin >> voltage2) )
      return -1;

   // make sure voltage2 is within the acceptable bounds
   if ( (voltage2<-5.0)||(voltage2>5.0) )
      return -1;


   // find out what our shortest period / highest freq may be
   clock_getres(CLOCK_REALTIME,&clock_resolution);
   res_period = (float) clock_resolution.tv_nsec;
   // we divide freq by 2, since we are going to change the voltage
   // at the the rising clock edge and the falling clock edge
   res_frequency = 1000000000 / (2*res_period);


   printf("Enter frequency (0Hz to %dHz, other to quit): ",
      (int)res_frequency);
   // make sure user enters some kind of number for user_frequency
   while( !(cin >> user_frequency) )
      return -1;


   // and we must check to see if the user's freq can be supported
   if ( (user_frequency<0)||(user_frequency>res_frequency) )
      return -1;


   // some math to convert the frequency to sec/nsec components of the period
   user_period_sec = 1 / (2.0*user_frequency);
   user_period_nsec = 1000000000 / (2 * user_frequency); 
   user_period_nsec = user_period_nsec - (user_period_sec * 1000000000);


   // printf("%d %d\n", user_period_sec, user_period_nsec);

   timer_spec.it_value.tv_sec = 0;   // first SIGALRM will go off only 1 nsec
   timer_spec.it_value.tv_nsec = 1;  // after the timer is started

   timer_spec.it_interval.tv_sec = user_period_sec;   // our timer interval is
   timer_spec.it_interval.tv_nsec = user_period_nsec; // whatever user wanted


   // sighandler initializations
   memset(&my_action,0,sizeof(my_action));  // clear all flags
   my_action.sa_handler = timer_handler;
   sigaction(SIGALRM,&my_action,NULL);

   cout << "Generating square wave (Ctrl+C to quit)...\n";


   // Convert -5/+5 voltage range into 0/4095 range for first voltage
   digitalV1 = (unsigned short)( ((voltage1 + 5.0)*4095.0) / 10.0 );

   // Convert -5/+5 voltage range into 0/4095 range for second voltage
   digitalV2 = (unsigned short)( ((voltage2 + 5.0)*4095.0) / 10.0 );

   // fun stuff from lab1:
   // loByte = bits:[03 02 01 00 xx xx xx xx]
   // hiByte = bits:[11 10 09 08 07 06 05 04]
   loByte1 = (digitalV1 <<4) & (0xF0);
   hiByte1 = (digitalV1 >>4) & (0xFF);
   loByte2 = (digitalV2 <<4) & (0xF0);
   hiByte2 = (digitalV2 >>4) & (0xFF);

   // start the timer!
   timer_settime(timer_id,0,&timer_spec,&old_timer_spec);

   while(1)
   {
      sigpause(SIGALRM);  // when there is a SIGALRM, run timer_handler()
   }

   return 0;
}
