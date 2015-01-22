#include <msp430.h> 

/*
 * main.c
 */
#define READY 0
#define ARMED 1

short int PeriodCnt;
short int DiscStatus = READY;
short int Test = 0;
short int WatchdogIntervalCnt = 0;
short int Sound = 1;
short int LEDPin[4] =  {0x01, 0x02, 0x08, 0x20};
short int DelayCount;
short int OnCounts = 10000;
short int OffCounts = 20000;
short int DiscFlashes;
void PiezoInput(void);
void PiezoOutput(void);
void Beep(short int);

int main(void){
	unsigned short int i;
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer


    while(1){

    	PiezoInput();
		__bis_SR_register(LPM4_bits + GIE);       // Enter LPM4 w/interrupt
		//when piezo tap is detected diskeep gives a ready beep
		Beep(500);
		Beep(450);
		Beep(400);

		//Disckeeper is now armed, do the flashy thing
		DiscFlashes = 40;
		//For now leave controller running.  This can be made more energy conservitive later with timers and sleep
		P1DIR |= 0x2B;
		while(DiscFlashes){
			for(i = 0; i < 4; i++){
				P1OUT = LEDPin[i];
				for(DelayCount = 0; DelayCount < OnCounts; DelayCount++);
				P1OUT = 0x00;
				for(DelayCount = 0; DelayCount < OffCounts; DelayCount++);
			}
			DiscFlashes--;
		}


/*  This is old code for non LED diskeepers
		//delay for player to throw
		//set Watchdog for max interval
		BCSCTL1 |= DIVA_3;	//ACLK/8

		WDTCTL = WDT_ADLY_1000;
		while(WatchdogIntervalCnt < 2){
			__bis_SR_register(LPM3_bits + GIE);
			WatchdogIntervalCnt++;
		}
		WatchdogIntervalCnt = 0;

*/

		while(DiscStatus){


			Beep(450);
			Beep(500);
			Beep(450);
			PiezoInput();
			WDTCTL = WDT_ADLY_250;
			__bis_SR_register(LPM3_bits + GIE);


		}
		Beep(550);
		Beep(500);
		Beep(450);
	}
	
	return 0;
}
//During testing using DIP20 pins 4 and 6, P1.2 and P1.4.  2 is pulldown 4 is input
void PiezoInput(void){
	P1DIR |= 0x04;	//P1.2 is output
	P1DIR &= ~0x10;	//P1.4 is input
	P1SEL &= ~0x14;	//deselect pwm as pin function
	P1OUT |= ~0x04;	//P1.2 is low
	P1REN |= 0x04;	//P1.2 Pull down resistor
	P1IFG &= ~0x10; //clear any latent interrupt flag on P1.4
	P1IE |= 0x10;	//enable interrupt P1.4
	P1IES &= ~0x10;	// P1.4 lo/hi edge

}

#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void){
	if(DiscStatus == ARMED){
		__bic_SR_register_on_exit(LPM3_bits + GIE);
	}
	else{
		__bic_SR_register_on_exit(LPM4_bits);
	}
		DiscStatus ^= 0x01;
		P1IFG &= ~0x10;
}
/*
 * Short beep, length of WDT
 */
void Beep(short int CountValue){
	PiezoOutput();
	//set timer to output complimentary PWM
	TACCR0 = CountValue;
	TACCR1 = CountValue/2;
	TACCR2 = CountValue/2;
	TACTL = TASSEL_2 +  MC_3;
	TACCTL1 = OUTMOD_4;
	TACCTL2 = OUTMOD_4;

	Sound = 1;
	//set WDT to time the beep
	BCSCTL1 |= DIVA_1;                        // ACLK/2
	BCSCTL3 |= LFXT1S_2;                      // ACLK = VLO
	WDTCTL = WDT_ADLY_16;                   // Interval timer
	IFG1 &= ~0x01;							//clear WDT Interrupt
	IE1 |= WDTIE;                             // Enable WDT interrupt
	//take a nap while sounding tone indicating device armed
	__bis_SR_register(LPM0_bits + GIE);
	TACTL = TASSEL_2 +  MC_0;	//halt timer
	Sound = 0;
	//make sure PWM outputs are low power




}

void PiezoOutput(void){
	P1IE &= ~0x10;	//disable interrupt P1.4
	P1DIR |= 0x14;	//P1.2 and P1.4 are output
	P1REN &= ~0x14;	//Disable pull ups.  Push-Pull Operation
	P1SEL |= 0x14;	//P1.2 and P1.4 are Compare 1 and 2 output

}

#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer (void){
	if(Sound){
		__bic_SR_register_on_exit(LPM0_bits);     // Clear LPM0 bits from 0(SR)
	}
	else{
		__bic_SR_register_on_exit(LPM3_bits);
	}
}
