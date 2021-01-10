
#include "tm4c123gh6pm.h"
#include "SwitchLED.h"

unsigned long PrevJump = 0;
unsigned long PrevSpecFire = 0;
unsigned long SuccessLedCount = 0;
unsigned long FailureLedCount = 0;

// Initialize switch inputs and LED outputs
// Input: none
// Output: none
void SwitchLed_Init(void){ 
  volatile unsigned long  delay;
	
		//Clock for Port E already activated in ADC_Init which is called before this function in main

	GPIO_PORTE_AMSEL_R &= ~0x01; // 3) disable analog function on PE0
  GPIO_PORTE_PCTL_R &= ~0x0000000F; // 4) enable regular GPIO on PE0
  GPIO_PORTE_DIR_R &= ~0x01;   // 5) inputs on PE0
  GPIO_PORTE_AFSEL_R &= ~0x01; // 6) regular function on PE0
  GPIO_PORTE_DEN_R |= 0x01;    // 7) enable digital on PE0
	
	
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB; // activate port B
  delay = SYSCTL_RCGC2_R;    // allow time to finish activating
  GPIO_PORTB_AMSEL_R &= ~0x10;      // no analog function for PB4
  GPIO_PORTB_PCTL_R &= ~0x000F0000; // regular function for PB4
  GPIO_PORTB_DIR_R |= 0x10;      // make PB4 in
  GPIO_PORTB_AFSEL_R &= ~0x10;   // disable alt funct on PB4
  GPIO_PORTB_DEN_R |= 0x10;      // enable digital I/O on PB4
}

// Input from jump button (PE0)
// Input: none 
// Output: 0 or 1 depending on whether button was just pressed (positive logic)
unsigned char Switch_Jump(void){
	 unsigned char JumpBool;
   if((GPIO_PORTE_DATA_R&0x01) && (PrevJump == 0)){ // just pressed
		 JumpBool = 1;
	 }
	 else{
			JumpBool = 0;
	 }
	 PrevJump = GPIO_PORTE_DATA_R&0x01;
	 return JumpBool;
}




//Turns on Led connected to PB4 when player is hit and game is lost;
//Initializes a value for the FailureLedCount variable which can be used to determine how many interrupt periods (like Timer2A perids the LED  should stay on
void Failure_LedOn(unsigned long count){
	GPIO_PORTB_DATA_R |= 0x10;
	FailureLedCount = count;
}

//Decrements the value of the FailureLedCount variable and then returns this value
unsigned long Failure_LedCount(void){
	if(FailureLedCount)
			FailureLedCount--;
	
	return FailureLedCount;
}


//Turns off Led connected to PB4
void Failure_LedOff(void){
	GPIO_PORTB_DATA_R &= ~0x10;
	
}