//***********************************************************************
// Plik: keyb.h
//
// Zaawansowana obsługa przycisków i klawiatur
// Wersja:    1.0
// Licencja:  GPL v2
// Autor:     Deucalion
// Email:     deucalion#wp.pl
// Szczegóły: http://mikrokontrolery.blogspot.com/2011/04/jezyk-c-biblioteka-obsluga-klawiatury.html
//
//***********************************************************************


#ifndef _KEYB_H_

#define _KEYB_H_

#define KEY_DDR		DDRC
#define KEY_PORT	PORTC
#define KEY_PIN		PINC
#define KEY0		(1<<PC0)
#define KEY1		(1<<PC1)
#define KEY2		(1<<PC2)
//#define KEY3		(1<<PC3)
//#define KEY4		(1<<PC4)
//#define KEY5		(1<<PC5)
//#define KEY6		(1<<PC6)

#define	KEY_UP			KEY0
#define	KEY_ENTER		KEY1
#define	KEY_DOWN		KEY2


#define ANYKEY		(KEY0 | KEY1 | KEY2 )
#define KEY_MASK	(KEY0 | KEY1 | KEY2 )

#define KBD_LOCK	1
#define KBD_NOLOCK	0

#define KBD_DEFAULT_ART	((void *)0)


void
ClrKeyb( int lock );

unsigned int
GetKeys( void );


unsigned int
KeysTime( void );


unsigned int
IsKeyPressed( unsigned int mask );


unsigned int

IsKey( unsigned int mask );


void
KeybLock( void );

void
KeybSetAutoRepeatTimes( unsigned short * AutoRepeatTab );


#endif
