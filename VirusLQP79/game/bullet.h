#ifndef BULLET_H
#define BULLET_H

#include "enemies.h"
#include "level.h"

// constants /////////////////////////////////////////////////////////////////

#define BULLET_MAX        6
#define BULLET_WIDTH      2
#define BULLET_HEIGHT     2
#define BULLET_DIRECTIONS 8

#include <stdint.h>
extern const int8_t BulletXVelocities[8];


// structures ////////////////////////////////////////////////////////////////

struct Bullet {
  int x;
  int y;
  int8_t vx;
  int8_t vy;
  byte active;
};

// globals ///////////////////////////////////////////////////////////////////

extern Bullet bullets[BULLET_MAX];

// method prototypes /////////////////////////////////////////////////////////

bool setBullet(Bullet& obj, int x, int y, int8_t vx, int8_t vy);
void addBullet(int x, int y, byte direction, int8_t vx, int8_t vy);
void updateBullet(Bullet& obj);
void updateBullets();
void drawBullets();
void clearBullets();

#endif
