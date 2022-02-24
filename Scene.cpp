/*------------------------------------------------------------------------------

   MiniLight C : minimal global illumination renderer
   Harrison Ainsworth / HXA7241 : 2009, 2011, 2013

   http://www.hxa.name/minilight

------------------------------------------------------------------------------*/


#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <obj_import.h>
#include <stdint.h>       // HEADER
#include <Triangle.h>     // HEADER
#include <SpatialIndex.h> // HEADER
#include <V3f.h>          // HEADER




// HEADERBEG
#define SceneEmittersCount( pS ) ((pS)->emittersLength)
#define MAX_TRIANGLES ((int)0x1000000)
// HEADEREND


/**
 * Collection of objects in the environment.<br/><br/>
 *
 * Constant.
 *
 * @invariants
 * * trianglesLength < MAX_TRIANGLES and >= 0
 * * emittersLength  < MAX_TRIANGLES and >= 0
 * * pIndex is not 0
 * * skyEmission      >= 0
 * * groundReflection >= 0 and <= 1
 */

// HEADERBEG
struct Scene
{
   /* objects */
   Triangle*   aTriangles;
   int     trianglesLength;

   Triangle**  apEmitters;
   int     emittersLength;

   SpatialIndex* pIndex;

   /* background */
   V3f   skyEmission;
   V3f   groundReflection;
};

typedef struct Scene Scene;

// HEADEREND



Scene* tri_cb_ps;

static TRI_CB_DEF(tri_cb){
   Triangle t;

   t.aVertexs[0].v[0] = a[0];
   t.aVertexs[0].v[1] = a[1];
   t.aVertexs[0].v[2] = a[2];

   t.aVertexs[1].v[0] = b[0];
   t.aVertexs[1].v[1] = b[1];
   t.aVertexs[1].v[2] = b[2];

   t.aVertexs[2].v[0] = c[0];
   t.aVertexs[2].v[1] = c[1];
   t.aVertexs[2].v[2] = c[2];

   t.reflectivity.v[0] = d[0];
   t.reflectivity.v[1] = d[1];
   t.reflectivity.v[2] = d[2];

   t.emitivity.v[0] = e[0];
   t.emitivity.v[1] = e[1];
   t.emitivity.v[2] = e[2];

   /* append to objects storage */
   assert( tri_cb_ps );
   assert( tri_cb_ps->aTriangles = (Triangle*)realloc( tri_cb_ps->aTriangles, ++tri_cb_ps->trianglesLength * sizeof(Triangle)));
   tri_cb_ps->aTriangles[tri_cb_ps->trianglesLength-1] = t;
}



/* initialisation ----------------------------------------------------------- */

// HEADERBEG
Scene* SceneConstruct
(
   const char *wavefront_obj_path,
   const V3f* pEyePosition
)
// HEADEREND
{
   assert(pEyePosition);

   Scene* pS;
   assert( pS = (Scene*)calloc( 1, sizeof(Scene)));

   pS->skyEmission      = V3f( 0.0906, 0.0943, 0.1151 );
   pS->groundReflection = V3f( 0.1,    0.09,   0.07   );

   assert( pS->aTriangles = (Triangle*)calloc( 0, sizeof(Triangle)) );
   pS->trianglesLength = 0;

   tri_cb_ps = pS;
   obj_import( wavefront_obj_path, tri_cb );

   /* find emitting objects */
   {
      int i;

      assert( pS->apEmitters = (Triangle**)calloc( 0, sizeof(Triangle*)));
      pS->emittersLength = 0;

      for( i = 0;  i < pS->trianglesLength;  ++i )
      {
         /* has non-zero emission and area */
         if( !pS->aTriangles[i].emitivity.is_zero() &&
            (TriangleArea( &pS->aTriangles[i] ) > 0.0) )
         {
            /* append to emitters storage */
            assert( pS->apEmitters = (Triangle**)realloc( pS->apEmitters, ++pS->emittersLength * sizeof(Triangle*)));
            pS->apEmitters[pS->emittersLength - 1] = &(pS->aTriangles[i]);
         }
      }
   }

   /* make index of objects */
   pS->pIndex = (SpatialIndex*)SpatialIndexConstruct( pEyePosition, pS->aTriangles, pS->trianglesLength );
   return pS;
}


// HEADERBEG
void SceneDestruct
(
   Scene* pS
)
// HEADEREND
{
   SpatialIndexDestruct( pS->pIndex );
   free( pS->apEmitters );
   free( pS->aTriangles );
   free( pS );
}




/* queries ------------------------------------------------------------------ */

// HEADERBEG
void SceneIntersection
(
   const Scene*     pS,
   const V3f*  pRayOrigin,
   const V3f*  pRayDirection,
   const void*      lastHit,
   const Triangle** ppHitObject_o,
   V3f*        pHitPosition_o
)
// HEADEREND
{
   SpatialIndexIntersection( pS->pIndex, pRayOrigin, pRayDirection, lastHit,
      0, ppHitObject_o, pHitPosition_o );
}


// HEADERBEG
void SceneEmitter
(
   const Scene*     pS,
   Random*          pRandom,
   V3f*        pPosition_o,
   const Triangle** pId_o
)
// HEADEREND
{
   if( pS->emittersLength > 0 )
   {
      /* select emitter */
      int index = (int)floor( RandomReal64( pRandom ) *
         (double)pS->emittersLength );
      index = index < pS->emittersLength ? index : pS->emittersLength - 1;

      /* choose position on emitter */
      *pPosition_o = TriangleSamplePoint( pS->apEmitters[index], pRandom );
      *pId_o       = pS->apEmitters[index];
   }
   else
   {
      *pPosition_o = V3f::ZERO;
      *pId_o       = 0;
   }
}


// HEADERBEG
V3f SceneDefaultEmission
(
   const Scene*    pS,
   const V3f* pBackDirection
)
// HEADEREND
{
   /* sky for downward ray, ground for upward ray */
   return (pBackDirection->Y() < 0.0) ?
      pS->skyEmission : pS->skyEmission * pS->groundReflection;
}
