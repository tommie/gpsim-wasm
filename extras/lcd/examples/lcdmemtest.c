#ifdef EEE
#include <pic.h>
#include "delay.h"
#endif
#include <pic16f873.h>

#ifdef RRR
__CONFIG(LVPDIS & BOREN & PWRTEN & WDTDIS & HS);
#endif

/*
 *	lcdmemtest.c
 *
 *	Test the memory of an LCD display.
 *	Converted to PIC 16F870 May 2004
 *	Copyright (c) 2003-2004, Bert Driehuis <driehuis@playbeing.org>
 *
 *	This code is subject to the BSD license.
 */

#define RS_0	0	// bit #5 off
#define RS_1	0x10	// bit #5 on
#define RW_0	0	// bit 6 off
#define RW_1	0x40	// bit 6 on
#define E_0	0x00
#define E_1	0x20

#define usleep(n)	DelayUs(n * 4)
#define sleep_1ms()	DelayMs(1)
#define sleep_100ms()	DelayMs(100)

/*
 *	Bang out D0-D7 with RS=0 on the 4-bit parallel lcd bus
 */
static void
bang_rs0(unsigned char arg)
{
	PORTC = ((arg & 0xf0) >> 4) | RW_0 | RS_0 | E_1;
	usleep(1);
	PORTC &= ~E_1;
	usleep(1);
	PORTC = (arg & 0x0f) | RW_0 | RS_0 | E_1;
	usleep(1);
	PORTC &= ~E_1;
	usleep(40);
}

static unsigned char
read_rs0(void)
{
	unsigned char rv = 0;
	TRISC = 0x0f;	// PORTC<3:0> as input

	PORTC = RW_1 | RS_0 | E_0;	// Raise RW to select read mode
	PORTC = RW_1 | RS_0 | E_1;	// Raise E
	usleep(1);
	PORTC = RW_1 | RS_0 | E_0;	// Drop E. Could read BF,AC<6:4> now
	usleep(1);
	PORTC = RW_1 | RS_0 | E_1;	// Raise E
	usleep(1);
	PORTC = RW_1 | RS_0 | E_0;	// Drop E. Could read AC<3:0> now
	usleep(1);

	PORTC = RW_1 | RS_1 | E_0;	// Raise RS (data mode)
	PORTC = RW_1 | RS_1 | E_1;	// Raise E
	usleep(1);			// Wait for IO lines to settle
	rv = (PORTC & 0x0f) << 4;

	PORTC = RW_1 | RS_1 | E_0;	// Drop E
	usleep(1);
	PORTC = RW_1 | RS_1 | E_1;	// Raise E
	usleep(1);			// Wait for IO lines to settle
	rv |= (PORTC & 0x0f);
	usleep(1);
	PORTC = RW_1 | RS_0 | E_0;	// Drop all lines to steady state
	PORTC = RW_0 | RS_0 | E_0;	// Drop all lines to steady state

	TRISC = 0;
	return rv;
}

static void
bang_rs1(unsigned char arg)
{
	PORTC = ((arg & 0xf0) >> 4) | RW_0 | RS_1 | E_1;
	usleep(1);
	PORTC &= ~E_1;
	usleep(1);
	PORTC = (arg & 0x0f) | RW_0 | RS_1 | E_1;
	usleep(1);
	PORTC &= ~E_1;
	usleep(40);
}

static unsigned char
read_cgram(unsigned char pos)
{
	unsigned char da_ta;
	if (pos > 64)
		return 0;
	bang_rs0(0x40 + pos);			// set cgram address
	da_ta = read_rs0();
	bang_rs0(0x80);				// home cursor; data output
	return da_ta;
}

static void
erase_cgram()
{
	unsigned char i;

	bang_rs0(0x40);	// Set CGRAM cursor to 0
	for (i = 0; i < 64; i++) {
		bang_rs1(0x00);
	}
}

#define write_ddram_at(where, what)		\
	bang_rs0(0x80 + where);			\
	bang_rs1(what);

/*
 *	Set up the character locations that point to the user defined
 *	fonts. Yes, this table is weird, but the most logical mapping
 *	(sequential) causes aliasing of LCD segments.
 */
static void
init_special_ddram()
{
	write_ddram_at(66, 0);
	write_ddram_at(75, 1);
	write_ddram_at(68, 2);
	write_ddram_at(69, 3);
	write_ddram_at(70, 4);
	write_ddram_at(79, 5);
	write_ddram_at(72, 6);
	write_ddram_at(77, 7);
}

main()
{
	static unsigned char i;
	unsigned char cgram_byte;
	i = 0;

	TRISC = 0;
	TRISA = 0xfe;
	PORTC = 0;
	sleep_1ms();
	bang_rs0(0x33); // FN SET DL=1 N=0 F=0 DB1=1 DB0=1
	bang_rs0(0x32); // FN SET DL=0 N=0 F=0 DB1=1 DB0=1 (set 4bit)
	bang_rs0(0x28); // FN SET DL=0 N=1 F=0 DB1=0 DB0=0
	bang_rs0(0x01); // Clear screen
	sleep_1ms();
	sleep_1ms();
	bang_rs0(0x02); // Unshift
	sleep_1ms();
	sleep_1ms();
	bang_rs0(0x06); // Entry mode I/D=1 S=0
	bang_rs0(0x0c); // DISPCTL on=1 curs=1 blink=0

	bang_rs1('A');
	bang_rs1('a');
	bang_rs1('p');
	bang_rs1(' ');
	//sendstring("Noot");
	bang_rs1('.');

	erase_cgram();
	init_special_ddram();

	bang_rs0(0x90);
	bang_rs1('-');
	bang_rs1('-');
	for (i = 0; i < 64; i++) {
		bang_rs0(0x40 | i);
		bang_rs1(i);
	}
	for (i = 0; i < 64; i++) {
		cgram_byte = read_cgram(i);
		if (cgram_byte != i) {
			bang_rs0(0x90);
			bang_rs1('0' + ((cgram_byte & 0xf0) >> 4));
			bang_rs1('0' + (cgram_byte & 0x0f));
			bang_rs1('0' + ((i & 0xf0) >> 4));
			bang_rs1('0' + (i & 0x0f));
		}
	}
	erase_cgram();

	while (1) {
		PORTA = 1;
		sleep_100ms();
		sleep_100ms();
		sleep_100ms();
		sleep_100ms();
		bang_rs0(0x8d);
		bang_rs1('+');
		PORTA = 0;
		sleep_100ms();
		sleep_100ms();
		sleep_100ms();
		sleep_100ms();
		bang_rs0(0x8d);
		bang_rs1('*');
	}
}
