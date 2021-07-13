#include <stdlib.h>
#include <iostream.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <mqueue.h>

int status;
mqd_t wave_q_id;   // message queue ID

extern "C"
{
  // ASM function that outputs voltage to D/A
  void hw_out(unsigned int port, unsigned char val);
  unsigned char hw_in(unsigned int port);
}



// this is called whenever there is a SIGINT (CTRL-C)
void sigint_handler()
{
  // close & unlink the shared message queue, then exit
  status = mq_close(wave_q_id);
  status = mq_unlink("WAVE_SPEC_Q");
  cout << "Ctrl-C Detected!\n";
  cout << "Queue unlinked.  Exiting.\n";
  exit(0);
}



int main(void)
{
  char prompt;
  unsigned char hiByteR, loByteR, hiByteL, loByteL;

  unsigned char buffer[2];
  struct mq_attr wave_q_attr;         // message queue attributes
  unsigned short digitalV;            // discrete 12-bit voltage
  unsigned short digitalL, digitalR;  // extreme L/R voltage bounds
  float angle, voltage;
  unsigned int prior = 5;             // arbitrary priority value

  // initialize the D/A, used in lab1
  hw_out( 0x309, 0 );   // disable int & DMA
  hw_out( 0x308, 0 );   // clear trigger flip flop for interrupt
  hw_out( 0x302, 0 );   // select analog input channel


  cout << "***********************************************************\n"
       << "***********************************************************\n"
       << "**                                                       **\n"
       << "**    CS 331 Lab 4 (Part 1) Display Process: Group 26    **\n"
       << "**                                                       **\n"
       << "***********************************************************\n"
       << "***********************************************************\n\n";

  cout << "Tilt the seesaw all the way to the right, and press ENTER ...\n";
  cin.get(prompt);

  hw_out(0x300,0x00);    // start A/D conversion
  // Cheap hack for getting stable A/D input
  // input loByte from BASE+0, hiByte from BASE+1
  while(hw_in(0x308) & 0x80 !=0) { };
  loByteR = hw_in(0x300);
  hiByteR = hw_in(0x301);

  cout << "Tilt the seesaw all the way to the left, and press ENTER ...\n";
  cin.get(prompt);

  hw_out(0x300,0x00);    // start A/D conversion
  // Cheap hack for getting stable A/D input
  // input loByte from BASE+0, hiByte from BASE+1
  while(hw_in(0x308) & 0x80 !=0) { };
  loByteL = hw_in(0x300);
  hiByteL = hw_in(0x301);

  // setup message queue attributes
  memset(&wave_q_attr,0,sizeof(wave_q_attr));
  wave_q_attr.mq_maxmsg = 10;
  wave_q_attr.mq_msgsize = sizeof(buffer);

  // setup buffer to store the waveform spec
  // buffer[0] = loByte, buffer[1] = hiByte
  memset(buffer,0,sizeof(buffer));
  wave_q_id = mq_open("WAVE_SPEC_Q",O_RDONLY | O_CREAT,0600,
    &wave_q_attr);

  struct sigaction my_action;              // sighandler initializations
  memset(&my_action,0,sizeof(my_action));  // clear all flags
  my_action.sa_handler = sigint_handler;
  sigaction(SIGINT,&my_action,NULL);

  digitalL = hiByteL & 0x00FF;
  digitalL = (digitalL << 4) & 0x0FF0;           // [0000HHHH HHHH0000]
  digitalL = ((loByteL >>4)& 0x0F) | digitalL;   // [0000HHHH HHHHLLLL]

  digitalR = hiByteR & 0x00FF;
  digitalR = (digitalR << 4) & 0x0FF0;           // [0000HHHH HHHH0000]
  digitalR = ((loByteR >>4)& 0x0F) | digitalR;   // [0000HHHH HHHHLLLL]

  cout << "Start the I/O process now...\n";

  while(1) {
    // pull out a chunk of data from the message queue, put into buffer
    status = mq_receive(wave_q_id, (char *)buffer, sizeof(buffer),&prior);

    digitalV = buffer[1] & 0x00FF;
    digitalV = (digitalV <<4) & 0x0FF0;            // [0000HHHH HHHH0000]
    digitalV = ((buffer[0] >>4)& 0x0F) | digitalV; // [0000HHHH HHHHLLLL]

    angle=(((float)(digitalV-digitalL)/(float)(digitalR-digitalL))*30.0)-15.0;
    voltage = (((float)digitalV/4095.0)*20.0)-10.0;
    cout << "raw=" << digitalV << "  angle=" << angle
         << "  voltage=" << voltage << "\n";
  }
}
