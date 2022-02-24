   /*------------------------------------------------------------------------------

   MiniLight C : minimal global illumination renderer
   Harrison Ainsworth / HXA7241 : 2009, 2011, 2013

   http://www.hxa.name/minilight

------------------------------------------------------------------------------*/


#include <math.h>

#include <Triangle.h>   // HEADER
#include <V3f.h>   // HEADER
#include <Random.h>   // HEADER




/**
 * Surface point at a ray-object intersection.<br/><br/>
 *
 * All direction parameters are away from surface.<br/><br/>
 *
 * Constant.<br/><br/>
  *
 * @invariants
 * * pTriangle is not 0
*/


// HEADERBEG
#define SurfacePointHitId( pS ) ((const void*)(pS)->pTriangle)
// HEADEREND

// HEADERBEG
struct SurfacePoint
{
   const Triangle* pTriangle;
   V3f        position;
};

typedef struct SurfacePoint SurfacePoint;
// HEADEREND



/* constants ---------------------------------------------------------------- */

static const double PI = 3.14159265358979;




/* initialisation ----------------------------------------------------------- */

// HEADERBEG
SurfacePoint SurfacePointCreate
(
   const Triangle* pTriangle,
   const V3f* pPosition
)
// HEADEREND
{
   SurfacePoint s;
   s.pTriangle = pTriangle;
   s.position  = *pPosition;
   return s;
}




/* queries ------------------------------------------------------------------ */

// HEADERBEG
V3f SurfacePointEmission
(
   const SurfacePoint* pS,
   const V3f* pToPosition,
   const V3f* pOutDirection,
   bool isSolidAngle
)
// HEADEREND
{
   const V3f    ray       = *pToPosition - pS->position;
   const double distance2 = ray.dot( ray );
   const V3f    normal    = TriangleNormal( pS->pTriangle );
   const double cosOut    = pOutDirection->dot( normal );
   const double area      = TriangleArea( pS->pTriangle );

   /* emit from front face of surface only */
   const double solidAngle = (double)(cosOut > 0.0) * (isSolidAngle ?
      /* with infinity clamped-out */
      (cosOut * area) / (distance2 >= 1e-6 ? distance2 : 1e-6) : 1.0);

   return pS->pTriangle->emitivity * solidAngle;
}


// HEADERBEG
V3f SurfacePointReflection
(
   const SurfacePoint* pS,
   const V3f* pInDirection,
   const V3f* pInRadiance,
   const V3f* pOutDirection
)
// HEADEREND
{
   const V3f normal = TriangleNormal( pS->pTriangle );
   const double   inDot  = pInDirection->dot( normal );
   const double   outDot = pOutDirection->dot( normal );

   /* directions must be on same side of surface (no transmission) */
   const bool isSameSide = !( (inDot < 0.0) ^ (outDot < 0.0) );

   /* ideal diffuse BRDF:
      radiance scaled by reflectivity, cosine, and 1/pi  */
   const V3f r = *pInRadiance * pS->pTriangle->reflectivity;
   return r * (fabs( inDot ) / PI) * (double)isSameSide;
}


// HEADERBEG
bool SurfacePointNextDirection
(
   const SurfacePoint* pS,
   Random* pRandom,
   const V3f* pInDirection,
   V3f* pOutDirection_o,
   V3f* pColor_o
)
// HEADEREND
{
   const double reflectivityMean =
      pS->pTriangle->reflectivity.dot( V3f::ONE ) / 3.0;

   /* russian-roulette for reflectance 'magnitude' */
   const bool isAlive = RandomReal64( pRandom ) < reflectivityMean;

   if( isAlive )
   {
      /* cosine-weighted importance sample hemisphere */

      const double _2pr1 = PI * 2.0 * RandomReal64( pRandom );
      const double sr2   = sqrt( RandomReal64( pRandom ) );

      /* make coord frame coefficients (z in normal direction) */
      const double x = cos( _2pr1 ) * sr2;
      const double y = sin( _2pr1 ) * sr2;
      const double z = sqrt( 1.0 - (sr2 * sr2) );

      /* make coord frame */
      const V3f t = TriangleTangent( pS->pTriangle );
      V3f       n = TriangleNormal( pS->pTriangle );
      V3f       c;
      /* put normal on inward ray side of surface (preventing transmission) */
      if( n.dot( *pInDirection ) < 0.0 )
      {
         n = -n;
      }
      c = n % t;

      {
         /* scale frame by coefficients */
         const V3f tx = t * x;
         const V3f cy = c * y;
         const V3f nz = n * z;

         /* make direction from sum of scaled components */
         const V3f sum = tx + cy;
         *pOutDirection_o = sum + nz;
      }

      /* make color by dividing-out mean from reflectivity */
      *pColor_o = pS->pTriangle->reflectivity * (1.0 / reflectivityMean);
   }

   /* discluding degenerate result direction */
   return isAlive && !pOutDirection_o->is_zero();
}
