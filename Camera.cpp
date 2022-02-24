/*------------------------------------------------------------------------------

   MiniLight C : minimal global illumination renderer
   Harrison Ainsworth / HXA7241 : 2009, 2011, 2013

   http://www.hxa.name/minilight

------------------------------------------------------------------------------*/

#include <setjmp.h>  // HEADER
#include <stdio.h>   // HEADER
#include <math.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <V3f.h>   // HEADER
#include <hdr.h>   // HEADER
#include <RayTracer.h>
#include <Scene.h>   // HEADER
#include <Random.h>  // HEADER

#include <globals.h>


// HEADERBEG

#define CameraEyePoint( pC ) ((pC)->viewPosition)

#define VIEW_ANGLE_MIN  10.0
#define VIEW_ANGLE_MAX 160.0

// HEADEREND


/**
 * View definition and rasterizer.<br/><br/>
 *
 * CameraFrame() accumulates a frame to the image.<br/><br/>
 *
 * Constant.
 *
 * @invariants
 * * viewAngle is >= 10 and <= 160 degrees, in radians
 * * viewDirection is unitized
 * * right is unitized
 * * up is unitized
 * * viewDirection, right, and up form a coordinate frame
 */

// HEADERBEG
#ifndef struct_Camera
#define struct_Camera

struct Camera
{
   /* eye definition */
   V3f viewPosition;
   double   viewAngle;

   /* view frame */
   V3f viewDirection;
   V3f right;
   V3f up;
};

typedef struct Camera Camera;

#endif
// HEADEREND




/* initialisation ----------------------------------------------------------- */

Camera *CameraCreate()   // HEADER
{

   Camera c;

   Camera *pc;
   assert( pc = (Camera*)malloc(sizeof(Camera)));

   const V3f Y( 0.0, 1.0, 0.0 );
   const V3f Z( 0.0, 0.0, 1.0 );

   /* read and condition view definition */
   {
      c.viewPosition = V3f( 0.278, 0.275, -0.789 );
      c.viewDirection = V3f( 0,0,1 );
      c.viewAngle = 40;

      c.viewDirection = c.viewDirection.normalized();
      /* if degenerate, default to Z */
      if( c.viewDirection.is_zero())
      {
         c.viewDirection = Z;
      }

      /* clamp and convert to radians */
      c.viewAngle = (c.viewAngle < VIEW_ANGLE_MIN ? VIEW_ANGLE_MIN :
         (c.viewAngle > VIEW_ANGLE_MAX ? VIEW_ANGLE_MAX : c.viewAngle)) *
         (M_PI / 180.0);
   }

   /* make other directions of view coord frame */
   {
      /* make trial 'right', using viewDirection and assuming 'up' is Y */
      V3f uxv = Y % c.viewDirection;
      c.up    = Y;
      c.right = uxv.normalized();

      /* check 'right' is valid
         -- i.e. viewDirection was not co-linear with 'up' */
      if( !c.right.is_zero())
      {
         /* use 'right', and make 'up' properly orthogonal */
         V3f vxr = c.viewDirection % c.right;
         c.up = vxr.normalized();
      }
      /* else, assume a different 'up' and redo */
      else
      {
         /* 'up' is Z if viewDirection is down, otherwise -Z */
         const V3f z = c.viewDirection.Y() < 0.0 ?
            Z : -Z;
         /* remake 'right' */
         V3f uxv = z % c.viewDirection;
         c.up    = z;
         c.right = uxv.normalized();
      }
   }

   memcpy(pc,&c,sizeof(*pc));
   return pc;
}




/* queries ------------------------------------------------------------------ */

// HEADERBEG
void CameraFrame
(
   const Camera* pC,
   const Scene*  pScene,
   Random*       pRandom
)
// HEADEREND
{
   const RayTracer rayTracer = RayTracerCreate( pScene );

   const double tanView = tan( pC->viewAngle * 0.5 );

#pragma omp parallel for
   for( int y=0;  y<H; ++y )
   {
      V3f row[W];
      float luma[W];
      for( int x=0;  x<W; ++x )
      {
         /* make sample ray direction, stratified by pixels */
         /* make image plane XY displacement vector [-1,+1) coefficients,
            with sub-pixel jitter */
         const double cx = (( (x + RandomReal64( pRandom )) * 2.0 / W ) - 1.0) * tanView;
         const double cy = (( (y + RandomReal64( pRandom )) * 2.0 / H ) - 1.0) * tanView * H / W;

         /* make image plane offset vector,
            by scaling the view definition by the coefficients */
         const V3f rcx = pC->right * +cx;
         const V3f ucy = pC->up    * -cy;
         const V3f offset = rcx + ucy;

         /* add image offset vector to view direction */
         V3f sdv = pC->viewDirection + offset;
         const V3f sampleDirection = sdv.normalized();

         /* get radiance from RayTracer */
         V3f radiance = RayTracerRadiance( &rayTracer,
            &pC->viewPosition, &sampleDirection, pRandom, 0 );

         /* add radiance to image */
         row[x]=radiance;
         luma[x]=radiance.luma();
      }


      for( int x=0;  x<W; ++x ) hdr_accum(x,y,row[x]);

//      // filtro quasi-mediano per fireflies
//      // se il pixel centrale è troppo distante dalla media dei due limitrofi
//      // scala la radianza
//      hdr_accum(0,y,row[0]);
//      for( int x=1;  x<W-1; ++x )
//      {
//         if( luma[x+0]/(luma[x-1]+luma[x+1]+1) < 2 ) hdr_accum(x,y,row[x]);
//      }
//      hdr_accum(W-1,y,row[W-1]);
   }
}



void CameraPrint( const Camera* pC )   // HEADER
{
   printf("viewPosition %f %f %f\n",pC->viewPosition.X(),pC->viewPosition.Y(),pC->viewPosition.Z());
   printf("viewDirection %f %f %f\n",pC->viewDirection.X(),pC->viewDirection.Y(),pC->viewDirection.Z());
   printf("right %f %f %f\n",pC->right.X(),pC->right.Y(),pC->right.Z());
   printf("up %f %f %f\n",pC->up.X(),pC->up.Y(),pC->up.Z());
}



