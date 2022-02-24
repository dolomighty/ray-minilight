/*------------------------------------------------------------------------------

   MiniLight C : minimal global illumination renderer
   Harrison Ainsworth / HXA7241 : 2009, 2011, 2013

   http://www.hxa.name/minilight

------------------------------------------------------------------------------*/

#include <V3f.h>     // HEADER
#include <SurfacePoint.h> // HEADER
#include <Random.h>       // HEADER
#include <Scene.h>        // HEADER




/**
 * Ray tracer for general light transport.<br/><br/>
 *
 * Traces a path with emitter sampling: A single chain of ray-steps advances
 * from the eye into the scene with one sampling of emitters at each
 * node.<br/><br/>
 *
 * Constant.
 *
 * @invariants
 * * pScene is not 0
 */

// HEADERBEG
struct RayTracer
{
   const Scene* pScene;
};

typedef struct RayTracer RayTracer;
// HEADEREND


/* implementation ----------------------------------------------------------- */

/**
 * Radiance from an emitter sample.
 */
// HEADERBEG
static V3f sampleEmitters
(
   const RayTracer* pR,
   const V3f* pRayBackDirection,
   const SurfacePoint* pSurfacePoint,
   Random* pRandom
)
// HEADEREND
{
   V3f radiance = V3f::ZERO;

   /* single emitter sample, ideal diffuse BRDF:
         reflected = (emitivity * solidangle) * (emitterscount) *
            (cos(emitdirection) / pi * reflectivity)
      -- SurfacePoint does the first and last parts (in separate methods) */

   /* get position on an emitter */
   V3f emitterPosition;
   const Triangle* emitterId = 0;
   SceneEmitter( pR->pScene, pRandom, &emitterPosition, &emitterId );

   /* check an emitter was found */
   if( emitterId )
   {
      /* make direction to emit point */
      V3f emitVector = emitterPosition - pSurfacePoint->position;
      const V3f emitDirection = emitVector.normalized();

      /* send shadow ray */
      const Triangle* pHitObject = 0;
      V3f hitPosition;
      SceneIntersection( pR->pScene, &pSurfacePoint->position, &emitDirection,
         SurfacePointHitId( pSurfacePoint ), &pHitObject, &hitPosition );

      /* check if unshadowed */
      if( !pHitObject | emitterId == pHitObject )
      {
         /* get inward emission value */
         const SurfacePoint sp = SurfacePointCreate( emitterId, &emitterPosition );
         const V3f backEmitDirection = -emitDirection;
         const V3f emissionIn = SurfacePointEmission( &sp, &pSurfacePoint->position, &backEmitDirection, true );
         const V3f emissionAll = emissionIn * SceneEmittersCount( pR->pScene );

         /* get amount reflected by surface */
         radiance = SurfacePointReflection( pSurfacePoint, &emitDirection,
            &emissionAll, pRayBackDirection );
      }
   }

   return radiance;
}




/* initialisation ----------------------------------------------------------- */

// HEADERBEG
RayTracer RayTracerCreate
(
   const Scene* pScene
)
// HEADEREND
{
   RayTracer r;
   r.pScene = pScene;

   return r;
}




/* queries ------------------------------------------------------------------ */

// HEADERBEG
V3f RayTracerRadiance
(
   const RayTracer* pR,
   const V3f* pRayOrigin,
   const V3f* pRayDirection,
   Random* pRandom,
   const void* lastHit
)
// HEADEREND
{
   V3f radiance;

   const V3f rayBackDirection = -*pRayDirection;

   /* intersect ray with scene */
   const Triangle* pHitObject = 0;
   V3f hitPosition;
   SceneIntersection( pR->pScene, pRayOrigin, pRayDirection, lastHit,
      &pHitObject, &hitPosition );

   if( pHitObject )
   {
      /* make surface point of intersection */
      const SurfacePoint surfacePoint = SurfacePointCreate( pHitObject, &hitPosition );

      /* local emission (only for first-hit) */
      const V3f localEmission = lastHit ? V3f::ZERO :
         SurfacePointEmission( &surfacePoint, pRayOrigin, &rayBackDirection, false );

      /* emitter sample */
      const V3f emitterSample = sampleEmitters( pR, &rayBackDirection, &surfacePoint, pRandom );

      /* recursed reflection */
      V3f recursedReflection = V3f::ZERO;
      {
         /* single hemisphere sample, ideal diffuse BRDF:
               reflected = (inradiance * pi) * (cos(in) / pi * color) *
                  reflectance
            -- reflectance magnitude is 'scaled' by the russian roulette,
            cos is importance sampled (both done by SurfacePoint),
            and the pi and 1/pi cancel out -- leaving just:
               inradiance * reflectance color */
         V3f nextDirection;
         V3f color;
         /* check surface reflects ray */
         if( SurfacePointNextDirection( &surfacePoint, pRandom,
            &rayBackDirection, &nextDirection, &color ) )
         {
            /* recurse */
            const V3f recursed = RayTracerRadiance( pR,
               &surfacePoint.position, &nextDirection, pRandom,
               SurfacePointHitId( &surfacePoint ));
            recursedReflection = recursed.pointwise( color );
         }
      }

      /* sum components */
      radiance = localEmission + emitterSample;
      radiance = recursedReflection + radiance;
   }
   else
   {
      /* no hit: default/background scene emission */
      radiance = SceneDefaultEmission( pR->pScene, &rayBackDirection );
   }

   const bool cx = fmod(hitPosition.X()+1e5,1) < 0.5;
   const bool cy = fmod(hitPosition.Y()+1e5,1) < 0.5;
   const bool cz = fmod(hitPosition.Z()+1e5,1) < 0.5;

   if( cx^cy^cz ) radiance = radiance * V3f(0.1,0.1,0.1);

   return radiance;
}
