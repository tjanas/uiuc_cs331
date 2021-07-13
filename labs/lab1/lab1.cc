#include <iostream.h>

extern "C"
{
   void hw_out(unsigned int port, unsigned char val);
   unsigned char hw_in(unsigned int port);
}

int main(void)
{
   float voltage;
   unsigned short digitalV;
   unsigned char loByte, hiByte;
   unsigned short loop = 0;

   cout << "********************************\n"
        << "********************************\n"
        << "**                            **\n"
        << "**   CS 331 Lab 1: Group 26   **\n"
        << "**                            **\n"
        << "********************************\n"
        << "********************************\n\n";

   cout << "Enter Voltage (-5V to +5V, other to Quit): ";

   while(1) // Let the user enter values until he chooses to quit
   {
      // Make sure user input is a number
      while( !(cin >> voltage) )
         return 0;

      // Voltage should be between -5V and +5V
      if ( (voltage<-5.0)||(voltage>5.0) )
         return 0;

      // Convert -5/+5 voltage range into 0/4095 range
      digitalV = (unsigned short)( ((voltage + 5.0)*4095.0) / 10.0 );

      // loByte = bits:[03 02 01 00 xx xx xx xx]
      // hiByte = bits:[11 10 09 08 07 06 05 04]
      loByte = (digitalV <<4) & (0xF0);
      hiByte = (digitalV >>4) & (0xFF);

      // Use D/A to output user-specified voltage by
      // sending loByte to BASE+4, hiByte to BASE+5
      hw_out( 0x304,loByte );
      hw_out( 0x305,hiByte );

      // Cheap hack for waiting for output to stabilize
      while (loop < 65535)
        loop++;

      // Start A/D Conversion
      hw_out( 0x300, 0x00 );

      // Cheap hack for getting stable A/D input
      // input loByte from BASE+0, hiByte from BASE+1
      hiByte = hw_in(0x301);
      loByte = hw_in(0x300);
      hiByte = hw_in(0x301);
      loByte = hw_in(0x300);

      // store hiByte in lobyte of digitalV, zero out digitalV hibyte
      digitalV = hiByte & 0x00FF;

      // digitalV = [0000HHHH HHHH0000]
      digitalV = (digitalV << 4) & (0x0FF0);

      // digitalV = [0000HHHH HHHHLLLL]
      digitalV = ((loByte >>4)&(0x000F)) | digitalV;

      // Translate 12-bit value to a voltage between -10V,+10V
      voltage = ( (float)digitalV * 20.0 / 4095.0 ) - 10.0;

      cout << "End Voltage: " << voltage << "\n";
      cout << "Enter Voltage: ";
   }
}
