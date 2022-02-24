

#include <SDL.h>
#include "M34.h"
#include <main.h>
#include <loop.h>
#include <globals.h>







void frame(){   // HEADER

    pCamera->viewPosition  = V3f( camera.t.x, camera.t.y, camera.t.z );
    pCamera->viewDirection = V3f( camera.z.x, camera.z.y, camera.z.z );
    pCamera->right         = V3f( camera.x.x, camera.x.y, camera.x.z );
    pCamera->up            = V3f( camera.y.x, camera.y.y, camera.y.z );

//    CameraPrint(pCamera);
//    exit(1);

    CameraFrame( pCamera, pScene, pRandom );
//    hdr_to_sdl( expo / samples );
    hdr_to_sdl();

    // progress bar
    SDL_SetRenderDrawColor( renderer, 255, 255, 255, 0 );
    SDL_Rect r;
    r.h = 2;
    r.w = samples;
    r.x = 0;
    r.y = WH-r.h;
    SDL_RenderFillRect( renderer, &r );

    // speed icon
    r.w = play_icon_w;
    r.h = play_icon_h;
    r.x = 5;
    r.y = 5;
    SDL_RenderCopy( renderer, play_icon, NULL, &r );
    if( speed_mult >= SPEED_MULT_2 ){ r.x+=5; SDL_RenderCopy( renderer, play_icon, NULL, &r ); }
    if( speed_mult >= SPEED_MULT_3 ){ r.x+=5; SDL_RenderCopy( renderer, play_icon, NULL, &r ); }

    SDL_RenderPresent( renderer );
}

