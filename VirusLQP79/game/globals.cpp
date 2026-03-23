#include "globals.h"

// globals ///////////////////////////////////////////////////////////////////

Arduboy2Base arduboy;
Sprites sprites;
ArduboyTones sound(arduboy.audio.enabled());
unsigned long scorePlayer;
byte gameID;
byte gameState  = STATE_MENU_INTRO;
byte gameType = STATE_GAME_NEW;
byte globalCounter = 0;

// function implementations //////////////////////////////////////////////////

// burp
// returns the value a given percent distance between start and goal
// percent is given in 4.4 fixed point
short burp(short start, short goal, unsigned char step)
{
    if (start == goal) return start;
    
    short diff = goal - start;
    // Плавное движение: двигаемся на 1/16 от разницы * step
    start += (diff * step) / 16;
    
    // Если очень близко, просто устанавливаем в goal
    if (abs(diff) < 2) return goal;
    
    return start;
}