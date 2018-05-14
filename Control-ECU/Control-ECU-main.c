/*
 *	Module: Control ECU
 *	Control-ECU-main.c
 *	Description: Control ECU for Door Lock System
 *  Created on: May 13, 2018
 *      Author: IAA
 *******************************************************************************/
#include "String.h"
#include "i2c.h"
#include "external_eeprom.h"
#include "uart.h"

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

void Send_to_HMI(const uint8 *str);
uint8 Add_password(uint8 *pass1);
void PWM_Timer0_Init(unsigned char set_duty_cycle);
void timer2_init_CTC_mode(unsigned char tick);



/*******************************************************************************
 *                      Main Function                                          *
 *******************************************************************************/

int main(void){

		//Initializing local Variables
		uint8 compare=1;
		uint8 key;
		uint8 pass1[6];
		uint8 oldpass[6];
		uint8 check=0;
		uint8 eeprom_pass_location=0x0001;

		//initializing modules
		EEPROM_init();
		UART_init();

		SET_BIT(DDRA,PA0);
		SET_BIT(DDRB,PB5);
		SET_BIT(DDRB,PB4);

		PWM_Timer0_Init(128);
		SREG  |= (1<<7);
		timer2_init_CTC_mode(250);


loop:	//adding new password operation
		compare = Add_password(pass1);

		UART_sendByte(compare);

		if (compare == 0){
			Send_to_HMI("Success#");
			//Saving password in EEPROM
			for (uint8 i =0; i<6; i++){
				EEPROM_writeByte(eeprom_pass_location, pass1[i]);
				eeprom_pass_location++;
			}
			eeprom_pass_location=0x0001;

menu:	//main menu
		Send_to_HMI("*: Change Pass#");
		Send_to_HMI("%: Open Door#");

		}
		else{
			//entered passwords not identical
			Send_to_HMI("Fail#");
			Send_to_HMI("Repeat again#");
			goto loop;
		}

		key = UART_recieveByte();	//receiving the option
		compare=1;

		if (key == '*') //Change password option
		{
				check=0;
			check1:
				UART_sendByte(check);
				if (check==3){
					//Handling wrong password
					check=0;
					SET_BIT(PORTA,PA0);
					Send_to_HMI("Wrong password!#");
					Send_to_HMI("Wait a minute :)#");

					g_timer=1;	//timer for 1 minute
					g_tick=0;	//begin timing
					while(g_flag!=1){}	//wait for 1 minute
					goto menu;

				}else{
					//handling changing the password option
					Send_to_HMI("Please enter old#");
					Send_to_HMI("password: #");
					UART_receiveString(oldpass);
					_delay_ms(10);

					//reading password in EEPROM
					for (uint8 i=0; i<6; i++){
						EEPROM_readByte(eeprom_pass_location,&pass1[i]);
						eeprom_pass_location++;
					}
					eeprom_pass_location=0x0001;

					//comparing the entered password with saved one
					compare = strcmp(pass1,oldpass);
					UART_sendByte(compare);

					check++;

					if (compare==0){
						goto loop;
					}else{
						goto check1;
					}
				}
		}

		compare=1;

		if (key == '%')		//open door option
		{
				check=0;
			check2:
				UART_sendByte(check);
				if (check==3){
					//Handling Wrong password
					check=0;
					SET_BIT(PORTA,PA0);		//enabling the buzzer
					Send_to_HMI("Wrong password!#");
					Send_to_HMI("Wait a minute#");

					g_timer=1;	//timer for 1 minute
					g_tick=0;	//begin timing
					while(g_flag!=1){}	//wait for 1 minute
					goto menu;

				}else{
					//handling Door locking and unlocking
					Send_to_HMI("Please enter the#");
					Send_to_HMI("password: #");
					UART_receiveString(oldpass);
					_delay_ms(10);

					//reading password in EEPROM
					for (uint8 i=0; i<6; i++){
						EEPROM_readByte(eeprom_pass_location,&pass1[i]);
						eeprom_pass_location++;
					}
					eeprom_pass_location=0x0001;

					//comparing the entered password with saved one
					compare = strcmp(pass1,oldpass);
					UART_sendByte(compare);

					check++;

					if (compare==0){

						//Locking the door
						SET_BIT(PORTB,PB5);
						CLEAR_BIT(PORTB,PB4);
						Send_to_HMI("Door Locking#");

						g_timer=2;	//timer for 15 seconds
						g_tick=0;	//begin timing
						while(g_flag!=2){}	//wait for 15 seconds

						CLEAR_BIT(PORTB,PB5);
						CLEAR_BIT(PORTB,PB4);

						//UnLocking the door
						SET_BIT(PORTB,PB4);
						CLEAR_BIT(PORTB,PB5);

						Send_to_HMI("Door unLocking#");
						g_timer=2;	//timer for 15 seconds
						g_tick=0;	//begin timing
						g_flag=0;
						while(g_flag!=2){}	//wait for 15 seconds

						//Motor off
						g_timer=0;
						g_tick=0;	//begin timing
						g_flag=0;

						CLEAR_BIT(PORTB,PB5);
						CLEAR_BIT(PORTB,PB4);
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


void Send_to_HMI(const uint8 *str)
{
	/*
	 * Function responsible for sending messages to the HMI ECU
	 */

	while(UART_recieveByte() != M2_READY){}	//HandShaking

	UART_sendString(str);	//Send text to HMI ECU
	_delay_ms(10);

}

uint8 Add_password(uint8 *pass1)
{
	/*
	 * Function responsible for adding the new password
	 */

	uint8 pass2[6];
	uint8 compare=1;

	//adding first password text to LCD
	Send_to_HMI("Please enter new#");

	Send_to_HMI("password: #");

	//receiving first password entered by user from HMI ECU
	UART_receiveString(pass1);
	_delay_ms(10);

	//adding second password text to LCD
	Send_to_HMI("Please re-enter #");

	Send_to_HMI("password: #");

	//receiving second password entered by user from HMI ECU
	UART_receiveString(pass2);
	_delay_ms(10);

	//compare entered passwords
	compare = strcmp(pass1,pass2);

return compare;
}

void PWM_Timer0_Init(unsigned char set_duty_cycle)
{
	/*
	 * Function responsible for making a PWM on OC0
	 */


	TCNT0 = 0; //initial timer value

	OCR0  = set_duty_cycle;
	SET_BIT(DDRB,PB3);

	/* Configure timer control register
	 * 1. Fast PWM mode FOC0=0
	 * 2. Fast PWM Mode WGM01=1 & WGM00=1
	 * 3. Clear OC0 when match occurs (non inverted mode) COM00=0 & COM01=1
	 * 4. clock = F_CPU/8 CS00=0 CS01=1 CS02=0
	 */
	TCCR0 = (1<<WGM00) | (1<<WGM01) | (1<<COM01) | (1<<CS01);
}

ISR(TIMER2_COMP_vect)
{
	/*
	 * Function responsible for timer interrupt
	 */

	g_tick++;
	if (g_timer == 1){
		if (g_tick==NUMBER_OF_OVERFLOWS_PER_minute) //timer for 1 minute
		{
			g_timer=0;
			CLEAR_BIT(PORTA,PA0);	//disabling the buzzer
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

