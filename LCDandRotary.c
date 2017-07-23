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
//          P2.0: (output) LCD D4
//          P2.1: (output) LCD D5
//          P2.2: (output) LCD D6
//          P2.3: (output) LCD D7
//          P2.4: (output) LCD E
//          P2.5: (output) LCD RS
//          LCD VDD - 5V (TP1 on MSP430 LaunchPad)
//          LCD VSS - GND
//          LCD RW  - GND
//          LCD V0  - 3V through pot (adjusts contrast)
//          LCD K   - GND
//          LCD A   - 5V through pot (adjusts brightness)
//
//      Rotary Encoder with SW:
//			P1.7: (input) signal A CW
//			P1.5: (input) signal B CCW
//			P1.3: (input) SW
//
//      AD5932 DDS Chip:
//          P1.x: (output) CTRL
//          P1.x: (output) SCLK
//          P1.x: (output) SDATA
//          P1.x: (output) FSYNC
//
// Comments:
//
// -------------------------------------------------------------------------------------
#include <msp430.h>

#define LCD_EN      BIT4
#define LCD_RS      BIT5
#define top         0x80 // top line of LCD starts here and goes to 8F
#define bottom      0xC0 // bottom line of LCD starts here and goes to CF

#define ENCODER_A   BIT7 //rotary encoder pin A clk
#define ENCODER_B   BIT5 //rotary encoder pin B DT
#define SW          BIT3 // rotary switch button

#define PVLength    8 //length of place value array

volatile unsigned int PVarray[PVLength] = {50,48,48,48,48,48,48,48}; // array holding Ascii value in decimal for each place value in Frequency between 19-21MHz
volatile unsigned int PVindex = 0; //value that will index through place value array
unsigned long Frequency = 21000000;
float frequencyWord = 20000000;

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

    frequencyWord = (Frequency*.33554432); //(16777216/50000000) = .33554432

    while(1)
    {
    	if(~P1IN & SW)
    	{
    		PVindex = PVindex+1;
    		frequencyWord = (Frequency*.33554432);
    		if(PVindex > 7)
    		{
    			PVindex = 0;
    		}
    		while(~P1IN & SW){}
    	}
    	Frequency = ((PVarray[0]-48)*10000000) + ((PVarray[1]-48)*1000000) + ((PVarray[2]-48)*100000) + ((PVarray[3]-48)*10000) + ((PVarray[4]-48)*1000) + ((PVarray[5]-48)*100) + ((PVarray[6]-48)*10) + ((PVarray[7]-48));
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
    P2DIR = 0xff;
    P2OUT = 0xff;
    __delay_cycles(20000);
    P2OUT = 0x03+LCD_EN;
    P2OUT = 0x03;
    __delay_cycles(10000);
    P2OUT = 0x03+LCD_EN;
    P2OUT = 0x03;
    __delay_cycles(1000);
    P2OUT = 0x03+LCD_EN;
    P2OUT = 0x03;
    __delay_cycles(1000);
    P2OUT = 0x02+LCD_EN;
    P2OUT = 0x02;
    __delay_cycles(1000);
}

void lcd_pos (char pos) //16*2
{
    P2OUT = ((pos >> 4) & 0x0F)|LCD_EN;
    P2OUT = ((pos >> 4) & 0x0F);

    P2OUT = (pos & 0x0F)|LCD_EN;
    P2OUT = (pos & 0x0F);

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
	P2OUT = (((dat >> 4) & 0x0F)|LCD_EN|LCD_RS);
	P2OUT = (((dat >> 4) & 0x0F)|LCD_RS);

	P2OUT = ((dat & 0x0F)|LCD_EN|LCD_RS);
	P2OUT = ((dat & 0x0F)|LCD_RS);

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
    P1OUT |= (ENCODER_A+ENCODER_B); //enable pull-up resistor
    P1REN |= (ENCODER_A+ENCODER_B);   //enable pull-up resistor
    P1IFG &= ~(ENCODER_A);            //clear interupt flag
    P1IE |= ENCODER_A;
    __enable_interrupt();
}

void switchInit()
{
	P1OUT |= SW;
	P1REN |= SW;
}


#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
	if(P1IN & ENCODER_B) //one step CCW (& is bitwise AND)
    {
		PVarray[PVindex] = PVarray[PVindex]-1;
    	if (PVarray[PVindex] < 48)
    	{
    		PVarray[PVindex] = 57;
    	}
		if(PVarray[0] < 49)
		{
			PVarray[0] = 49;
		}
		if((PVarray[0] == 49) && (PVarray[1] != 57))
		{
			PVarray[1] = 57;
		}
		if((PVarray[0] == 50) && !((PVarray[1] == 48) || (PVarray[1] == 49)))
		{
			PVarray[1] = 48;
		}
		if(((PVarray[0] == 50)&&(PVarray[1] == 49))&&((PVarray[2] != 48)||(PVarray[3] != 48)||(PVarray[4] != 48)||(PVarray[5] != 48)||(PVarray[6] != 48)||(PVarray[7] != 48)))
		{
			PVarray[2] = 48;
			PVarray[3] = 48;
		    PVarray[4] = 48;
		    PVarray[5] = 48;
		    PVarray[6] = 48;
		    PVarray[7] = 48;
		}
    	P1IFG &= ~ENCODER_A;    //clear interupt flag
    }
    else  //one step CW
    {
    	PVarray[PVindex] = PVarray[PVindex]+1;
    	if (PVarray[PVindex] > 57)
    	{
    		PVarray[PVindex] = 48;
    	}
    	if(PVarray[0] > 50)
    	{
    		PVarray[0] = 50;
    	}
    	if((PVarray[0] == 50) && !((PVarray[1] == 48) || (PVarray[1] == 49)))
    	{
    		PVarray[1] = 49;
    	}
    	if((PVarray[0] == 49) && (PVarray[1] != 57))
    	{
    		PVarray[1] = 57;
    	}
    	if(((PVarray[0] == 50)&&(PVarray[1] == 49))&&((PVarray[2] != 48)||(PVarray[3] != 48)||(PVarray[4] != 48)||(PVarray[5] != 48)||(PVarray[6] != 48)||(PVarray[7] != 48)))
    	{
    		PVarray[2] = 48;
    	    PVarray[3] = 48;
    	    PVarray[4] = 48;
    	    PVarray[5] = 48;
    	    PVarray[6] = 48;
    	    PVarray[7] = 48;
    	}
    	P1IFG &= ~ENCODER_A;    //clear interupt flag
    }
}
// encoder
