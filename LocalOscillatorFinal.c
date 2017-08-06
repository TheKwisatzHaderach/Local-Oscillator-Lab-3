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
// Parts Used:
//            MSP430g2553
//            TI Launchpad
//            AD5932
//            Rotary Encoder
//            LCD 1602
//
// Description: This code is used to program a Local Oscillator that is tunable between 19-21Mhz.
// 			User Input: An LCD 1602 is programmed in 4 bit mode to output the Frequency value which is tuned by the rotary encoder. The rotary encoder turned in a
//                      clockwise motion will increase number from 0-9 at the current place value. Turning counter clockwise will decrease the number form 9-0 at the
//                      present place value. Pressing the rotary encoder switch will change the place value form tens millions - ones and loop back around. The minimum
//                      frequency that can be tuned is 19MHz and the Maximum is 21MHz. The variable frequency computes the value from the LCD screen into one type long
//                      variable. using the frequency word function the frequency word is calculated using floating point to conserve ram. To update the DDS the control
//                      data is sent first and then the fstart data. Then the ctrl pin is driven high to output new frequency.
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
//          P1.0: (output) CTRL
//          P1.4: (output) SCLK
//          P1.2: (output) SDATA
//          P1.6: (output) FSYNC //1.5
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

#define SDATA BIT2
#define CTRL BIT0
#define SCLK BIT4
#define FSYNC BIT6

#define PVLength    8 //length of place value array

volatile unsigned int IFlag = 0; //Flag goes to 1 when there is a rotary encoder state change
static int loopC = 16; //amount of bits for corresponding passed in value
//static int loopW = 23;
static int bcountC = 0x8000; // used to bitwise and with the 16th bit as 1
//static long bcountW = 0x400000; //  used to bitwise and with the 23th bit as 1

volatile unsigned int PVarray[PVLength] = {50,48,48,48,48,48,48,48}; // array holding Ascii value in decimal for each place value in Frequency between 19-21MHz
volatile unsigned int PVindex = 0; //value that will index through place value array
unsigned long Frequency = 20000000; // frequency value that corresponds to value on lcd screen
float frequencyWord = 20000000; // Word that is passed into function to achieve desired frequency

unsigned int ctrl       = 0x000006F3; //0000011011010011b
unsigned int N_INC      = 0x0001002; //0001000000000010b
unsigned int deltaf_lsb = 0x002000; //0010000000000000b
unsigned int deltaf_msb = 0x03000;
unsigned int t_INT      = 0x04002;
unsigned int Fstart_lsb = 0x0E666;
unsigned int Fstart_msb = 0x0F666;

void lcd_reset();
void lcd_pos(char pos);
void lcd_setup();
void lcd_data(unsigned char dat);
void lcd_display_top(char *line);
void encoderInit();
void switchInit();
void DDSinit();
void serial(unsigned int reg, int loopcount, int bcount);

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer

    //Initializes Systems
    encoderInit();
    lcd_setup();
    switchInit();
    DDSinit();

    // Sends an intial reading into DDS 20MHz is 66666666
    serial(ctrl, loopC, bcountC);
    serial(Fstart_lsb, loopC, bcountC);
    serial(Fstart_msb, loopC, bcountC);
    serial(deltaf_lsb, loopC, bcountC);
    serial(deltaf_lsb, loopC, bcountC);
    serial(N_INC, loopC, bcountC);
    // drives control pin high which tells DDS to begin output
    P1OUT |= CTRL;
    P1OUT &= (~CTRL); //set to 0

    // Never ending while loop so that system always is on unless powered down
    while(1)
    {

    	// updates frequency value
    	Frequency = ((PVarray[0]-48)*10000000) + ((PVarray[1]-48)*1000000) + ((PVarray[2]-48)*100000) + ((PVarray[3]-48)*10000) + ((PVarray[4]-48)*1000) + ((PVarray[5]-48)*100) + ((PVarray[6]-48)*10) + ((PVarray[7]-48));
    	// Updates DDS with new frequency

    	//Constantly Polling for Switch to be pressed. This method was chosen due to conflicts with LCD and port interupts
    	if(~P1IN & SW)
    	{
    		PVindex = PVindex+1;

    		if(PVindex > 7)
    		{
    			PVindex = 0;
    		}
    		IFlag = 1;
    		// The blocking while loop prevents bounce
    		while(~P1IN & SW){}


    	}


    	// Updates LCD display
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

        if(IFlag == 1)
        {
        	frequencyWord = (Frequency*.33554432);
        	//int lsb = frequency[23:1];
            serial(ctrl, loopC, bcountC);
            serial(Fstart_lsb, loopC, bcountC);
            serial(Fstart_msb, loopC, bcountC);
            P1OUT |= CTRL;
            P1OUT &= (~CTRL); //set to 0
            IFlag = 0;
        }
    }
}

//LCD
// resets lcd
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

//display number(int)
void lcd_data (unsigned char dat)
{
	P2OUT = (((dat >> 4) & 0x0F)|LCD_EN|LCD_RS);
	P2OUT = (((dat >> 4) & 0x0F)|LCD_RS);

	P2OUT = ((dat & 0x0F)|LCD_EN|LCD_RS);
	P2OUT = ((dat & 0x0F)|LCD_RS);

    __delay_cycles(400);
}

//displays strings
void lcd_display_top(char *line)
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
	//Sets internal pull up resistor
	P1OUT |= SW;
	P1REN |= SW;
}

void DDSinit()
{
	P1DIR |= SDATA+SCLK+FSYNC+CTRL; // sets as outputs
	P1OUT |= SCLK + FSYNC; // sets to 1
	P1OUT &= (~CTRL); //set to 0
}

void serial(unsigned int reg, int loopcount, int bcount){
    int i;
    P1OUT &= ~FSYNC; //Set low
    for (i = 0; i < loopcount ; i++) // Go through 16-bits
    {
        // Check if MSB is 1 or 0
        if (reg & bcount)
            P1OUT |= SDATA; //send 1
        else
            P1OUT &= ~SDATA; //send 0


        P1OUT |= SCLK;  //Pulse SCLK to high
        P1OUT &= ~SCLK; //Pulse SCLK to low; SDATA is shifted into DDS input shift register at falling edge of SCLK

        //shift register to left for new MSB
        reg = reg << 1;
    }
    P1OUT |= FSYNC;// End of transfer; set to high
}

//The ISR below will allow the PVarray (Frequency value) to be updated by the rotary encoder rotations
// The state machine below will keeps the value between 19 and 21MHz
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
	IFlag = 1;
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

