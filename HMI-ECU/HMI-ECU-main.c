/*
 *  Module: HMI ECU
 *	HMI-ECU-main.c
 *	Description: HMI ECU for Door Lock System
 *  Created on: May 13, 2018
 *      Author: IAA
 *******************************************************************************/
#include "lcd.h"
#include "keypad.h"
#include "uart.h"
#include "String.h"

/*******************************************************************************
 *                      Preprocessor Macros                                    *
 *******************************************************************************/

#define M2_READY 0x50
#define NUMBER_OF_OVERFLOWS_PER_minute 240
#define NUMBER_OF_OVERFLOWS_PER_15seconds 60

/*******************************************************************************
 *                      Initializing Global Variables                                    *
 *******************************************************************************/

uint8 g_tick = 0;
uint8 g_timer = 0;
uint8 g_flag=0;


/*******************************************************************************
 *                      Functions Prototypes(Private)                          *
 *******************************************************************************/
void enter_password(uint8 *pass);
void Receive_from_control();
void Add_password();
void timer2_init_CTC_mode(unsigned char tick);


/*******************************************************************************
 *                      Main Function                                          *
 *******************************************************************************/

int main(void){

		//initializing local Variables
		uint8 key;
		uint8 compare;
		uint8 check=0;
		uint8 oldpass[6];

		//initializing modules
		LCD_init();
		LCD_clearScreen();
		UART_init();

		SREG |= (1<<7);

		timer2_init_CTC_mode(250);

loop:	//adding new password operation
		_delay_ms(10);
		LCD_clearScreen();

		Add_password();

		_delay_ms(10);
		LCD_clearScreen();

		compare = UART_recieveByte();
		if(compare==0){
			Receive_from_control();

menu:		//main menu
			_delay_ms(10);
			LCD_clearScreen();

			Receive_from_control();
			LCD_goToRowColumn(1,0);
			Receive_from_control();

		}
		else{
			//entered passwords not identical
			Receive_from_control();
			LCD_goToRowColumn(1,0);
			Receive_from_control();
			goto loop;
		}

loop2:	//choosing the option
		key = KeyPad_getPressedKey();
		if (key != '*' && key != '%')
		{		goto loop2;}
		else {
			_delay_ms(10);
			LCD_clearScreen();
		}

		UART_sendByte(key);	//sending the option

		if (key == '*')	//Change password option
		{

				check=0;
			check1:
				_delay_ms(10);
				LCD_clearScreen();

				check = UART_recieveByte();

				if (check == 3)
				{
					//Handling wrong password
					check=0;
					Receive_from_control();
					LCD_goToRowColumn(1,0);
					Receive_from_control();

					g_timer=1;	//timer for 1 minute
					g_tick=0;	//begin timing
					while(g_flag!=1){}	//wait for 1 minute
					goto menu;

				}else{

					//handling changing the password option
					check++;

					Receive_from_control();
					LCD_goToRowColumn(1,0);
					Receive_from_control();
					LCD_goToRowColumn(1,10);

					enter_password(oldpass);
					_delay_ms(10);
					UART_sendString(oldpass);

					//comparing the entered password with saved one
					compare = UART_recieveByte();
					if (compare==0)
					{
						goto loop;
					}else{
						goto check1;
					}
				}
		}

		if (key == '%')		//open door option
		{
				check=0;
			check2:
				_delay_ms(10);
				LCD_clearScreen();

				check = UART_recieveByte();

				if (check == 3)
				{
					//Handling Wrong password
					check=0;
					Receive_from_control();
					LCD_goToRowColumn(1,0);
					Receive_from_control();

					g_timer=1;	//timer for 1 minute
					g_tick=0;	//begin timing
					while(g_flag!=1){}	//wait for 1 minute
					goto menu;

				}else{

					//handling Door locking and unlocking
					check++;

					Receive_from_control();
					LCD_goToRowColumn(1,0);
					Receive_from_control();
					LCD_goToRowColumn(1,10);

					enter_password(oldpass);
					_delay_ms(10);
					UART_sendString(oldpass);

					//comparing the entered password with saved one
					compare = UART_recieveByte();
					if (compare==0)
					{
						//Locking the door
						_delay_ms(10);
						LCD_clearScreen();
						Receive_from_control();

						g_timer=2;	//timer for 15 seconds
						g_tick=0;	//begin timing
						while(g_flag!=2){}	//wait for 15 seconds

						LCD_clearScreen();

						//UnLocking the door
						Receive_from_control();

						g_timer=2;	//timer for 15 seconds
						g_tick=0;	//begin timing
						g_flag=0;
						while(g_flag!=2){}	//wait for 15 seconds

						//Motor off
						g_timer=0;
						g_tick=0;	//begin timing
						g_flag=0;
						goto menu;

					}else{
						goto check2;
					}
				}
		}



		while(1){


		}
}

/*******************************************************************************
 *                      Functions Definitions                                  *
 *******************************************************************************/



void enter_password(uint8 *pass)
{
	/*
	 * Function responsible for getting entered password in the keypad
	 */

	uint8 i = 0;
	uint8 key;
	while(i!=5){

		key = KeyPad_getPressedKey();	//get pressed key
		pass[i]=key;	//saving pressed key to password
		LCD_displayCharacter('*');	//display unseen key

		_delay_ms(50);
		i++;
	}

	pass[i]='#';
}

void Receive_from_control()
{
	/*
	 * Function responsible for receiving messages from the Control ECU
	 */

	uint8 string[17];
	UART_sendByte(M2_READY);	//HandShaking
	UART_receiveString(string);	//receive text from control ECU
	LCD_displayString(string);	//Display text received
	_delay_ms(10);

}
void Add_password()
{
	/*
	* Function responsible for adding the new password
	*/
	uint8 pass[6];

	//adding first password text to LCD
	Receive_from_control();
	LCD_goToRowColumn(1,0);

	Receive_from_control();
	LCD_goToRowColumn(1,10);

	//sending first password to control ECU
	enter_password(pass);
	_delay_ms(10);
	UART_sendString(pass);

	LCD_clearScreen();
	_delay_ms(1);

	//adding second password text to LCD
	Receive_from_control();
	LCD_goToRowColumn(1,0);

	Receive_from_control();
	LCD_goToRowColumn(1,10);

	//sending second password to control ECU
	enter_password(pass);
	_delay_ms(10);
	UART_sendString(pass);

}


ISR(TIMER2_COMP_vect)
{
	/*
	 * Function responsible for timer interrupt
	 */

	g_tick++;
	if (g_timer == 1){
		if (g_tick==NUMBER_OF_OVERFLOWS_PER_minute)	//timer for 1 minute
		{
			g_timer=0;
			g_flag=1;	//1 minute flag
			g_tick = 0;
		}
	}
	else if (g_timer==2)
	{
		if (g_tick==NUMBER_OF_OVERFLOWS_PER_15seconds)	//timer for 15 seconds
		{
			g_timer=0;
			g_flag=2;	//15 seconds flag
			g_tick = 0;
		}

	}
}

void timer2_init_CTC_mode(unsigned char tick)
{
	/*
	 * Function responsible for initializing the timer
	 */

	TCNT2 = 0; //timer initial value
	OCR2  = tick; //compare value
	TIMSK |= (1<<OCIE2); //enable compare interrupt
	/* Configure timer0 control register
	 * 1. Non PWM mode FOC0=1
	 * 2. CTC Mode WGM01=1 & WGM00=0
	 * 3. No need for OC0 in this example so COM00=0 & COM01=0
	 * 4. clock = F_CPU/1024 CS00=1 CS01=0 CS02=1
	 */
	TCCR2 = (1<<FOC2) | (1<<WGM21) | (1<<CS22) | (1<<CS21) | (1<<CS20);
}



