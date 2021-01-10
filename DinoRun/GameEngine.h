//Prototype for functions dealing with game mechanics such as movement, collision etc.

 //Initialize game and players
void Game_Init(void);

//Causes the player dinosaur to jump into the air
//this allows the dinosaur to avoid obstacles
void Jump(void);

//Move all the alive objects in the game according to each objects behavior
void Move_ActiveObjects(void);

//Detect if player has collided in current frame and respond appropiatly
void CheckCollision(void);

//Draw the current game state 
void Draw_GameFrame(void); 

//returns 1 if game over; 0 otherwise
unsigned char Check_GameOver(void);

//Output the frame for the Game Over State 
void State_GameOver(void);
