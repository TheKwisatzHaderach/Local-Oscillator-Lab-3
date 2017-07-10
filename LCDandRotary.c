// ------------------------------------------------------------------------------------
//
// Author: Troy Davis, Sterling Collins, Chris Garcia
// Company: Texas Tech University
// Department: EE
// Status: Incomplete
//
// Date: 7/05/2017
// Assembler: Code Composer Studio
// Target Board:  MSP430G2553 with TI LaunchPad
//
// Flash Used:
// RAM Used:
// I/O Pins Used:
//
// Description:
//
// I/O Assignments:
//
//      16x2 Screen + Extra:
//          P1.0: (output) LCD D4
//          P1.1: (output) LCD D5
//          P1.2: (output) LCD D6
//          P1.3: (output) LCD D7
//          P1.4: (output) LCD E
//          P1.5: (output) LCD RS
//          P1.6: (TBD)
//          P1.7: (TBD)
//          LCD VDD - 5V (TP1 on MSP430 LaunchPad)
//          LCD VSS - GND
//          LCD RW  - GND
//          LCD V0  - 3V through pot (adjusts contrast)
//          LCD K   - GND
//          LCD A   - 5V through pot (adjusts brightness)
//
//      Rotary Encoder with SW:
//
//      AD5932 DDS Chip:
//
// Comments:
//
// -------------------------------------------------------------------------------------
#include <msp430.h>

#define LCD_EN      BIT4
#define LCD_RS      BIT5
#define top         0x80 // top line of LCD starts here and goes to 8F
#define bottom      0xC0 // bottom line of LCD starts here and goes to CF

#define ENCODER_A   BIT0 //rotary encoder pin A clk
#define ENCODER_B   BIT5 //rotary encoder pin B DT
#define SW          BIT3 // rotary switch button

#define PVLength    8 //length of place value array

volatile unsigned int PVarray[PVLength] = {50,48,48,48,48,48,48,48}; // array holding Ascii value in decimal for each place value in Frequency between 19-21MHz
volatile unsigned int PVindex = 0; //value that will index through place value array

void lcd_reset();
void lcd_pos(char pos);
void lcd_setup();
void lcd_data(unsigned char dat);
void lcd_display_top(char *line);
void encoderInit();
void switchInit();

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer

    encoderInit();
    lcd_setup();
    switchInit();

    while(1)
    {
    	if(~P2IN & SW)
    	{
    		PVindex = PVindex+1;
    		if(PVindex > 7)
    		{
    			PVindex = 0;
    		}
    		while(~P2IN & SW){}
    	}
    	lcd_display_top("Frequency:       ");
    	lcd_pos(0xC6);
    	lcd_data(PVarray[0]);
    	lcd_data(PVarray[1]);
    	lcd_data(PVarray[2]);
    	lcd_data(PVarray[3]);
    	lcd_data(PVarray[4]);
    	lcd_data(PVarray[5]);
    	lcd_data(PVarray[6]);
        lcd_data(PVarray[7]);
        lcd_data(72); //char H
        lcd_data(122); // char z
    }
}

//LCD
void lcd_reset()
{
    P1DIR = 0xff;
    P1OUT = 0xff;
    __delay_cycles(20000);
    P1OUT = 0x03+LCD_EN;
    P1OUT = 0x03;
    __delay_cycles(10000);
    P1OUT = 0x03+LCD_EN;
    P1OUT = 0x03;
    __delay_cycles(1000);
    P1OUT = 0x03+LCD_EN;
    P1OUT = 0x03;
    __delay_cycles(1000);
    P1OUT = 0x02+LCD_EN;
    P1OUT = 0x02;
    __delay_cycles(1000);
}

void lcd_pos (char pos) //16*2
{
    P1OUT = ((pos >> 4) & 0x0F)|LCD_EN;
    P1OUT = ((pos >> 4) & 0x0F);

    P1OUT = (pos & 0x0F)|LCD_EN;
    P1OUT = (pos & 0x0F);

    __delay_cycles(4000);
}

void lcd_setup()
{
    lcd_reset();         // Call LCD reset
    lcd_pos(0x28);       // 4-bit mode - 2 line - 5x7 font.
    lcd_pos(0x0C);       // Display no cursor - no blink.
    lcd_pos(0x06);       // Automatic Increment - No Display shift.
    lcd_pos(0x80);       // Address DDRAM with 0 offset 80h.
    lcd_pos(0x01);       // Clear screen
}


void lcd_data (unsigned char dat)//display number(hz)
{
	P1OUT = (((dat >> 4) & 0x0F)|LCD_EN|LCD_RS);
	P1OUT = (((dat >> 4) & 0x0F)|LCD_RS);

	P1OUT = ((dat & 0x0F)|LCD_EN|LCD_RS);
	P1OUT = ((dat & 0x0F)|LCD_RS);

    __delay_cycles(400);
}

void lcd_display_top(char *line) //displays strings
{
    lcd_pos(top);
    while (*line)
        lcd_data(*line++);
}
//LCD

//encoder
void encoderInit()
{

    P2OUT |= (ENCODER_A+ENCODER_B); //enable pull-up resistor
    P2REN |= ENCODER_A+ENCODER_B;   //enable pull-up resistor
    P2IFG &= ~(ENCODER_A);            //clear interupt flag
    P2IE |= ENCODER_A;
    __enable_interrupt();
}

void switchInit()
{
	P2OUT |= SW;
	P2REN |= SW;
}


#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
{
	if(P2IN & ENCODER_B) //one step CCW (& is bitwise AND)
    {
    	PVarray[PVindex] = PVarray[PVindex]-1;
    	if (PVarray[PVindex] < 48)
    	{
    		PVarray[PVindex] = 57;
    	}
    	P2IFG &= ~ENCODER_A;    //clear interupt flag
    }
    else  //one step CW
    {
    	PVarray[PVindex] = PVarray[PVindex]+1;
    	if (PVarray[PVindex] > 57)
    	{
    		PVarray[PVindex] = 48;
    	}
    	P2IFG &= ~ENCODER_A;    //clear interupt flag
    }
}
// encoder
