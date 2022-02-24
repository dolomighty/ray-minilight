
#include <assert.h>
#include <loop.h>
#include <math.h>
#include <SDL.h>    // HEADER
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <globals.h>

#include "Camera.h"
#include "Random.h"
#include "Scene.h"



static void makeRenderingObjects( const char *sModelFilePathname ){
    /* make random generator */
    pRandom = RandomCreate();
    /* create main rendering objects, from model file */
    pCamera = CameraCreate();
    pScene  = SceneConstruct( sModelFilePathname, &CameraEyePoint( pCamera ));
}



int main( int argc, char *argv[]){ 

    SDL_Window *win;
    assert( win = SDL_CreateWindow( argv[0] 
       , SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED
       , WW, WH,
//        SDL_WINDOW_FULLSCREEN_DESKTOP |
        0 ));

    assert( renderer = SDL_CreateRenderer( win, -1, 0 ));
    assert( framebuffer = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, W, H ));

    uint8_t t[]={
        #include "dyn/play.h" // SHELL python mk-sprite.py png/play.png
    };
    assert( play_icon = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, PLAY_W, PLAY_H ));
    SDL_UpdateTexture( play_icon, NULL, t, PLAY_W*4 );
    SDL_SetTextureBlendMode( play_icon, SDL_BLENDMODE_BLEND );
    play_icon_w = PLAY_W;
    play_icon_h = PLAY_H;

    makeRenderingObjects( argv[1]);

    loop();

    SceneDestruct((Scene*)pScene );

    SDL_DestroyRenderer( renderer );
    SDL_DestroyWindow( win );
    return 0 ;
}


