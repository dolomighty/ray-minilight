
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <globals.h>
#include <V3f.h>  // HEADER 


typedef V3f HDR_PIXMAP[H][W];

HDR_PIXMAP HDR;


/* constants ---------------------------------------------------------------- */

/* guess of average screen maximum brightness */
static const float DISPLAY_LUMINANCE_MAX = 200.0;

/* ITU-R BT.709 standard RGB luminance weighting */
static const V3f RGB_LUMINANCE( 0.2126, 0.7152, 0.0722 );

/* ITU-R BT.709 standard gamma */
static const float GAMMA_ENCODE = 0.45;     // = 1/2.2, srgb gamma


/**
 * Calculate tone-mapping scaling factor.
 */
static float calculateToneMapping( const float divider )
{
    /* calculate estimate of world-adaptation luminance
        as log mean luminance of scene */

    const V3f scale = RGB_LUMINANCE * divider;

    float adaptLuminance = 1e-4;
    {
        float sumOfLogs = 0.0;
        for( int y=0;  y<H; ++y )
        {
            for( int x=0;  x<W; ++x )
            {
                const float Y = HDR[y][x].dot( scale );
                /* clamp luminance to a perceptual minimum */
                sumOfLogs += log10( Y > 1e-4 ? Y : 1e-4 );
            }

            adaptLuminance = pow( 10.0, sumOfLogs / (W*H));
        }
    }

    /* make scale-factor from:
        ratio of minimum visible differences in luminance, in display-adapted
        and world-adapted perception (discluding the constant that cancelled),
        divided by display max to yield a [0,1] range */
    {
        const float a = 1.219 + pow( DISPLAY_LUMINANCE_MAX * 0.25, 0.4 );
        const float b = 1.219 + pow( adaptLuminance, 0.4 );

        return pow( a / b, 2.5 ) / DISPLAY_LUMINANCE_MAX;
    }
}





void hdr_accum( int x, int y, const V3f radiance ) // HEADER
{
    if( x <  0 ) return;
    if( y <  0 ) return;
    if( x >= W ) return;
    if( y >= H ) return;
    HDR[y][x] = HDR[y][x] + radiance;
}





void hdr_zero() // HEADER
{
    bzero(HDR,sizeof(HDR));
}







void firefly_filter( HDR_PIXMAP& out, HDR_PIXMAP& in ){

    bzero(out,sizeof(out));

#pragma omp parallel for
    for( int y=1; y<H-1 ; y++ ){
        for( int x=1; x<W-1 ; x++ ){

            // elimina bene le fireflies
            // qualche artefatto nelle zone d'ombra
            // ma niente di che
            V3f avg = in[y-1][x-1]+in[y-1][x-0]+in[y-1][x+1]+
                      in[y-0][x-1]       +      in[y-0][x+1]+
                      in[y+1][x-1]+in[y+1][x-0]+in[y+1][x+1];
            avg = avg*(2.0/8);

//            V3f avg =              in[y-1][x-0]+
//                      in[y-0][x-1]       +      in[y-0][x+1]+
//                                   in[y+1][x-0];
//            avg = avg*(2.0/4);

//            V3f avg = in[y][x-1]+in[y][x+1];
//            avg = avg*(2.0/2);

            float a = in[y][x].dot(in[y][x]);
            float b = avg.dot(avg);

            out[y][x] = (a > b ? avg : in[y][x]);

//            // bloom
//            // l'energia dei pixel leaka nei limitrofi
//            // mmm... fa cagare
//            V3f e1 = in[y][x];
//            V3f e2 = in[y][x]*(0.01);
//            V3f e4 = in[y][x]*(0.01/2);
//
//#define C(X,Y,E)    out[y+Y][x+X] = out[y+Y][x+X]+E
//
//            C(-1,-1,e4);    C(0,-1,e2);     C(+1,-1,e4);
//            C(-1, 0,e2);    C(0, 0,e1);     C(+1, 0,e2);
//            C(-1,+1,e4);    C(0,+1,e2);     C(+1,+1,e4);
//#undef C

        }
    }    
}











void hdr_to_sdl(){    // HEADER

    uint8_t RGB8[H][W][3];
    HDR_PIXMAP HDR2;

    firefly_filter( HDR2, HDR );

    float isamples = expo/samples;
    
#pragma omp parallel for
    for( int y=0; y<H ; y++ ){
        for( int x=0; x<W ; x++ ){

            // avg
            V3f color = HDR2[y][x] * isamples;

//            // max
//            V3f color;
//            color.X() = max(0,HDR2[y][x].x);
//            color.Y() = max(0,HDR2[y][x].y);
//            color.Z() = max(0,HDR2[y][x].z);

//            // Reinhard tone mapping - no gamma
//            V3f d = color + 3;
//            color = V3f(color.X() / d.X(), color.Y() / d.Y(), color.Z() / d.Z());
//            RGB8[y][x][0]=clamp(color.X(),0.0f,1.0f)*255;
//            RGB8[y][x][1]=clamp(color.Y(),0.0f,1.0f)*255;
//            RGB8[y][x][2]=clamp(color.Z(),0.0f,1.0f)*255;

            // Reinhard tone mapping - srgb
            V3f d = color + V3f::ONE;
            color = V3f( color.R()/d.R(), color.G()/d.G(), color.B()/d.B());
            RGB8[y][x][0]=pow(color.R(),GAMMA_ENCODE)*255;
            RGB8[y][x][1]=pow(color.G(),GAMMA_ENCODE)*255;
            RGB8[y][x][2]=pow(color.B(),GAMMA_ENCODE)*255;

//            // abs tone mapping
//            float d = sqrtf(color%color)+1;
//            color = V3f(color.X() / d, color.Y() / d, color.Z() / d);
//            RGB8[y][x][0]=clamp(color.X(),0.0f,1.0f)*255;
//            RGB8[y][x][1]=clamp(color.Y(),0.0f,1.0f)*255;
//            RGB8[y][x][2]=clamp(color.Z(),0.0f,1.0f)*255;

            // srgb 
            // con reinhart non si nota molta differenza
            // perche la curva applicata è abbastanza simile 
            // alla curva della correzione gamma
        }
    }

    SDL_UpdateTexture( framebuffer , NULL, RGB8, W*sizeof(RGB8[0][0]));
    SDL_RenderCopy( renderer, framebuffer , NULL , NULL );
}

