#include "GameEngine.h"
#include "Nokia5110.h"
#include "Sprites.h"
#include "ADC.h"
#include "Sound.h"
#include "SwitchLED.h"
#include "Random.h"

#define MAX_CACTUS 2
#define MAX_VULTURE 1
#define WIN_SCORE 100

unsigned long FrameCount=0; //to anmate enemies while they move
unsigned long Distance; // units 0.001 cm
unsigned long ADCdata;  // 12-bit 0 to 4095 sample
unsigned long Score; //game score
unsigned long Time;
unsigned char CactusDelay; //used to create a delay between spawning of cacti
unsigned char VultureDelay; //used to create a delay between spawning of vultures

struct GameObject {
	unsigned int x; // x coordinate
	unsigned int y; // y coordinate
  unsigned char active;  // 0= not on screen, greater than 0 = on screen
};
typedef struct GameObject GType;

struct PlayerSprite {
	GType GObj;
	const unsigned char *image[2]; // a single pointer to image
																		//no animation used
	int yspeed;
	int inAir; //is true if currently jumping, else is 0
	unsigned char explode; //explode if > 0
};
typedef struct PlayerSprite PType;

struct CactusSprite {
	GType GObj;
	const unsigned char *image; // two pointers to images
														
};
typedef struct CactusSprite CType;


struct VultureSprite {
	GType GObj;
	const unsigned char *image[2]; // a single pointer to image
	unsigned long xspeed;																	
};
typedef struct VultureSprite VType; 


PType Player;
CType Cacti[MAX_CACTUS];
VType Vultures[MAX_VULTURE];


// Convert a 12-bit binary ADC sample into a 32-bit unsigned
// fixed-point distance (resolution 0.001 cm).  Calibration
// data is gathered using known distances and reading the
// ADC value measured on PE1.  
// Input: sample  12-bit ADC sample
// Output: 32-bit distance (resolution 0.001cm)
unsigned long ConvertToDistance(unsigned long sample){
   // A is slope and B is intercept of calibration graph
	//For 3 cm long potentiometer with 10 kohm resistance:
	return ((750*sample) >> 10) + (429 >> 10); //A was found to be 0.7326, right shift by 10 is division by 1024, 750/1024 is approximately 0.7326
																	 //B was found to be 0.4188, right shift by 10 is division by 1024, 429/1024 is approximately 0.4188
																	//For 3cm long 10 kohm potentiometer, method returns a number betweem 0 and 3000
}

// Return a random number, the upper limit is defined in the parameter
// This generates a random value for VultureDelay and CactusDelay
// Input: upper limit
// Output: random number
unsigned long RandomDelayGenerator(unsigned long limit){
  return ((Random()>>22)%limit);  
}

// Return a random number, the upper limit is defined in the parameter
// This generates a height and speed for Vulture
// Input: upper limit
// Output: random number
unsigned long RandomGenerator(unsigned long limit){
  return (Random()%limit);  
}

void Game_Init(void){ 
	unsigned char i;
	Score = 0;
	Time = 0;
	Player.GObj.x = 32;
	Player.GObj.y = 47;
	Player.image[0] = PlayerDino0;
	Player.image[1] = PlayerDino1;
	Player.inAir = 0;
	Player.yspeed = 0;
	Player.GObj.active = 1;
	Player.explode = 0;
	Player.inAir = 0;

	VultureDelay = RandomDelayGenerator(80);
	for(i = 0; i < MAX_VULTURE; i++) {
		Vultures[i].image[0] = Vulture0;
		Vultures[i].image[1] = Vulture1;
		Vultures[i].GObj.active = 0;
	}
		
	CactusDelay = RandomDelayGenerator(60);
	for(i = 0; i < MAX_CACTUS; i++) {
		if(i%2 == 0) 
			Cacti[i].image = Cactus0;
		else
			Cacti[i].image = Cactus1;
		Cacti[i].GObj.active = 0;
	}
	
}	 

// unused utility functions for removing dead objects,
// A linked list would be better for large array, but for small sizes this is fine
void remove_element_cactus(CType *array, int index, int array_length)
{
   int i;
   for(i = index; i < array_length - 1; i++) array[i] = array[i + 1];
}

void remove_element_vulture(VType *array, int index, int array_length)
{
   int i;
   for(i = index; i < array_length - 1; i++) array[i] = array[i + 1];
}

// checks if player has collided with obstical
// Input: none
// Output: none
void CheckCollision(void){ 
	unsigned char j;
		if(Player.GObj.active){
			for(j = 0; j < MAX_CACTUS; j++){
					if((!(((Cacti[j].GObj.x + CACTUSW) < Player.GObj.x) || (Cacti[j].GObj.x > (Player.GObj.x + PLAYERW))) &&
						!((Cacti[j].GObj.y < (Player.GObj.y - PLAYERH)) || ((Cacti[j].GObj.y - CACTUSH) > Player.GObj.y))) && (Cacti[j].GObj.active > 0)){
							Player.GObj.active = 0;
							Player.explode = 1;
							Failure_LedOn(1000); // 1000 Timer2A periods approximately equal 0.9s
							Sound_Explosion();
							break;
					}
			}
			for(j = 0; j < MAX_VULTURE; j++){
					if((!(((Vultures[j].GObj.x+VULTUREW) < Player.GObj.x) || (Vultures[j].GObj.x > (Player.GObj.x + PLAYERW))) &&
						!((Vultures[j].GObj.y < (Player.GObj.y - PLAYERH)) || ((Vultures[j].GObj.y - VULTUREH) > Player.GObj.y))) && (Vultures[j].GObj.active > 0)){
					
							Player.GObj.active = 0;
							Player.explode = 1;
							Failure_LedOn(1000); // 1000 Timer2A periods approximately equal 0.9s
							Sound_Explosion();
							break;
					}
			}
		}
}

// causes player to jump
// Input: none
// Output: none
void Jump(void){
	if(Player.inAir == 0){
		Player.inAir = 1;
		Player.yspeed = 6;
	}
}

// Spawns a cactus on the right side of screen
// Input: none
// Output: none
void SpawnCactus(void) {
	int x;
	
	if(CactusDelay){
		CactusDelay--;
		return;
	}
	CactusDelay = RandomDelayGenerator(60);
	for(x = 0; x < MAX_CACTUS; x++) {
		if(!Cacti[x].GObj.active) {
			Cacti[x].GObj.x = 84-CACTUSW;
			Cacti[x].GObj.y = 47;
			Cacti[x].GObj.active = 1;
			return;
		}
	}
	return;
}

// Spawns a vulture at random height and speed
// Input: none
// Output: none
void SpawnVulture(void) {
	int x;
	
	if(VultureDelay){
		VultureDelay--;
		return;
	}
	VultureDelay = RandomDelayGenerator(80);
	for(x = 0; x < MAX_VULTURE; x++) {
		if(!Vultures[x].GObj.active) {
			Vultures[x].GObj.x = 84-VULTUREW;
			Vultures[x].GObj.y = 47 - RandomGenerator(15);
			Vultures[x].xspeed = 1 + RandomGenerator(5);
			Vultures[x].GObj.active = 1;
			return;
		}
	}
	return;
}

//Move player horizontally across the screen
//The x co-ordinate of the bottom left corner of the player can move from 0 to 64 on the x axis as the player is 18 pixels wide and the screen is 84 pixels
//The ConvertToDistance method in GameEngine.c returns a distance between 0 and 3 cm in units of 0.001 cm (i.e. a number between 0 and 3000
//This distance was matched with pixel location on x axis, for the bottom left corner of player, using raw data from the simulation debugging
//The relationship was found to be BottomLeftX = Distance*0.0229 - 1.8 via a linear fit to data
//0.0229 was approximated as 22/1024 to get a pixel location accurate to 1 pixel
void PlayerMove(void){
	Player.GObj.x = (ConvertToDistance(ADC0_In())*22) >> 10; 
	Player.GObj.y -= Player.yspeed;
	if(Player.inAir > 0) {
		Player.yspeed -= 1;
		if(Player.GObj.y >= 47) { 
			Player.inAir = 0;
			Player.GObj.y = 47;
			Player.yspeed = 0;
		}	
	}
}

//Move all cacti 2 pixels to the left
//if cactus reaches left edge of screen then it is deactivated
void CactusMove(void){ unsigned char i;
  for(i=0;i<MAX_CACTUS;i++){
    if(Cacti[i].GObj.x > 0){
      Cacti[i].GObj.x -= 2; 
    }else{
      Cacti[i].GObj.active = 0;
    }
  }
}

//Move all vultures by their speed to the left
//if vultures reaches left edge of screen then it is deactivated
void VultureMove(void){ unsigned char i;
  for(i=0;i<MAX_VULTURE;i++){
    if(Vultures[i].GObj.x > 0){
      Vultures[i].GObj.x -= Vultures[i].xspeed; 
    }else{
      Vultures[i].GObj.active = 0;
    }
  }
}


void Move_ActiveObjects(void){
	PlayerMove();
	CactusMove();
	VultureMove();
}


void DrawPlayer(void){
	if(Player.GObj.active){
		Nokia5110_PrintBMP(Player.GObj.x, Player.GObj.y, Player.image[FrameCount], 0);
	}
	else if(Player.explode){
		Player.explode = 0;
		Nokia5110_PrintBMP(Player.GObj.x, Player.GObj.y,  BigExplosion0, 0); 
	}
}


void DrawCacti(void){unsigned char i;
	for(i=0;i<MAX_CACTUS;i++){
		if(Cacti[i].GObj.active > 0){
			Nokia5110_PrintBMP(Cacti[i].GObj.x, Cacti[i].GObj.y, Cacti[i].image, 0);
		}
	}
}

void DrawVultures(void){unsigned char i;
	for(i=0;i<MAX_VULTURE;i++){
		if(Vultures[i].GObj.active > 0){
			Nokia5110_PrintBMP(Vultures[i].GObj.x, Vultures[i].GObj.y, Vultures[i].image[FrameCount], 0);
		}
	}
}

void UpdateScore(void) {
	Time++;
  FrameCount = (FrameCount+1)&0x01; 
	if(Time%50)
		Score++;
}


void Draw_GameFrame(void){
  Nokia5110_ClearBuffer();
	SpawnCactus();
	SpawnVulture();
	DrawPlayer();
	DrawCacti();
  DrawVultures();
	UpdateScore();
  Nokia5110_DisplayBuffer();      // draw buffer
}

//returns 1 if game over; 0 otherwise
unsigned char Check_GameOver(void){
	if((Score == WIN_SCORE) || (Player.GObj.active == 0))
		return 1;
	return 0;
}

// Outputes the game over screen
void State_GameOver(void){
	Nokia5110_Clear();
  Nokia5110_SetCursor(1, 0);
  Nokia5110_OutString("GAME OVER");
  Nokia5110_SetCursor(1, 1);
	if(Score == WIN_SCORE){
		Nokia5110_OutString("You Won!");
		Nokia5110_SetCursor(1, 2);
		Nokia5110_OutString("Good job!");
	}
	else{
		Nokia5110_OutString("You Lost!");
		Nokia5110_SetCursor(1, 2);
		Nokia5110_OutString("Nice try!");
	}
	Nokia5110_SetCursor(1, 4);
  Nokia5110_OutString("Score:");
  Nokia5110_SetCursor(7, 4);
  Nokia5110_OutUDec(Score);
}
