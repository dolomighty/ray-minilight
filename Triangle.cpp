/*------------------------------------------------------------------------------

   MiniLight C : minimal global illumination renderer
   Harrison Ainsworth / HXA7241 : 2009, 2011, 2013

   http://www.hxa.name/minilight

------------------------------------------------------------------------------*/


#include <math.h>
#include <stdio.h>    // HEADER
#include <V3f.h> // HEADER
#include <Random.h>   // HEADER


// HEADERBEG
#define TOLERANCE (1.0 / 1024.0)
// HEADEREND


// HEADERBEG
struct Triangle
{
   /* geometry */
   V3f aVertexs[3];

   /* quality */
   V3f reflectivity;
   V3f emitivity;
};

typedef struct Triangle Triangle;
// HEADEREND






/* constants ---------------------------------------------------------------- */

/* reasonable for single precision FP */
static const float EPSILON = 1.0 / (1<<20);


/* implementation ----------------------------------------------------------- */

/**
 * The normal vector, unnormalised.
 */
static V3f TriangleNormalV
(
   const Triangle* pT
)
{
   const V3f edge1 = pT->aVertexs[1] - pT->aVertexs[0];
   const V3f edge3 = pT->aVertexs[2] - pT->aVertexs[1];
   return edge1 % edge3;
}




/* initialisation ----------------------------------------------------------- */
// HEADERBEG
Triangle TriangleCreate 
(    
   FILE*   pIn
)
// HEADEREND
{
   Triangle t;

   /* read geometry */
   t.aVertexs[0].read( pIn );
   t.aVertexs[1].read( pIn );
   t.aVertexs[2].read( pIn );

   /* read and condition quality */
   t.reflectivity.read( pIn );
   t.reflectivity = t.reflectivity.clamped( V3f::ZERO, V3f::ONE );

   t.emitivity.read( pIn );
   t.emitivity = t.emitivity.clamped( V3f::ZERO, t.emitivity );

   return t;
}




/* queries ------------------------------------------------------------------ */
// HEADERBEG
void TriangleBound(
   const Triangle* pT,
   float          aBound_o[6]
)
// HEADEREND
{

   for( int i=0; i<3; i++ ){
      aBound_o[i+0] = pT->aVertexs[0].v[i];
      if( aBound_o[i+0] > pT->aVertexs[1].v[i] ) aBound_o[i+0] = pT->aVertexs[1].v[i];
      if( aBound_o[i+0] > pT->aVertexs[2].v[i] ) aBound_o[i+0] = pT->aVertexs[2].v[i];
      aBound_o[i+3] = pT->aVertexs[0].v[i];
      if( aBound_o[i+3] < pT->aVertexs[1].v[i] ) aBound_o[i+3] = pT->aVertexs[1].v[i];
      if( aBound_o[i+3] < pT->aVertexs[2].v[i] ) aBound_o[i+3] = pT->aVertexs[2].v[i];
   }

   for( int i=0; i<3; i++ ){
      aBound_o[i+0] -= TOLERANCE;
      aBound_o[i+3] += TOLERANCE;
   }
}


/**
 * @implementation
 * Adapted from:
 * <cite>'Fast, Minimum Storage Ray-Triangle Intersection';
 * Moller, Trumbore;
 * Journal Of Graphics Tools, v2n1p21; 1997.
 * http://www.acm.org/jgt/papers/MollerTrumbore97/</cite>
 */
// HEADERBEG
bool TriangleIntersection
(
   const Triangle* pT,
   const V3f* pRayOrigin,
   const V3f* pRayDirection,
   float*         pHitDistance_o
)
// HEADEREND
{
  float inv_det;
  V3f tvec;
  float u;
  V3f qvec;
  float v;

  /* make vectors for two edges sharing vert0 */
  const V3f edge1 = pT->aVertexs[1] - pT->aVertexs[0];
  const V3f edge2 = pT->aVertexs[2] - pT->aVertexs[0];

  /* begin calculating determinant - also used to calculate U parameter */
  const V3f pvec = *pRayDirection % edge2;

  /* if determinant is near zero, ray lies in plane of triangle */
  const float det = edge1.dot( pvec );

  if( det >= -EPSILON && det <= +EPSILON ) return false;

  inv_det = 1.0 / det;

  /* calculate distance from vertex 0 to ray origin */
  tvec = *pRayOrigin - pT->aVertexs[0];

  /* test bounds */
  u = tvec.dot( pvec ) * inv_det;
  if( u <= 0.0 || u >= 1.0 ) return false;

  /* prepare to test V parameter */
  qvec = tvec % edge1;

  /* test bounds */
  v = pRayDirection->dot( qvec ) * inv_det;
  if( v <= 0.0 || u + v >= 1.0 ) return false;

  /* calculate t, ray intersects triangle */
  *pHitDistance_o = edge2.dot( qvec ) * inv_det;

  /* only allow intersections in the forward ray direction */
  return (*pHitDistance_o >= 0.0);
}


// HEADERBEG
V3f TriangleSamplePoint
(
   const Triangle* pT,
   Random*         pRandom
)
// HEADEREND
{
   /* get two randoms */
   const float sqr1 = sqrt( RandomReal64( pRandom ) );
   const float r2   = RandomReal64( pRandom );

   /* make barycentric coords */
   const float c0 = 1.0 - sqr1;
   const float c1 = (1.0 - r2) * sqr1;
   /*const float c2 = r2 * sqr1;*/

   /* make barycentric axes */
   const V3f a0 = pT->aVertexs[1] - pT->aVertexs[0];
   const V3f a1 = pT->aVertexs[2] - pT->aVertexs[0];

   /* scale axes by coords */
   const V3f ac0 = a0 * c0;
   const V3f ac1 = a1 * c1;

   /* sum scaled components, and offset from corner */
   const V3f sum = ac0 + ac1;
   return sum + pT->aVertexs[0];
}


// HEADERBEG
V3f TriangleNormal
(
   const Triangle* pT
)
// HEADEREND
{
   V3f normalV = TriangleNormalV( pT );
   return normalV.normalized();
}


// HEADERBEG
V3f TriangleTangent
(
   const Triangle* pT
)
// HEADEREND
{
   V3f edge1 = pT->aVertexs[1] - pT->aVertexs[0];
   return edge1.normalized();
}


// HEADERBEG
float TriangleArea
(
   const Triangle* pT
)
// HEADEREND
{
   /* half area of parallelogram (area = magnitude of cross of two edges) */
   const V3f normalV = TriangleNormalV( pT );
   return sqrt( normalV.dot( normalV )) * 0.5;
}

