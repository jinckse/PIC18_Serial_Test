/*
 * File: main.c
 * Description: USART serial test
 * Name: Jarrod Nix
 * Created: December 5, 2016
 */

#include <p18f4520.h>
#include <delays.h>
#include <EEP.h>
#include <errno.h>
#include <stdio.h>
#include "global.h"

/*
 * I N T E R R U P T S
 */
#pragma code High_Priority_Int = 0x0008
void High_Priority_Int(void) {
    _asm GOTO High_ISR _endasm
}

#pragma code Low_Priority_Int = 0x0018
void Low_Priority_Int(void) {
    _asm GOTO Low_ISR _endasm
}

/*
 * I S R s
 */

/*
 * Name: High_ISR()
 * Returns: void
 * Parameters: void
 * Description: Handles high priority interrupts
 */
#pragma interrupt High_ISR
void High_ISR(void) {
    /* Do something */
    
    /* Clear interrupt flag*/
    INTCONbits.INT0F = 0;
}

/*
 * Name: Low_ISR()
 * Returns: void
 * Parameters: void
 * Description: Handles low priority interrupts
 */
#pragma interrupt Low_ISR
void Low_ISR(void) {
    /* Do something*/

    // Load low and high bytes of timer for 1 second countdown
    TMR0H = 0x67;
    TMR0L = 0x6A;
    
    /* Clear interrupt flag*/
    INTCONbits.TMR0IF = 0;
}

/* 
 * M A I N  R O U T I N E
 */

/*
 * Function Name: main()
 * Returns: void
 * Parameters: void
 * Description: main function
 */
void main(void) {
    BYTE b = 0x00;
    /* Initialize system */
    Init();
   
    // Run POST for initial boot
    POST();
    
    // Enable interrupts
    INTCONbits.GIE_GIEH = 1;
    
    T1CON = 0x10;
    
    // Handle menu Selection
    while(1) {
        
        TMR1H = 0;
        TMR1L = 0;
        
        Transmit_Msg("Hello human...\n");
        Delay_ms(1000);
    }
}

/*
 * F U N C T I O N S
 */

/*
 * Function Name: Delay_ms()
 * Returns: void
 * Parameters: m
 * Description: Creates a delay in milliseconds dictated by parameter m
 * to make a specific delay time.
 */
void Delay_ms(unsigned int m) {
    int delay = m;
    int i = 0;
    
    // Each delay cost 1ms
    for (i = 0; i < delay; i++) {
        // Total delay = Argument * DelayType * Tcyc
        Delay100TCYx(50); // 50 * 100 * 0.2us, where Tcyc = 4/freq = 0.2us
    }
}

/*
 * Function Name: Delay_us()
 * Returns: void
 * Parameters: N/A
 * Description: Creates a delay in microseconds in multiples of 10
 * dictated by parameter m to make a specific delay time.
 */
void Delay_us(unsigned int m) {
    int delay = m;
    int i = 0;
    
    // Each delay cost 10us
    for (i = 0; i < delay; i++) {
        // Total delay = Argument * DelayType * Tcyc
        Delay10TCYx(5); // 5 * 10 * 0.2us, where Tcyc = 4/freq = 0.2us
    }
}

/*
 * Function Name: Init()
 * Returns: void
 * Parameters: N/A
 * Description: Initializes SFRs and registers required for peripherals to run
 * as desired at program start.
 */
void Init(void) {
    ADCON0 = 0x01;
    ADCON1 = 0x0E; // Set AN0 as analog input for temperature sensor
    ADCON2 = 0x84;      
    
    TRISA  = 0b01100001;            
    TRISB  = 0b11011101;            
    TRISC  = 0b11000000;        

    /* Set up Port D for controlling distance sensors trig and echo pins */
    TRISD  = 0b01010101;            
    
    /* LEDs */
    TRISE  = 0b00000000;            
    
    // Set up interrupt control bits
    INTCON = 0b01110000; // Enable peripheral, TMR0 overflow, and external interrupts
    INTCON2 = 0b10110001; // Enable PORTB pull-ups, Rising edge, High priority
    INTCON3 = 0; // unused
    
    // Load low and high bytes of timer using 
    // F = fosc/4, D = 1, x = D * F, preload = 65536-(x/prescaler) = 26474
    TMR0H = 0x67;
    TMR0L = 0x6A;
    
    // Set Timer0 module as 16-bit timer, rising source edge, and prescaler
    // of 1:128 
    T0CON = 0b10000110;
    
    // Enable priorities
    RCONbits.IPEN = 1;
}

/*
 * Function Name: Transmit_Msg()
 * Returns: int
 * Parameters: BYTE c
 * Description: Transmit a message via RS232 in manner similar to printf() from
 * stdio.h minus formatting options
 */
int Transmit_Msg(rom const char *s) {
    BYTE i = 0;
    
    // Prepare USART for transmission
    TXSTA = 0x20; // Enable TX
    SPBRG = 31; // Set baud rate generator to 9600 on 20Mhz Xtal
    TXSTAbits.TXEN = 1; 
    RCSTAbits.SPEN = 1; // Enable Serial
    
    // Load circular buffer with message
    for(i = 0; s[i] != '\0'; i++) {
        Circ_Buf_Insert(s[i]);
    }
    
    // Unload circular buffer and send to serial terminal
    for(i = 0; s[i] != '\0'; i++) {
        TXREG = Circ_Buf_Retreive(); 
        while(PIR1bits.TXIF == 0); // Wait for successful transmission
        Delay_ms(1); // Required to make sure return is not corrupted
    }
    
    // Disable transmission when finished
    TXSTAbits.TXEN = 0;
    
    return 1;
    
    // Upon error return negative value
    error:
    return -1;
}

/*
 * Function Name: Transmit_Msg()
 * Returns: int
 * Parameters: BYTE* c
 * Description: Transmit a message via RS232 in form of char* array
 */
int Transmit_Msg2(BYTE* c) {
    BYTE i = 0;
    
    TXSTA = 0x20;
    SPBRG = 31;
    TXSTAbits.TXEN = 1;
    RCSTAbits.SPEN = 1;
    
    // Exhaust string until null byte is encountered
    for(i = 0; c[i] != '\0'; i++) {
        Circ_Buf_Insert(c[i]);
    }
    
    // Same process as above to unload buffer and transmit
    for(i = 0; c[i] != '\0'; i++) {
        TXREG = Circ_Buf_Retreive();
        while(PIR1bits.TXIF == 0);
        Delay_ms(1); // Required to make sure return is not corrupted
    }
    
    TXSTAbits.TXEN = 0;
    
    return 1;
    
    error:
    return -1;
}

/*
 * Function Name: Receive_Msg()
 * Returns: BYTE
 * Parameters: void
 * Description: Receive a byte via RS232 one at a time
 */
BYTE Receive_Msg(void) {
    BYTE b;
    BYTE tmp;
    
    /* Set up for Receive */
    // Enable serial port and receiver
    RCSTA = 0x90;
    SPBRG = 31;
    
    // Wait to receive
    while(PIR1bits.RCIF == 0); // Wait for successful reception
    
    tmp = RCREG;
    // Write to buffer
    Circ_Buf_Insert(tmp);
    
    b = Circ_Buf_Retreive();
    
    return b;
}

/*
 * Function Name: Circ_Buf_Insert()
 * Returns: int
 * Parameters: BYTE c
 * Description: Insert a BYTE into the circular buffer
 */
int Circ_Buf_Insert(BYTE c) {
	if (g_buffSize < MAX_CIRC_BUF_SIZE) {
		g_buf[g_head] = c; 
		// Advance pointer
		g_head++;
		g_head %= MAX_CIRC_BUF_SIZE;

		// Update buffer size
		g_buffSize++;
	}
	else {
		error:
			return -1;
	}
	
	return 1;
}

/*
 * Function Name: Circ_Buf_Retreive()
 * Returns: int
 * Parameters: BYTE c
 * Description: Retreive a BYTE from the circular buffer.
*/
BYTE Circ_Buf_Retreive(void) {
	BYTE b = g_buf[g_tail];

	if (g_buffSize > 0) {
		// Advance pointer
		g_tail++;
		g_tail %= MAX_CIRC_BUF_SIZE;

		// Update buffer size
		g_buffSize--;
	}
		  
	return b;
}

/*
 * Function Name: Buff_Len()
 * Returns: int
 * Parameters: N/A
 * Description: Return length of circular buffer.
 */
int Buff_Len() {
	return g_buffSize;
	error:
		return -1;
}

/*
 * Function Name: POST
 * Returns: void
 * Parameters: N/A
 * Description: Display System menu to user
 */
void POST(void) {
    // Notify user of successful start-up and blink green LED
    LATEbits.LATE0 = 1;
    Delay_ms(500);
    LATEbits.LATE0 ^= 1;
    Delay_ms(500);
    LATEbits.LATE0 ^= 1;
}
