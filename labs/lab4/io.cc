#include <stdlib.h>
#include <iostream.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <mqueue.h>

int status;                     // status of queue operations
unsigned char buffer[2];        // a unit of data in the message queue
mqd_t wave_q_id;                // ID of the message queue
struct mq_attr wave_q_attr;     // message queue attributes

extern "C"
{
  // ASM function that outputs voltage to D/A
  void hw_out(unsigned int port, unsigned char val);
  unsigned char hw_in(unsigned int port);
}



void timer_handler()  // this is called whenever there is a SIGALRM
{
  hw_out(0x300,0x00);     // start A/D conversion

  // Cheap hack for getting stable A/D input
  // input loByte from BASE+0, hiByte from BASE+1  
  while(hw_in(0x308) & 0x80 !=0) { };
  buffer[0] = hw_in(0x300);
  buffer[1] = hw_in(0x301);
}



void sigint_handler()  // this is called whenever there is a SIGINT (CTRL-C)
{
  status = mq_close(wave_q_id);       // close & unlink the
  status = mq_unlink("WAVE_SPEC_Q");  // shared message queue,
  exit(0);                            // then exit
}



int main(void)
{
   // initialize the D/A, used in lab1
   hw_out( 0x309, 0 );   // disable int & DMA
   hw_out( 0x308, 0 );   // clear trigger flip flop for interrupt
   hw_out( 0x302, 0 );   // select analog input channel

   cout << "*************************************************************\n" 
        << "*************************************************************\n"  
        << "**                                                         **\n"
        << "**   CS 331 Lab 4 (Part 1) I/O Process Example: Group 26   **\n"
        << "**                                                         **\n"
        << "*************************************************************\n"
        << "*************************************************************\n\n";

   cout << "The display process should already be running and calibrated.\n\n";

   /* make queue */
   memset(buffer,0,sizeof(buffer));        // clean up buffer space
   memset(&wave_q_attr, 0, sizeof(wave_q_attr));
   wave_q_attr.mq_flags = O_NONBLOCK;      // set mqueue parameters
   wave_q_attr.mq_maxmsg = 10;
   wave_q_attr.mq_msgsize = sizeof(buffer);

   // open the message queue
   wave_q_id = mq_open("WAVE_SPEC_Q",O_WRONLY | O_CREAT | O_NONBLOCK,0600,
      &wave_q_attr);

   struct sigaction my_action;   // timer_handler() is called for SIGALRM's

   // used by the timer created, and initialized for the timer handler
   timer_t timer_id;
   struct itimerspec timer_spec, old_timer_spec;
   timer_create(CLOCK_REALTIME, NULL, &timer_id);

   // set timer parameters
   timer_spec.it_value.tv_sec = 0;   // first SIGALRM will go off only 1 nsec
   timer_spec.it_value.tv_nsec = 1;  // after the timer is started 
   timer_spec.it_interval.tv_sec = 0;   // our timer interval is
   timer_spec.it_interval.tv_nsec = 200000000;

   // sighandler initializations
   memset(&my_action,0,sizeof(my_action));  // clear all flags
   my_action.sa_handler = timer_handler;
   sigaction(SIGALRM,&my_action,NULL);
   my_action.sa_handler = sigint_handler;
   sigaction(SIGINT,&my_action,NULL);

   // start the timer!
   timer_settime(timer_id,0,&timer_spec,&old_timer_spec);

   while(1) {
      sigpause(SIGALRM);  // when there is a SIGALRM, run timer_handler()
      status = mq_send(wave_q_id,(char *)buffer,sizeof(buffer),5);
      if (status == -1)   // queue overflow?
      {
         status = mq_close(wave_q_id);
         status = mq_unlink("WAVE_SPEC_Q");
         return 1;
      }
   }
}
