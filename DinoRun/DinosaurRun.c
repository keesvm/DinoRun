// DinosaurRun.c
// Runs on LM4F120/TM4C123
// This project can run on either a simulated Nokia5110 project or on a real board.
// The Nokia5110 is 48x84 black and white
// pixel LCD to display text, images, or other information.

// ***** Pre-processor Directives Section *****
#include "tm4c123gh6pm.h"
#include "Nokia5110.h"
#include "SwitchLED.h"
#include "Sound.h"
#include "Random.h"
#include "ADC.h"
#include "GameEngine.h"
#include "TExaS.h"


void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Timer2_Init(void(*task)(void), unsigned long period);
void Delay100ms(unsigned long count); // time delay in 0.1 seconds
void PF1Init(void); //Initialize PF1 (PF1) for debugging the SysTick interrupts
void SwitchLed_Init(void);// Initialize switch inputs and LED outputs
void SysTick_Init(unsigned long period); // Initialize SysTick interrupts
void (*PeriodicTask)(void);   // user function for Timer2A (comes from Sound.c and outputs to 4 bit DAC)


//***** Global Declarations Section *****

//Global variables
unsigned char GameOverFlag;
unsigned char Semaphore = 0;

// ***** Subroutines Section *****

int main(void){ 
DisableInterrupts();
  TExaS_Init(SSI0_Real_Nokia5110_Scope);  // set system clock to 80 MHz
	Random_Init(1);
  Nokia5110_Init();
	PF1Init();
  SysTick_Init(2666666); //Initialize SysTick with 30 Hz interrupts
	//SysTick_Init(2666666*4); //Increased period by 4 for actual hardware to make the game run at a playable speed
  Nokia5110_ClearBuffer();
	Nokia5110_DisplayBuffer();      // draw buffer
	ADC0_Init();
	Game_Init();
	SwitchLed_Init();
	Sound_Init();
	Timer2_Init(&Sound_Play,7256); //11.025 kHz. 80,000,000/11,025 cycles, which is about 7256
	GameOverFlag = 0;
	EnableInterrupts();
	
  while(1){
		while(Semaphore==0){};
    Semaphore = 0;
		if(GameOverFlag){
			State_GameOver();
		}
		else{
			Draw_GameFrame(); // update the LCD
		}	
		if((GameOverFlag == 0) && (Check_GameOver())){ //just detected game over
			Delay100ms(2);//Delay 200ms
			GameOverFlag = Check_GameOver();
			SysTick_Init(2666666);//Re-initialize with 30 Hz interrupt
			//SysTick_Init(2666666*4); //Increased period by 4 for actual hardware to make the game run at a playable speed
		}
	}
}
	
void PF1Init(void) {
  unsigned long volatile delay;
	SYSCTL_RCGC2_R |= 0x00000020;     // 1) activate clock for Port F
  delay = SYSCTL_RCGC2_R;           // allow time for clock to start
  GPIO_PORTF_AMSEL_R &= ~0x02;        // 3) disable analog on PF1
  GPIO_PORTF_PCTL_R &= ~0x000000F0;   // 4) PCTL GPIO on PF1
  GPIO_PORTF_DIR_R |= 0x02;          // 5) PF1 out
  GPIO_PORTF_AFSEL_R &= ~0x02;        // 6) disable alt funct on PF7-0
  GPIO_PORTF_DEN_R |= 0x02;          // 7) enable digital I/O on PF1
}


void SysTick_Init(unsigned long period){
	NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_RELOAD_R = period-1;// reload value
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x20000000; // priority 1      
  NVIC_ST_CTRL_R = 0x0007;  // enable SysTick with core clock and interrupts
}

void SysTick_Handler(void) {
	GPIO_PORTF_DATA_R ^= 0x02; // toggle PF1
	if(GameOverFlag) {
			if(Switch_Jump()){
				GameOverFlag = 0;
				Game_Init();
			}
	} else {
		CheckCollision();
		Move_ActiveObjects();
		if(Switch_Jump()){
			Jump();
			Sound_Jump();
		}
	}
 Semaphore = 1;
}


void Timer2_Init(void(*task)(void), unsigned long period){
  unsigned long volatile delay;
  SYSCTL_RCGCTIMER_R |= 0x04;   // 0) activate timer2
  delay = SYSCTL_RCGCTIMER_R;
  PeriodicTask = task;          // user function
  TIMER2_CTL_R = 0x00000000;    // 1) disable timer2A during setup
  TIMER2_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER2_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER2_TAILR_R = period-1;    // 4) reload value
  TIMER2_TAPR_R = 0;            // 5) bus clock resolution
  TIMER2_ICR_R = 0x00000001;    // 6) clear timer2A timeout flag
  TIMER2_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0x00FFFFFF)|0x80000000; // 8) priority 4
// vector number 39, interrupt number 23
  NVIC_EN0_R = 1<<23;           // 9) enable IRQ 23 in NVIC
  TIMER2_CTL_R = 0x00000001;    // 10) enable timer2A
}


void Timer2A_Handler(void){ 
	unsigned long checkFailureLed = Failure_LedCount();
  TIMER2_ICR_R = 0x00000001;   // acknowledge timer2A timeout
	if(!checkFailureLed)//FailureLedCount = 0
		Failure_LedOff();
  (*PeriodicTask)();                // execute user task
}
void Delay100ms(unsigned long count){unsigned long volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}
