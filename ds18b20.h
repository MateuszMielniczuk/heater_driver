#ifndef DS18B20_H
#define DS18B20_H

/* DS18B20 przylaczony do portu  PC5 AVRa  */
int WE;
#define SET_ONEWIRE_PORT     PORTD  |=  _BV(WE)
#define CLR_ONEWIRE_PORT     PORTD  &= ~_BV(WE)
#define IS_SET_ONEWIRE_PIN   PIND   &   _BV(WE)
#define SET_OUT_ONEWIRE_DDR  DDRD   |=  _BV(WE)
#define SET_IN_ONEWIRE_DDR   DDRD   &= ~_BV(WE)

unsigned char ds18b20_ConvertT(void);		//konwersja danych przez czujnik temperatury zapis w MSB i LSB
int ds18b20_Read(unsigned char []);			//odczyt z czujnika
void OneWireStrong(char);
unsigned char OneWireReset(void);			// reset czujnika
void OneWireWriteByte(unsigned char);		
unsigned char OneWireReadByte(void);		

#endif
