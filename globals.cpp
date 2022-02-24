

// HEADERBEG

#include "M34.h"
#include <SDL.h>
#include <Camera.h>
#include <Random.h>
#include <Scene.h>
#include <stdint.h>

#define W         320
#define H         180
#define PIXELATE  3

#define WW (W*PIXELATE)
#define WH (H*PIXELATE)

#define COUNT(ARRAY) (sizeof(ARRAY)/sizeof(ARRAY[0]))

#define SPEED_MULT_1 1
#define SPEED_MULT_2 10
#define SPEED_MULT_3 100

// HEADEREND


M34   camera;     // HEADER
float expo;       // HEADER
int   samples;    // HEADER
int   speed_mult; // HEADER

SDL_Renderer *renderer;    // HEADER
SDL_Texture  *framebuffer; // HEADER
SDL_Texture  *play_icon;  // HEADER
int play_icon_w;  // HEADER
int play_icon_h;  // HEADER

Camera *pCamera; // HEADER
Scene  *pScene;  // HEADER
Random *pRandom; // HEADER



