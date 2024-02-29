/*
 * \brief Manually communicate with the 23LC1024 via SPI
 *
 * Plug the SRAMBoard onto Port B (J12) facing towards the side with the LEDs
 * and provide it with power from J3 or J4. 
 * Connect the LCD (J15) to Port A (J11), i.e. connect R/W to Port A6, EN to
 * Port A5, RS to Port A4, DB7 to Port A3, DB6 to Port A2, DB5 to Port A1,
 * and DB4 to Port A0. 
 * Connect the Buttons SW1..3 (J6) to Port C0, C1, and C6 (J13). 
 * Connect an LED (doesn't matter which) to PC7. 
 */

#include<avr/io.h>
#include<util/delay.h>
#include<stdbool.h>
#include"lcd.h"

/**
 * \brief Stores the (recent) history of the SPI lines
 */
typedef struct {uint16_t mosi, sck, cs, miso;} SPI;

/**
 * \brief Displays the history of an SPI pin as a graph 
 */
void showHistory(uint16_t history, uint8_t length)
{
	for(uint8_t i = 0; i < length; i++)
		lcd_writeChar((history >> (length - i - 1)) & 0x03);
}

/**
 * \brief Updates the graphs on the LCD
 */
void updateLcd(SPI history)
{
	// Update LCD
	lcd_goto(1, 2);
	showHistory(history.mosi, 12);
	lcd_goto(2, 2);
	showHistory(history.sck, 12);
	lcd_goto(1, 15);
	showHistory(history.cs, 2);
	lcd_goto(2, 15);
	showHistory(history.miso, 2);
}

/**
 * \brief Retrieves the state of the MISO line and the buttons and stores it in
 * one byte: 0b(0 0 0 0 <MISO> <SW3> <SW2> <SW1>)
 */
inline uint8_t misoBtnState()
{
	return ((PINC >> 4) & 4) | (PINC & 3) | ((PINB >> 3) & 8);
}

/**
 * \brief Checks what changes have happened between two states.
 * For MISO any change counts, for the buttons only falling edges.
 */
inline uint8_t misoBtnChanges(uint8_t oldState, uint8_t newState)
{
	return ((oldState ^ newState) & 8) | ((oldState & ~newState) & 7);
}

/**
 * \brief Main program
 */
void main(void)
{
	// Initialise LCD
	lcd_init();
	lcd_registerCustomChar(0, CUSTOM_CHAR(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f)); // low
	lcd_registerCustomChar(1, CUSTOM_CHAR(0x17, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10)); // rising
	lcd_registerCustomChar(2, CUSTOM_CHAR(0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x17)); // falling
	lcd_registerCustomChar(3, CUSTOM_CHAR(0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)); // high
	lcd_registerCustomChar(4, CUSTOM_CHAR(0x06, 0x09, 0x06, 0x1f, 0x08, 0x04, 0x08, 0x1f)); // MOSI
	lcd_registerCustomChar(5, CUSTOM_CHAR(0x00, 0x11, 0x11, 0x0e, 0x00, 0x12, 0x15, 0x09)); // SCK
	lcd_registerCustomChar(6, CUSTOM_CHAR(0x12, 0x15, 0x09, 0x00, 0x11, 0x11, 0x0e, 0x00)); // CS
	lcd_registerCustomChar(7, CUSTOM_CHAR(0x00, 0x17, 0x00, 0x1f, 0x08, 0x04, 0x08, 0x1f)); // MISO
	lcd_writeString("              ");
	lcd_writeString("              ");

	// Initialise SPI pins:
	// MOSI (PB5) and SCK (PB7) as ouput low, CS (PB4) as output high,
	// MISO (PB6) as input without pull-up
	PORTB = 0b00010000;
	DDRB = 0b10110000;

	// Initialise button pins:
	// SW1 (PC0), SW2 (PC1), SW3 (PC6) as inputs with pull-up
	DDRC = 0;
	PORTC = 0b01000011;

	// Store the history of MOSI, MISO, SCK, and CS
	SPI history = {.mosi = 0, .sck = 0, .cs = 0xffff, .miso = (PINB >> 6) & 1 ? 0xffff : 0};
	updateLcd(history);

	// Main loop
	uint8_t state = misoBtnState();
	while(1)
	{
		// Check if a button is pressed or the MISO pin changed
		uint8_t oldState = state;
		state = misoBtnState();
		uint8_t changes = misoBtnChanges(oldState, state);
		if(changes)
		{
			// Update histories
			history.mosi = (history.mosi << 1) | (changes & 1 ? ~history.mosi & 1 : history.mosi & 1);
			history.sck = (history.sck << 1) | (changes & 2 ? ~history.sck & 1 : history.sck & 1);
			history.cs = (history.cs << 1) | (changes & 4 ? ~history.cs & 1 : history.cs & 1);
			history.miso = (history.miso << 1) | ((PINB >> 6) & 1);

			// Update MOSI, SCK, and CS pins
			PORTB = (PORTB & 0b01001111)
			      | ((uint8_t)(history.mosi & 1) << 5)
			      | ((uint8_t)(history.sck & 1) << 7)
			      | ((uint8_t)(history.cs & 1) << 4);

			// Update LCD
			updateLcd(history);
		}
		_delay_ms(10);
	}
}

