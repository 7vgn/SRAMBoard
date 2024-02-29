/*
 * Testing the SRAMBoard add-on for the Evaluation Board
 *
 * Plug the SRAMBoard onto Port B (J12) facing towards the side with the LEDs
 * and provide it with power from J3 or J4. 
 * Connect the LCD (J15) to Port A (J11), i.e. connect R/W to Port A6, EN to
 * Port A5, RS to Port A4, DB7 to Port A3, DB6 to Port A2, DB5 to Port A1,
 * and DB4 to Port A0. 
 * 
 * This is a very slow bit-banging implementation. The ATmega's SPI peripheral
 * is not used. 
 */

#include<avr/io.h>
#include<util/delay.h>
#include<stdlib.h>
#include"lcd.h"

//=============================================================================
// Configuration

// All pins can be chosen individually. Default settings are for Port B and
// coincide with the SPI peripheral pins. 

#define MOSI_REG_DDR DDRB
#define MOSI_REG_PORT PORTB
#define MOSI_PIN 5

#define MISO_REG_DDR DDRB
#define MISO_REG_PORT PORTB
#define MISO_REG_PIN PINB
#define MISO_PIN 6

#define SCK_REG_DDR DDRB
#define SCK_REG_PORT PORTB
#define SCK_PIN 7

#define CS_REG_DDR DDRB
#define CS_REG_PORT PORTB
#define CS_PIN 4

//=============================================================================
// SPI helper functions

void spiInit()
{
	// Configure CS as output, high
	CS_REG_PORT &= ~(1 << CS_PIN);
	CS_REG_DDR |= (1 << CS_PIN);

	// Configure MOSI and SCK as output, low
	MOSI_REG_PORT &= ~(1 << MOSI_PIN);
	MOSI_REG_DDR |= (1 << MOSI_PIN);
	SCK_REG_PORT &= ~(1 << SCK_PIN);
	SCK_REG_DDR |= (1 << SCK_PIN);

	// Configure MISO as input with no pull-up
	MISO_REG_PORT &= ~(1 << MISO_PIN);
	MISO_REG_DDR &= ~(1 << MISO_PIN);
}

void spiStart()
{
	// Pull CS low to select the chip
	CS_REG_PORT &= ~(1 << CS_PIN);
	_delay_us(10);
}

void spiStop()
{
	// Drive CS (PB4) high to deselect the chip
	CS_REG_PORT |= (1 << CS_PIN);
	_delay_us(10);
}

uint8_t spiTransfer(uint8_t data)
{
	for(uint8_t i = 0; i < 8; i++)
	{
		// Output data's MSB on MOSI
		MOSI_REG_PORT = (MOSI_REG_PORT & ~(1 << MOSI_PIN)) | ((data >> 7) << MOSI_PIN);
		_delay_us(10);
		// Drive SCK high
		SCK_REG_PORT |= (1 << SCK_PIN);
		_delay_us(10);
		// Read bit from MISO and shift it into data from the left
		data = (data << 1) | ((MISO_REG_PIN >> MISO_PIN) & 1);
		// Pull SCK low again
		SCK_REG_PORT &= ~(1 << SCK_PIN);
		_delay_us(10);
	}
	return data;
}

//=============================================================================
// Main function

void main(void)
{
	// Initialise LCD
	lcd_init();
	lcd_writeString("Initialising...");

	// Initialise SPI
	spiInit();
	_delay_ms(500);

	// Set sequential mode (supposed to be default according to datasheet but isn't)
	spiStart();
	spiTransfer(0x01);
	spiTransfer(0x40);
	spiStop();

	// Do some reading and writing
	while(1)
	{
		// Choose an address and data
		uint32_t address = (uint32_t)random() & 0x0001ffff;
		uint16_t dataSend = (uint16_t)random();
		lcd_clear();
		lcd_writeString("Addr: ");
		lcd_write32bitHex(address);
		lcd_line2();

		// Write some data
		lcd_line2();
		lcd_writeString("W:");
		lcd_writeHexWord(dataSend);
		spiStart();
		spiTransfer(0x02);
		spiTransfer((uint8_t)(address >> 16));
		spiTransfer((uint8_t)(address >> 8));
		spiTransfer((uint8_t)address);
		spiTransfer((uint8_t)(dataSend >> 8));
		spiTransfer((uint8_t)dataSend);
		spiStop();

		// Read it back
		spiStart();
		spiTransfer(0x03);
		spiTransfer((uint8_t)(address >> 16));
		spiTransfer((uint8_t)(address >> 8));
		spiTransfer((uint8_t)address);
		uint16_t dataRecv = ((uint16_t)spiTransfer(0xff) << 8) | (uint16_t)spiTransfer(0xff);
		spiStop();
		lcd_goto(2, 8);
		lcd_writeString("R:");
		lcd_writeHexWord(dataRecv);

		// Check
		if(dataRecv != dataSend)
		{
			lcd_goto(2, 16);
			lcd_writeString("E");
			while(1);
		}

		_delay_ms(2000);
	}
}

