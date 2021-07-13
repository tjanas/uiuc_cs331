void hw_out(unsigned int port, unsigned char val)
{
   __asm__ __volatile__ ("outb %b0, %w1"
                         : /* no output */
                         : "a" (val), "d" (port) );
}

unsigned char hw_in(unsigned int port)
{
   unsigned char val;
   __asm__ __volatile__ ("inb %w1, %b0"
                         : "=a" (val)
                         : "d" (port) );
   return val;
}
