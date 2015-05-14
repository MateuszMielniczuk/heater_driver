#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include "hd44780.h"
#include "ds18b20.h"
#include "keyb.h"

//wyj�cie na za��czanie czego� przy zadanej temperaturze
#define ADC_DDR			DDRC
#define ADCKOLEKTOR	 	PC5
#define OUT_PORT		PORTB
#define OUT_DDR			DDRB
#define grzalka			PB3
#define out1 			PB2
#define out2 			PB4
#define LED_LCD			PB1
#define buzzer			PB0
#define buzzer_ON		OUT_PORT &=~ (1 << buzzer)
#define buzzer_OFF		OUT_PORT |= (1 << buzzer)
#define grzalka_ON 		OCR2 //OUT_PORT &=~ (1 << grzalka);
#define grzalka_OFF 	OCR2=0//OUT_PORT |= (1 << grzalka);
#define out1_ON			OCR1B=255	//BOUT_PORT &=~ (1 << out1);
#define out1_OFF		OCR1B=0	//OUT_PORT |= (1 << out1);
#define out2_ON			OUT_PORT &=~ (1 << out2)
#define out2_OFF		OUT_PORT |= (1 << out2)
#define LED_LCD_ON		lcd[a]
#define LED_LCD_OFF		OCR1A=255

uint8_t check_eeprom;
volatile uint8_t grzalka_min=30;
volatile uint8_t grzalka_max=32;
volatile uint8_t out1_min=26;
volatile uint8_t out1_max=28;
volatile uint8_t out2_min=29;
volatile uint8_t out2_max=29;
volatile uint8_t alarm=100;
volatile uint8_t allow=0;
volatile uint8_t dzielnik=30;
volatile unsigned int refresh=1;
volatile char lcd[]={255, 253, 250, 245, 235, 220, 200, 165, 115, 65, 0};
volatile uint8_t a;
volatile unsigned int MENU=0;
static char _key;
static unsigned int _key_time;
int checkvalue;
volatile int counter_LCD;
volatile uint8_t czas_LCD;


void adc_init()
{
	 ADMUX = (1<<REFS1) | (1<<REFS0) 	// AREF = 2,56V and external capacitor on AREF

	 |(1<<MUX2) | (1<<MUX0);// wybór kana?u ADC5 na pinie PC5

	   ADCSRA = (1<<ADEN) //turn on ADC
	   |(1<<ADFR) // turn on Free run mode
	   |(1<<ADIE) //uruchomienie zg?aszania przerwa?
	   |(1<<ADSC) //rozpocz?cie konwersji
	   |(1<<ADPS0)   //ADPS2:0: ustawienie preskalera na 128
	   |(1<<ADPS1)
	   |(1<<ADPS2);

	   ADC_DDR &=~ (1<<ADCKOLEKTOR);            //Ustawienie Wej?cia ADC


}

void init_io(void)
	{
	//===========================PWM=============================

		TCCR1A |= (1<<COM1A1) |(1<<COM1B1) |(1<<COM1A0) |(1<<COM1B0) | (1<<WGM10); //PWM, Phase Correct, 8-bit initalize
		TCCR1B |= (1<<CS11);					//fclk/(2*N*TOP)=8M/(2*8*255)~=2kHz  1/8 preskaler
							//| (1<<CS10) ;			fclk=250Hz			// 1/64 preskaler


		TCCR2 |= (1<<WGM20) | (1<<COM20) | (1<<COM21) | (1<<CS21) | (1<<CS22);// | (1<<CS20) ;
		//===================TIMER1 FOR LED LCD+++++++++++

		//Setup Buttons
		KEY_DDR &= ~(KEY0 | KEY1 | KEY2 );  //Set pins as input
		KEY_PORT |= KEY0 | KEY1 | KEY2 ;    //enable pull-up resistors

		// connect LED
		OUT_DDR |= (1 << grzalka);
		OUT_DDR |= (1 << out1);
		OUT_DDR |= (1 << out2);
		OUT_DDR	|= (1 << LED_LCD);
		OUT_DDR	|= (1 << buzzer);

		// Initial value LED off
		grzalka_OFF;
		out2_OFF;
		out1_OFF;
		LED_LCD_OFF;
		buzzer_OFF;



		//===================TIMER0 FOR BUTTONS+++++++++++

		cli();            // read and clear atomic !
		//Timer0 for buttons
		TCCR0 |= 1<<CS02 | 1<<CS00;  //Divide by 1024
		TIMSK |= 1<<TOIE0;     //enable timer overflow interrupt
		sei();            // enable interrupts


	}



// port do kt�rego jesp przy��czony czujnik(5-3)
int WE=0;



//zmienne do przeliczania warto�ci z czujnika na wy�wietlacz
typedef struct
{
	unsigned char Calkowita;
	unsigned char Ulamek;
	unsigned char Znak;
}TTemperatura;

unsigned char i=0;
unsigned char tempLSB=0, tempMSB=0;
/* W tablicy b?d? formowane komunikaty tekstowe
   wysy?ane do wy?wietlacza */
char str[17];


// do przeliczania warto�ci z czujnika na wy�wietlacz
char * ltoaz( unsigned long val, char *buf, int radix, int dig )
{
	char tmp[ 32 ];
	int i = 0;
	
	if(( radix >= 2 ) && ( radix <= 16 ))
	{
		do
		{
			tmp[ i ] = ( val % radix ) + '0';
			if(tmp[ i ] > '9' ) tmp[ i ] += 7;
			val /= radix;
			i++;
		}while( val );
		
		while(( i < dig ) && ( i < 32 ))
		{
			tmp[ i++ ] = '0';
		}
		
		while( i )
		{
			*buf++ = tmp[ i-1 ];
			i--;
		}
	}
	*buf = 0;
	return buf;
}



void display_caption(void)
{
	if (refresh==1)
	{
	switch(MENU)
	{
		case 2:
		{

			LCD_LOCATE(0,0);
			lcd_puts("   Grzalka MAX  ");
			numbertostr(grzalka_max);
			LCD_LOCATE(0,1);
			lcd_puts("                ");
			LCD_LOCATE(6,1);
			lcd_puts(str);

		}break;

		case 1:
		{
			_delay_ms(1000);
			LCD_LOCATE(0,0);
			lcd_puts("   Grzalka MIN  ");
			numbertostr(grzalka_min);
			LCD_LOCATE(0,1);
			lcd_puts("                ");
			LCD_LOCATE(6,1);
			lcd_puts(str);

		}break;

		case 4:
		{
			LCD_LOCATE(0,0);
			lcd_puts("   OUT_1 MAX    ");
			numbertostr(out1_max);
			LCD_LOCATE(0,1);
			lcd_puts("                ");
			LCD_LOCATE(6,1);
			lcd_puts(str);

		}break;

		case 3:
		{
			LCD_LOCATE(0,0);
			lcd_puts("   OUT_1 MIN    ");
			numbertostr(out1_min);
			LCD_LOCATE(0,1);
			lcd_puts("                ");
			LCD_LOCATE(6,1);
			lcd_puts(str);

		}break;

		case 6:
		{
			LCD_LOCATE(0,0);
			lcd_puts("   OUT_2 MAX    ");
			numbertostr(out2_max);
			LCD_LOCATE(0,1);
			lcd_puts("                ");
			LCD_LOCATE(6,1);
			lcd_puts(str);

		}break;

		case 5:
		{
			LCD_LOCATE(0,0);
			lcd_puts("   OUT_2 MIN    ");
			numbertostr(out2_min);
			LCD_LOCATE(0,1);
			lcd_puts("                ");
			LCD_LOCATE(6,1);
			lcd_puts(str);

		}break;

		case 7:
		{
			LCD_LOCATE(0,0);
			lcd_puts("Jasnosc LED LCD");
			numbertostr(a*10);
			LCD_LOCATE(0,1);
			lcd_puts("                ");
			LCD_LOCATE(6,1);
			lcd_puts(str);

		}break;

		case 8:
		{
			LCD_LOCATE(0,0);
			lcd_puts("   Wlacz alarm  ");
			numbertostr(alarm);
			LCD_LOCATE(0,1);
			lcd_puts("                ");
			LCD_LOCATE(6,1);
			lcd_puts(str);

		}break;

		case 9:
		{
			LCD_LOCATE(0,0);
			lcd_puts("Czas podswietl.");
			numbertostr(czas_LCD);
			LCD_LOCATE(0,1);
			lcd_puts("                ");
			LCD_LOCATE(6,1);
			lcd_puts(str);

		}break;

		case 10:
		{
			LCD_LOCATE(0,0);
			lcd_puts("    Dzielnik   ");
			numbertostr(dzielnik);
			LCD_LOCATE(0,1);
			lcd_puts("                ");
			LCD_LOCATE(6,1);
			lcd_puts(str);

		}break;


		case 11:
		{
			LCD_CLEAR;
			LCD_LOCATE(0,0);
			lcd_puts("   Saving.      ");
			_delay_ms(100);
			LCD_LOCATE(0,0);
			lcd_puts("   Saving..     ");
			_delay_ms(100);
			LCD_LOCATE(0,0);
			lcd_puts("   Saving...    ");
			_delay_ms(100);
			LCD_LOCATE(0,0);
			lcd_puts("   Saving....   ");
			_delay_ms(100);
			MENU=0;
		}break;



	}



	if (MENU>=20)
	{
		grzalka_OFF;
		buzzer_OFF;
		out2_OFF;
		out1_OFF;
		_delay_ms(2000);
		LCD_LOCATE(0,1);
		lcd_puts("       OFF      ");

		while (MENU>=20)
		{
			_delay_ms(100);

		switch(MENU)
			{

		case 20:
		{

			LCD_LOCATE(0,0);
			lcd_puts("     BUZZER     ");

		}break;

		case 21:
		{
			LCD_LOCATE(0,0);
			lcd_puts("      OUT_2     ");

		}break;


		case 22:
		{
			LCD_LOCATE(0,0);
			lcd_puts("      OUT_1     ");

		}break;

		case 23:
		{
			LCD_LOCATE(0,0);
			lcd_puts("     GRZALKA    ");

		}break;



		case 24:
		{
			LCD_CLEAR;
			LCD_LOCATE(0,0);
			lcd_puts("   Saving.      ");
			_delay_ms(100);
			LCD_LOCATE(0,0);
			lcd_puts("   Saving..     ");
			_delay_ms(100);
			LCD_LOCATE(0,0);
			lcd_puts("   Saving...    ");
			_delay_ms(100);
			LCD_LOCATE(0,0);
			lcd_puts("   Saving....   ");
			_delay_ms(100);
			MENU=0;
		}break;





			}
		}
	}



	//if (MENU!=0)
	OCR1A=LED_LCD_ON;
	}
	refresh=0;
}


//przekszta�ca warto�� otrzyman� z czujnika na liczb�
char * DSTempToStr(char *buf, unsigned char DStempLSB, unsigned char DStempMSB)
{
	char * ret = buf;
	TTemperatura temp;
	
	unsigned short DStemp = (DStempMSB << 8 ) | DStempLSB;
	
	temp.Znak = DStempMSB >> 7;
	if( temp.Znak )
	{
		DStemp = ~DStemp + 1;
	}
	
	temp.Calkowita = ( unsigned char )(( DStemp >> 4 ) & 0x7F );
	temp.Ulamek = ( unsigned char )((( DStemp & 0xF ) * 625) / 1000 );
	
	if( temp.Znak )
	{
		*buf++ = '-';
	}	else {*buf++ = '+';}
	
	buf = ltoaz( temp.Calkowita, buf, 10, 2 );
	
	*buf++ = '.';
	buf = ltoaz( temp.Ulamek, buf, 10, 1);
	*buf++ ='%4.1f\xdf';
	*buf++ = 'C';
	*buf = '\0';
	return   ret;
	
}


void numbertostr(int numer)
{
	int zmienna=numer;
	for (i=0; i<=14; i++)
	{
		str[i]=32;

	}

	i=0;

	if (numer>=100)
	{
		numer=numer/100;
		str[i]=numer+48;
		i++;
		numer=zmienna-numer*100;
		zmienna=numer;
		if (numer<10)
		{
			str[1]=0+48;
			i++;
		}
	}
	if (numer>=10)
	{
		numer=numer/10;
		str[i]=numer+48;
		i++;
		numer=zmienna-numer*10;
	}


	if (numer<10)
		str[i]=numer+48;

	if (MENU!=7 && MENU!=0 && MENU!=9 && MENU!=10)
	{
		i++;
		str[i]='%4.1f\xdf';
		i++;
		str[i]='C';
	}
	else
	{
		if (MENU==0)
		{
			i++;
			str[i]='V';
		}
		else
		{

			if (MENU==7)
			{
				i++;
				str[i]=37;
			}
			else
			{
				if (MENU!=10)
				{
					i++;
					str[i]=' ';
					i++;
					str[i]='s';
					i++;
					str[i]='e';
					i++;
					str[i]='k';
					i++;
					str[i]='.';
				}
			}

		}

	}

}

//---------------------INT MAIN VOID___________________

int main(void)
{
	
	init_io();
	adc_init();
///*


	grzalka_max = eeprom_read_byte(( uint8_t *) 1) ;
	grzalka_min = eeprom_read_byte(( uint8_t *) 2) ;
	out1_max = eeprom_read_byte(( uint8_t *) 3) ;
	out1_min = eeprom_read_byte(( uint8_t *) 4) ;
	out2_max = eeprom_read_byte(( uint8_t *) 5) ;
	out2_min = eeprom_read_byte(( uint8_t *) 6) ;
	a = eeprom_read_byte(( uint8_t *) 7) ;
	alarm = eeprom_read_byte(( uint8_t *) 8) ;
	czas_LCD = eeprom_read_byte(( uint8_t *) 9) ;
	dzielnik = eeprom_read_byte((uint8_t *) 10) ;

	if (grzalka_max>200)
		grzalka_max=100;
	if (grzalka_min>200)
		grzalka_min=100;
	if (out1_max>200)
		out1_max=100;
	if (out1_max>200)
		out1_min=100;
	if (out2_max>200)
		out2_max=100;
	if (out2_min>200)
		out2_min=100;
	if (alarm>200)
		alarm=100;
	if (a>10)
		a=5;
//*/
	      
	/* Funkcja inicjalizuje wy?wietlacz */
	lcd_init();
	/* W??cza wy?wietlanie */
	LCD_DISPLAY(LCDDISPLAY);
	/* Czy?ci  ekran */
	LCD_CLEAR;
  


	//OneWireReset();
	//OneWireWriteByte(0xcc); // SKIP ROM
	//OneWireWriteByte(0x4E); // SKIP ROM
	//OneWireWriteByte(10); // SKIP ROM
	//OneWireWriteByte(10); // SKIP ROM
	//OneWireWriteByte(64); // READ SCRATCHPAD
	//_delay_ms(750);
	LCD_LOCATE(0,0);
	lcd_puts("DRIVER KOLEKTORA");
	LCD_LOCATE(0,1);
	lcd_puts("BY MMI");
	_delay_ms(2000);
	 LCD_CLEAR;

  while(1)
  {


    /* Funkcja 'ds18b20_ConvertT' wysyla polecenie pomiaru do czujnika */      
     if(ds18b20_ConvertT())
    {

       /* 750ms - czas konwersji 12 bitowej (wartosci z datasheet) */
       _delay_ms(300);
       display_caption();
       _delay_ms(300);
       display_caption();
       _delay_ms(300);
       display_caption();

       OneWireReset();
       OneWireWriteByte(0xcc); // SKIP ROM
       OneWireWriteByte(0xBE); // READ SCRATCHPAD
       tempLSB=OneWireReadByte(); //zapis Less significant bit
       tempMSB=OneWireReadByte();	// zapis Most significant bit
          
       
       char text[10];
	   char value;		//zmienna do przeliczania warto�ci z wy�wietlacza na liczby char (aby omin�� warto�ci zmiennoprzecinkowe i zaoszczedzi� 1,7kB)
	   DSTempToStr( text, tempLSB, tempMSB);
//*****************przeliczenie wartosci string na char do obliczen temperatury***********

		if (text[2]=='.')
		
			value=text[1]-48;
			else
			{
				if (text[3]=='.')
				value=(text[1]-48)*10+text[2]-48;
				else
				value=(text[1]-48)*100+(text[2]-48)*10+text[3]-48;
			
			}
			
		if (text[0]=='-') value=value*-1;
		
//****************************************************************************************		

		if (MENU==0)
		{
			LCD_LOCATE(0,0);
			lcd_puts(" TEMP.  | VOLT. ");
			LCD_LOCATE(0,1);					//od�wiezanie linii wy�wietlacza
			lcd_puts("                ");		//od�wiezanie linii wy�wietlacza

			LCD_LOCATE(10,1);
			numbertostr(ADC/dzielnik);
			lcd_puts(str);

			LCD_LOCATE(0,1);
			lcd_puts(text);						//wy�wietlanie temperatury na wy�wietlaczu

		}
	



		if ( text[0]=='+')
			//grzalka_ON
			//else
			{
			if (value==checkvalue)
			{
				if (value>grzalka_max || value<grzalka_min)
				{
					allow=0;
					grzalka_OFF;
				}

				else
				{
					if (value>=grzalka_min && value<=grzalka_max)
					allow=1;		//zezwolenie na pwm opis na dole
					//grzalka_ON;	//tutaj PWM zamiast stałego włączenia
				}
			


				if (value>out1_max || value<out1_min)
				{
					out1_OFF;
				}

				else
				{
					if (value>=out1_min && value<=out1_max)
					out1_ON;
				}

				if (value>=alarm)
				{
				buzzer_OFF;
				_delay_ms(100);
				buzzer_ON;
				}
				else
				{
					if (value<alarm)
					buzzer_OFF;
				}



				if (value>out2_max || value<out2_min)
				{
					out2_OFF;
				}

				else
				{
					if (value>=out2_min && value<=out2_max)
					out2_ON;
				}


			}

			}

		checkvalue=value;
		if (MENU==0)
		{
			if (counter_LCD>((czas_LCD-1)*100))
				LED_LCD_OFF;
			else
				LED_LCD_ON;
		}
    }
	// Napis po nie wykryciu czujnika
	else
	{
		if (MENU==0)
		{
			LCD_LOCATE(0,1);
			lcd_puts("ERROR NO SENSOR!");
			OCR1A=0;
			buzzer_OFF;
			_delay_ms(1000);
			OCR1A=210;
			buzzer_ON;
			_delay_ms(900);
		}
		grzalka_OFF;
		out1_OFF;
		out2_OFF;

	}
  }
}


// ========================================================================

ISR(TIMER0_OVF_vect)           // interrupt every 10ms
{
	KeybProc();	// Check if button pressed
	cli();

	if( IsKey( KEY_ENTER ))
	{
		_key = 1;
		_key_time = KeysTime();
	}




	if( IsKey( KEY_ENTER ) && KeysTime( ) >= 180 )
	{
		ClrKeyb( KBD_LOCK );	// autorepetition blocking
		if(MENU==0)
		{
			LCD_CLEAR;
			LCD_LOCATE(0,0);
			lcd_puts("   USTAWIENIA   ");
			MENU=1;
			refresh=1;
		}
		else
		{
			MENU=0;
		}
	}
	else
		if( ((KEY_PIN & KEY_ENTER)) && ( _key_time <100) )
		{
			if (_key == 1)
			{
				_key=0;
				ClrKeyb( KBD_LOCK );	// autorepetition blocking
				counter_LCD=0;

				if(MENU>=1 && MENU<11)
				MENU++;
				else
				{
					if(MENU>=20 && MENU<24)
						MENU++;
				}
				refresh=1;

				switch(MENU)
				{
					case 3:
					{
						eeprom_update_byte(( uint8_t *) 1 , grzalka_max ) ;

					}break;

					case 2:
					{
						eeprom_update_byte(( uint8_t *) 2 , grzalka_min ) ;

					}break;

					case 5:
					{
						eeprom_update_byte(( uint8_t *) 3 , out1_max ) ;

					}break;

					case 4:
					{
						eeprom_update_byte(( uint8_t *) 4 , out1_min ) ;

					}break;

					case 7:
					{
						eeprom_update_byte(( uint8_t *) 5 , out2_max) ;

					}break;

					case 6:
					{
						eeprom_update_byte(( uint8_t *) 6 , out2_min ) ;

					}break;

					case 8:
					{
						//uint8_t ByteOfData ;
						//ByteOfData = a ;
						eeprom_update_byte(( uint8_t *) 7 , a ) ;
					}break;


					case 9:
					{
						eeprom_update_byte(( uint8_t *) 8 , alarm ) ;

					}break;

					case 10:
					{
						eeprom_update_byte(( uint8_t *) 9 , czas_LCD ) ;

					}break;

					case 11:
					{
						eeprom_update_byte(( uint8_t *) 10 , dzielnik ) ;

					}break;

				}

			}
		}

	//----------------------------------------------------------------
	    switch( GetKeys() )
	    {
			//----------BACK_KEY_PRESSED----------------------------------
		    case KEY_UP:
			//ClrKeyb( KBD_LOCK );	// autorepetition blocking
		    	refresh=1;
				counter_LCD=0;
			switch(MENU)
			{
				case 2:
				{
					if(grzalka_max<100 && grzalka_max>=grzalka_min)
						grzalka_max++;
					numbertostr(grzalka_max);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;

				case 1:
				{
					if (grzalka_min>=0 && grzalka_min<grzalka_max)
						grzalka_min++;
					numbertostr(grzalka_min);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;

				case 4:
				{
					if(out1_max<100 && out1_max>=out1_min)
						out1_max++;
					numbertostr(out1_max);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;

				case 3:
				{
					if (out1_min>=0 && out1_min<out1_max)
						out1_min++;
					numbertostr(out1_min);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;

				case 6:
				{
					if(out2_max<100 && out2_max>=out2_min)
						out2_max++;
					numbertostr(out2_max);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;

				case 5:
				{
					if (out2_min>=0 && out2_min<out2_max)
						out2_min++;
					numbertostr(out2_min);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;

				case 7:
				{
					if(lcd[a]>1)
					{
						a++;
					}
					numbertostr(a*10);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;

				case 8:
				{
					if (alarm<100)
					alarm++;
					numbertostr(alarm);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;

				case 9:
				{
					if (czas_LCD<250)
					czas_LCD++;
					numbertostr(czas_LCD);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;

				case 10:
				{
					if (dzielnik<250)
					dzielnik++;
					numbertostr(dzielnik);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;


				case 20:
				{
					LCD_LOCATE(0,1);
					buzzer_ON;
					lcd_puts("       ON       ");
				}break;


				case 21:
				{
					LCD_LOCATE(0,1);
					out1_ON;
					lcd_puts("       ON       ");

				}break;


				case 22:
				{
					LCD_LOCATE(0,1);
					out2_ON;
					lcd_puts("       ON       ");

				}break;


				case 23:
				{
					LCD_LOCATE(0,1);
					grzalka_ON;
					lcd_puts("       ON       ");

				}break;

			}

			break;
			//-------------------------------------------------------------------

			//case KEY_ENTER:
			//ClrKeyb( KBD_LOCK );	// autorepetition blocking
			//MENU++;

			//break;
			//-------------------------------------------------------------
			 case  KEY_DOWN:
			//ClrKeyb( KBD_LOCK );	// autorepetition blocking
				refresh=1;
				counter_LCD=0;
			switch(MENU)
			{
				case 2:
				{
					if(grzalka_max>=0 && grzalka_max>grzalka_min)
						grzalka_max--;
					numbertostr(grzalka_max);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;

				case 1:
				{
					if (grzalka_min>0 && grzalka_min<=grzalka_max)
						grzalka_min--;
					numbertostr(grzalka_min);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;

				case 4:
				{
					if(out1_max>=0 && out1_max>out1_min)
						out1_max--;
					numbertostr(out1_max);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;

				case 3:
				{
					if (out1_min>0 && out1_min<=out1_max)
						out1_min--;
					numbertostr(out1_min);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;

				case 6:
				{
					if(out2_max>=0 && out2_max>out2_min)
						out2_max--;
					numbertostr(out2_max);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;

				case 5:
				{
					if (out2_min>0 && out2_min<=out2_max)
						out2_min--;
					numbertostr(out2_min);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;

				case 7:
				{
					if (lcd[a]<254)
					{
						a--;
					}
					numbertostr(a*10);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;

				case 8:
				{
					if (alarm>0)
						alarm--;
					numbertostr(alarm);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;


				case 9:
				{
					if (czas_LCD>0)
						czas_LCD--;
					numbertostr(czas_LCD);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;

				case 10:
				{
					if (dzielnik>0)
						dzielnik--;
					numbertostr(dzielnik);
					LCD_LOCATE(0,1);
					lcd_puts("                ");
					LCD_LOCATE(6,1);
					lcd_puts(str);

				}break;


				case 20:
				{
					LCD_LOCATE(0,1);
					buzzer_OFF;
					lcd_puts("       OFF      ");
				}break;


				case 21:
				{
					LCD_LOCATE(0,1);
					out1_OFF;
					lcd_puts("       OFF      ");

				}break;


				case 22:
				{
					LCD_LOCATE(0,1);
					out2_OFF;
					lcd_puts("       OFF       ");

				}break;


				case 23:
				{
					LCD_LOCATE(0,1);
					grzalka_OFF;
					lcd_puts("       OFF      ");

				}break;

			}


			break;
			//-----------------------------------------------------------------


			case  KEY_UP | KEY_DOWN:
			{

				if(MENU==0)
				{
					LCD_CLEAR;
					LCD_LOCATE(0,0);
					lcd_puts("   TEST WYJSC   ");
					MENU=20;
					refresh=1;
				}
				else
				{
					//MENU=0;
				}

			}
			break;


		    default:
		    break;
	    }


	//--------------------------------------------------





	//		if( IsKey( ANYKEY ))
	//		{
	//		}
	if (counter_LCD<59000)
		counter_LCD++;

	TCNT0 = 171;


	sei();
}




	ISR(ADC_vect)//obs?uga przerwania po zako?czeniu konwersji ADC
	{

		if (allow==1)
		{
			if ((ADC >=  700) )    //przeliczenie warto?ci na napi?cie

			{
				grzalka_ON=255;
			}
			else
			{
				if (ADC >= 350 )
				{
					grzalka_ON=170;
				}

				else
				{
					if (ADC >= 200)
					{

						grzalka_ON=70;
					}
					else
					if (ADC >= 100)
					{

						grzalka_ON=20;
					}
					else
					{
						grzalka_ON=0;

					}

				}
			}


		}
	}
