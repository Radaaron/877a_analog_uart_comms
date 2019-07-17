#define _XTAL_FREQ 20000000

#include <xc.h>
#include <stdlib.h>
#include <stdint.h>

// BEGIN CONFIG
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT enabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable bit (BOR enabled)
#pragma config LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off; all program memory may be written to by EECON control)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)
//END CONFIG

volatile unsigned int pot_output;
uint8_t low, high, request; // unsigned int is 16 bits = two bytes

char uart_init(const long int baudrate) {
	unsigned int x;
	x = (_XTAL_FREQ - baudrate * 64) / (baudrate * 64);
	if (x > 255) {
		x = (_XTAL_FREQ - baudrate * 16) / (baudrate * 16);
		BRGH = 1;
	}
	if (x < 256) {
		SPBRG = x;
		SYNC = 0;
		SPEN = 1;
		TRISC7 = 1;
		TRISC6 = 1;
		CREN = 1;
		TXEN = 1;
		return 1;
	}
	return 0;
}

char uart_tx_empty() {
	return TRMT;
}

char uart_data_ready() {
	return RCIF;
}

char uart_read() {
	while (!RCIF);
	return RCREG;
}

void uart_write(char data) {
	while (!TRMT);
	TXREG = data;
}

unsigned int adc_read() {
    __delay_us(50); // minimum time for hold capacitor charging
    // read adc
    GO_nDONE = 1;
    while (GO_nDONE);
    return ((ADRESH << 8 ) + ADRESL); // use all 10 bits
}

void main() {
    // initialize ports and custom peripherals
    TRISA = 0b11111111;
    TRISC = 0b00000110;
    uart_init(9600); // initialize with baud rate
    // configuration registers
    ADCON0 = 0b10010001; // 64*Tosc, AN2 enabled
    ADCON1 = 0b11000000; // 64*Tosc, right-justified to use 10 bits
    // initial values
    while (1) {
        // read analog pin and split into bytes
        pot_output = adc_read(); // average over multiple reads if analog values are noisy
        high = pot_output >> 8;
        low = pot_output;
        // uart communication on request
        if(uart_data_ready()) {
            request = uart_read();
            if(request == 1) { // request for high byte
                uart_write(high);
            } else if (request == 2) { // low byte
                uart_write(low);
            }
        }
    }
}
