#include <avr/io.h>
#include <util/delay.h>
#include "ds18b20.h"


/**********RESET CZUJNIKA******************/

unsigned char OneWireReset()
{
	CLR_ONEWIRE_PORT;

	if (!(IS_SET_ONEWIRE_PIN)) return 0;

	SET_OUT_ONEWIRE_DDR;
	_delay_us(500);
	SET_IN_ONEWIRE_DDR;
	_delay_us(70);

	if(!(IS_SET_ONEWIRE_PIN))
	{
		_delay_us(500);
		return(1);
	}

	_delay_us(500);

	return(0);
}


/*************KONWERSJA TEMP> W CZUJNIKU*************/

unsigned char ds18b20_ConvertT(void)
{
  if (!OneWireReset()) return 0;

  OneWireWriteByte(0xcc); // SKIP ROM
  OneWireWriteByte(0x44); // CONVERT T

  return -1;
}

/************ODCZYT WARTOSCI TEMPERATURY Z CZUJNIKA Z CZUJNIKA***************/

int ds18b20_Read(unsigned char scratchpad[])
{
  unsigned char i;    

  if (!OneWireReset()) return 0;

  OneWireWriteByte(0xcc); // SKIP ROM
  OneWireWriteByte(0xbe); // READ SCRATCHPAD

  for(i=0; i<9; i++) scratchpad[i] = OneWireReadByte();
 
  return 1;
}

/**********************************************************/

void OneWireStrong(char s)
{
  if (s)
  {
     SET_ONEWIRE_PORT; 
     SET_OUT_ONEWIRE_DDR; 
  }
  else
  {
     SET_IN_ONEWIRE_DDR; 
  }
}



/**********************************************************/

void OneWireWriteByte(unsigned char byte)
{
   unsigned char i;

   CLR_ONEWIRE_PORT; 

   for (i=0; i<8; i++)
   {
     SET_OUT_ONEWIRE_DDR; 

     if (byte & 0x01)
     {
       _delay_us(7);
       SET_IN_ONEWIRE_DDR; 
       _delay_us(70);
     }
     else
     {
        _delay_us(70);
        SET_IN_ONEWIRE_DDR; 
        _delay_us(7);
     }

     byte >>= 1;
   }
}

/***********************************************************/

unsigned char OneWireReadByte(void)
{
  unsigned char i, byte = 0;

  SET_IN_ONEWIRE_DDR; 
  
  for (i=0; i<8; i++)
  {
     SET_OUT_ONEWIRE_DDR; 
     _delay_us(7);
     SET_IN_ONEWIRE_DDR; 
     _delay_us(7);
     byte >>= 1;
     
     if(IS_SET_ONEWIRE_PIN) byte |= 0x80;

     _delay_us(70);
  }

  return byte;
}
