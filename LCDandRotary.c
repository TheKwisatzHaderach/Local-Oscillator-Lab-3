// ------------------------------------------------------------------------------------
//
// Author: Troy Davis, Sterling, Collins
// Company: Texas Tech University
// Department: EE
// Status: Incomplete
//
// Date: 7/05/2017
// Assembler: Code Composer Studio
// Target Board: TI MSP430G2553 LaunchPad
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
//
//
//      Extra Connections:
//          LCD VDD - 5V (TP1 on MSP430 LaunchPad)
//          LCD VSS - GND
//          LCD RW  - GND
//          LCD V0  - 3V through pot (adjusts contrast)
//          LCD K   - GND
//          LCD A   - 5V through pot (adjusts brightness)
//

//
// Comments:
//
// -------------------------------------------------------------------------------------

#include <msp430.h>

#define lcd_port        P1OUT
#define lcd_port_dir    P1DIR

#define LCD_EN      BIT4
#define LCD_RS      BIT5
#define top         0x80 // top line of LCD starts here and goes to 8F
#define bottom      0xC0 // bottom line of LCD starts here and goes to CF

#define ENCODER_A   BIT0 //rotary encoder pin A clk
#define ENCODER_B   BIT5 //rotary encoder pin B DT
#define LED1        BIT0
#define LED2        BIT6

volatile unsigned int dig = 53; // Ascii representation (48-57) of numerical char (0-9)
int tens = 48;
int hundreds = 48;
int thousands = 48;
int tensthousands = 48;
int hundredsthousands = 48;
int millions = 48;
int tensmillions = 50;

void lcd_reset();
void lcd_pos(char pos);
void lcd_setup();
void lcd_data(unsigned char dat);
void lcd_display_top(char *line);
void lcd_display_tempC_bottom();
void lcd_display_tempF_bottom();
void encoderInit();

void main(void)
{

    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer

    P1DIR |= LED1 + LED2;
    P1OUT &= ~(LED1 + LED2);

    encoderInit();
    lcd_setup();

    while(1)
    {
    	lcd_display_top("Frequency:       ");
    	lcd_pos(0xC6);
    	lcd_data(tensmillions);
    	lcd_data(millions);
    	lcd_data(hundredsthousands);
    	lcd_data(tensthousands);
    	lcd_data(thousands);
    	lcd_data(hundreds);
    	lcd_data(tens);
        lcd_data(dig);
        lcd_data(72); //char H
        lcd_data(122); // char z
    }
}

//LCD
void lcd_reset()
{
    lcd_port_dir = 0xff;
    lcd_port = 0xff;
    __delay_cycles(20000);
    lcd_port = 0x03+LCD_EN;
    lcd_port = 0x03;
    __delay_cycles(10000);
    lcd_port = 0x03+LCD_EN;
    lcd_port = 0x03;
    __delay_cycles(1000);
    lcd_port = 0x03+LCD_EN;
    lcd_port = 0x03;
    __delay_cycles(1000);
    lcd_port = 0x02+LCD_EN;
    lcd_port = 0x02;
    __delay_cycles(1000);
}

void lcd_pos (char pos) //16*2
{
    lcd_port = ((pos >> 4) & 0x0F)|LCD_EN;
    lcd_port = ((pos >> 4) & 0x0F);

    lcd_port = (pos & 0x0F)|LCD_EN;
    lcd_port = (pos & 0x0F);

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
    lcd_port = (((dat >> 4) & 0x0F)|LCD_EN|LCD_RS);
    lcd_port = (((dat >> 4) & 0x0F)|LCD_RS);

    lcd_port = ((dat & 0x0F)|LCD_EN|LCD_RS);
    lcd_port = ((dat & 0x0F)|LCD_RS);

    __delay_cycles(400);
}

void lcd_display_top(char *line) //displays strings
{
    lcd_pos(top);
    while (*line)
        lcd_data(*line++);
}
//LCD

// encoder
void encoderInit(){

    P2OUT |= (ENCODER_A+ENCODER_B); //enable pull-up resistor
    P2REN |= ENCODER_A+ENCODER_B;   //enable pull-up resistor
    P2IFG &= ~ENCODER_A;            //clear interupt flag
    P2IE |= ENCODER_A;              //enable interupt for encoder

    __enable_interrupt();
}

#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
{
    if(P2IN & ENCODER_B) //one step CCW (& is bitwise AND)
    {
    	P1OUT ^= LED1; //toggle led1
    	dig = dig-1;
    	if (dig < 48)
    	{
    		dig = 57;
    	}
    }
    else  //one step CW
    {
    	P1OUT ^= LED2; //toggle led2
    	dig = dig+1;
    	if (dig > 57)
    	{
    		dig = 48;
    	}
    }
    P2IFG &= ~ENCODER_A;    //clear interupt flag
}
// encoder

