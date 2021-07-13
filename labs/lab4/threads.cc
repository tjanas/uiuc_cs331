#include <stdlib.h>
#include <iostream.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <mqueue.h>
#include <pthread.h>

pthread_mutex_t wave_spec_mutex;    // protects buffer
unsigned char buffer[2];            // loByte, hiByte
unsigned short digitalL, digitalR;  // digital left/right bounds of seesaw

sigset_t io_signals;           // signal set used by SIGWAIT for io
sigset_t display_signals;      // signal set used by SIGWAIT for display

extern "C"
{
  // ASM function that outputs voltage to D/A
  void hw_out(unsigned int port, unsigned char val);
  unsigned char hw_in(unsigned int port);
}



// function executed by display thread
void* display(void* args)
{
  int status;
  unsigned char loByte, hiByte;  // data from A/D card
  unsigned short digitalV;       // 12-bit discrete voltage
  float angle, voltage;          // seesaw angle & analog voltage

  while(1)
  {
    sigwait(&display_signals,&status);  // wait until SIGUSR2 is received

    status = pthread_mutex_lock(&wave_spec_mutex);
    /***  enter critical section, read global variable  ***/
    loByte = buffer[0];
    hiByte = buffer[1];
    /***  exit critical section  ***/
    status = pthread_mutex_unlock(&wave_spec_mutex);

    // convert loByte and hiByte into one 12-bit value
    digitalV = hiByte & 0x00FF;
    digitalV = (digitalV << 4) & (0x0FF0);
    digitalV = ((loByte >>4)&(0x000F)) | digitalV;

    // from our discrete value, we can calculate the angle and analog voltage
    angle=(((float)(digitalV-digitalL)/(float)(digitalR-digitalL))*30.0)-15.0;
    voltage = (((float)digitalV/4095.0)*20.0)-10.0;
    cout << "raw=" << digitalV << "  voltage=" << voltage
         << "  angle=" << angle << "\n";
  }
}



// function executed by main thread after SIGUSR1 interrupt occurs
void io_timer_handler()
{
  int status;
  unsigned char loByte, hiByte;

  hw_out(0x300,0x00);   // start A/D conversion
  // Cheap hack for getting stable A/D input
  // input loByte from BASE+0, hiByte from BASE+1
  while(hw_in(0x308) & 0x80 != 0) { };
  loByte = hw_in(0x300);
  hiByte = hw_in(0x301);

  status = pthread_mutex_lock(&wave_spec_mutex);
  // enter critical section, read global variable
  buffer[0] = loByte;
  buffer[1] = hiByte;
  // exit critical section
  status = pthread_mutex_unlock(&wave_spec_mutex);
}



int main(void)
{
  int status;
  char prompt;
  unsigned char hiByteR, loByteR, hiByteL, loByteL;

  pthread_mutex_init(&wave_spec_mutex, NULL);  // initialize mutex
  pthread_t wave_thread_id;                    // thread ID

  // initialize the A/D, D/A controller
  hw_out( 0x309, 0 );   // disable int & DMA
  hw_out( 0x308, 0 );   // clear trigger flip flop for interrupt
  hw_out( 0x302, 0 );   // select analog input channel

  cout << "**************************************************\n"
       << "**************************************************\n"
       << "**                                              **\n"
       << "**    CS 331 Lab 4 (Part 2) Thread: Group 26    **\n"
       << "**                                              **\n"
       << "**************************************************\n"
       << "**************************************************\n\n";

  cout << "Tilt the seesaw all the way to the right, and press ENTER...\n";
  cin.get(prompt);

  hw_out(0x300,0x00);                    // start A/D conversion
  // Cheap hack for getting stable A/D input
  // input loByte from BASE+0, hiByte from BASE+1
  while(hw_in(0x308) & 0x80 !=0) { };
  loByteR = hw_in(0x300);                // get calibration data,
  hiByteR = hw_in(0x301);                // right-side of seesaw

  cout << "Tilt the seesaw all the way to the left, and press ENTER ...\n";
  cin.get(prompt);

  hw_out(0x300,0x00);                    // start A/D conversion
  // Cheap hack for getting stable A/D input
  // input loByte from BASE+0, hiByte from BASE+1
  while(hw_in(0x308) & 0x80 !=0) { };
  loByteL = hw_in(0x300);                // get calibration data,
  hiByteL = hw_in(0x301);                // right-side of seesaw

  // convert "left" calibration voltage data into 12 bit discrete value
  digitalL = hiByteL & 0x00FF;
  digitalL = (digitalL << 4) & (0x0FF0);
  digitalL = ((loByteL >>4)&(0x000F)) | digitalL;

  // convert "right" calibration voltage data into 12 bit discrete value
  digitalR = hiByteR & 0x00FF;
  digitalR = (digitalR << 4) & (0x0FF0);
  digitalR = ((loByteR >>4)&(0x000F)) | digitalR;


  struct itimerspec io_timer_spec, display_timer_spec,
                    io_old_timer_spec, display_old_timer_spec;

  // create io timer
  timer_t io_timer;
  struct sigevent io_timer_evp;
  memset(&io_timer_evp, 0, sizeof(io_timer_evp));
  io_timer_evp.sigev_signo = SIGUSR1;        // io responds to SIGUSR1 signal
  io_timer_evp.sigev_notify = SIGEV_SIGNAL;
  timer_create(CLOCK_REALTIME, &io_timer_evp, &io_timer);

  // set io timer settings (goes off 10 times per second)
  io_timer_spec.it_value.tv_sec = 0;   // first SIGUSR1 will go off only 1 ns
  io_timer_spec.it_value.tv_nsec = 1;   // after the timer is started
  io_timer_spec.it_interval.tv_sec = 0;   // our timer interval is
  io_timer_spec.it_interval.tv_nsec = 200000000;

  // create display timer
  timer_t display_timer;
  struct sigevent display_timer_evp;
  memset(&display_timer_evp, 0, sizeof(display_timer_evp));
  display_timer_evp.sigev_signo = SIGUSR2;   // display responds to SIGUSR2
  display_timer_evp.sigev_notify = SIGEV_SIGNAL;
  timer_create(CLOCK_REALTIME, &display_timer_evp, &display_timer);

  // set display timer settings (goes off 10 times per second)
  display_timer_spec.it_value.tv_sec = 0;   // 1st SIGUSR2 will go off only
  display_timer_spec.it_value.tv_nsec = 1;  // 1 ns after the timer is started
  display_timer_spec.it_interval.tv_sec = 0;   // our timer interval is
  display_timer_spec.it_interval.tv_nsec = 200000000;

  sigemptyset(&io_signals);              // Create signal set for SIGWAIT,
  sigaddset(&io_signals, SIGUSR1);       //  used by io.
  sigemptyset(&display_signals);         // Create signal set for SIGWAIT,
  sigaddset(&display_signals, SIGUSR2);  //  used by display thread.

  // start io timer
  timer_settime(io_timer,0,&io_timer_spec,&io_old_timer_spec);

  // start display timer
  timer_settime(display_timer,0,&display_timer_spec,&display_old_timer_spec);

  io_timer_handler();  // store initial data in the buffer

  // spawn the display thread...
  status = pthread_create(&wave_thread_id, NULL, display, NULL);

  while(1) {                        // pass raw data to buffer
    sigwait(&io_signals,&status);   // 10 times per second, until
    io_timer_handler();             // user presses CTRL-C
  }
}
