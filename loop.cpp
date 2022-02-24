

#include "M34.h"
#include <SDL.h>
#include <unistd.h>
#include <frame.h>
#include <last.h>
#include <globals.h>




void loop(){    // HEADER

    static char mouselook = 0;
    static char key_fwd   = 0;
    static char key_back  = 0;
    static char key_left  = 0;
    static char key_right = 0;
    static char key_up    = 0;
    static char key_down  = 0;

    last_load();

    while(1){

        static Uint32 ticks_now = 0;
        static Uint32 ticks_pre = 0;
        ticks_pre = ticks_now;

        ticks_now = SDL_GetTicks();
        static Uint32 look_renorm_deadline = ticks_now+3000;
        if( look_renorm_deadline < ticks_now ){
            look_renorm_deadline = ticks_now+3000;
//            camera.normalize();
//            fprintf( stderr, "camera.normalize()\n" );
        }



        bool camera_touched = false;


        SDL_Event event;
        while( SDL_PollEvent( &event )){
            switch( event.type ){
                case SDL_MOUSEWHEEL:
//                    fprintf( stderr, "SDL_MOUSEWHEEL %d %d\n", event.wheel.x, event.wheel.y );
                    {
                        float expo_incr = (expo+1)*(-event.wheel.y)/100.0;
                        expo += expo_incr;
                        if(expo<0)expo=0;
                    }
                    break;

                case SDL_KEYDOWN:
//                    // https://wiki.libsdl.org/SDL_Keycode
//                    fprintf( stderr, "SDL_KEYDOWN %s\n",SDL_GetKeyName(event.key.keysym.sym));
                    switch(event.key.keysym.sym){
                        case SDLK_1: speed_mult = SPEED_MULT_1; break;
                        case SDLK_2: speed_mult = SPEED_MULT_2; break;
                        case SDLK_3: speed_mult = SPEED_MULT_3; break;
                    }
//                    // ---- no break intenzionale ----

                case SDL_KEYUP:
//                    // https://wiki.libsdl.org/SDL_Keycode
//                    fprintf( stderr, "SDL_KEYUP %s\n",SDL_GetKeyName(event.key.keysym.sym));
                    switch(event.key.keysym.sym){
                        case SDLK_w: key_fwd   = event.key.state; break;
                        case SDLK_s: key_back  = event.key.state; break;
                        case SDLK_a: key_left  = event.key.state; break;
                        case SDLK_d: key_right = event.key.state; break;
                        case SDLK_e: key_up    = event.key.state; break;
                        case SDLK_c: key_down  = event.key.state; break;
                    }
                    break;

                case SDL_MOUSEMOTION:
//                    fprintf( stderr, 
//                        "SDL_MOUSEMOTION %d,%d pos %d,%d\n"
//                        , event.motion.xrel
//                        , event.motion.yrel
//                        , event.motion.x
//                        , event.motion.y );
                    if(!mouselook)break;
                    // controlli pitchyaw incrementali
                    camera.rotate_x( -event.motion.yrel*0.001 );
                    camera.rotate_y( +event.motion.xrel*0.001 );
                    camera_touched=true;
                    break;

                case SDL_MOUSEBUTTONDOWN:
//                    fprintf( stderr,
//                        "SDL_MOUSEBUTTONDOWN %d (%d,%d)\n"
//                        , event.button.button
//                        , event.button.x
//                        , event.button.y
//                        );
                    mouselook=1;
                    SDL_SetRelativeMouseMode(SDL_TRUE);
                    break;

                case SDL_MOUSEBUTTONUP:
//                    fprintf( stderr,
//                    "SDL_MOUSEBUTTONUP %d (%d,%d)\n"
//                    , event.button.button
//                    , event.button.x
//                    , event.button.y
//                    );
                    mouselook=0;
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                    break;

                case SDL_QUIT:
//                    fprintf( stderr, "SDL_QUIT\n" );
                    last_save();
                    return;
            }
        }


        // rettifica camera
        // il metodo di navigazione non assicura che l'asse x sia orizzontale (no twist)
        // qui ricalcoliamo gli assi x e y
        // X = Z × UP
        // Y = X × Z
        auto z = V3f( camera.z.x, camera.z.y, camera.z.z );
        auto x = (z % V3f(0,1,0)).norm();
        camera.x.x = x.X(); camera.x.y = x.Y(); camera.x.z = x.Z();
        auto y = x % z; // non serve normalizzare, x e z sono unitari e perpendicolari
        camera.y.x = y.X(); camera.y.y = y.Y(); camera.y.z = y.Z();


        float speed_scale = (ticks_now - ticks_pre)*speed_mult/1000.0;

        char move_x = 0;
        if(key_right == SDL_PRESSED) move_x ++;
        if(key_left  == SDL_PRESSED) move_x --;

        char move_y = 0;
        if(key_up   == SDL_PRESSED) move_y ++;
        if(key_down == SDL_PRESSED) move_y --;

        char move_z = 0;
        if(key_fwd  == SDL_PRESSED) move_z ++;
        if(key_back == SDL_PRESSED) move_z --;

        if( move_x || move_y || move_z ){
            camera.translate( move_x*speed_scale , move_y*speed_scale , move_z*speed_scale );
            camera_touched=true;
        }



//        camera_touched = true;  // fissa 1 raggio per pixel

        if( camera_touched ){
            // se cambia la visuale, resettiamo il buffer hdr
            samples=0;
            hdr_zero();
        }
        samples++;

        frame();
        usleep( 1000 );
    }
}    

