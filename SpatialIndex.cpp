/*------------------------------------------------------------------------------

   MiniLight C : minimal global illumination renderer
   Harrison Ainsworth / HXA7241 : 2009, 2011, 2013

   http://www.hxa.name/minilight

------------------------------------------------------------------------------*/


#include <stdlib.h>
#include <stdint.h>
#include <float.h>
#include <assert.h>

#include <Triangle.h>   // HEADER

/**
 * A minimal spatial index for ray tracing.<br/><br/>
 *
 * Suitable for a scale of 1 metre == 1 numerical unit, and with a resolution
 * of 1 millimetre. (Implementation uses fixed tolerances)
 *
 * Constant.<br/><br/>
 *
 * @implementation
 * A crude State pattern: typed by isBranch field to be either a branch
 * or leaf cell.<br/><br/>
 *
 * Octree: axis-aligned, cubical. Subcells are numbered thusly:
 * <pre>      110---111
 *            /|    /|
 *         010---011 |
 *    y z   | 100-|-101
 *    |/    |/    | /
 *    .-x  000---001      </pre><br/><br/>
 *
 * Each cell stores its bound (fatter data, but simpler code).<br/><br/>
 *
 * Calculations for building and tracing are absolute rather than incremental --
 * so quite numerically solid. Uses tolerances in: bounding triangles (in
 * TriangleBound), and checking intersection is inside cell (both effective
 * for axis-aligned items). Also, depth is constrained to an absolute subcell
 * size (easy way to handle overlapping items).
 *
 * @invariants
 * * aBound[0-2] <= aBound[3-5]
 * * bound encompasses the cell's contents
 * if isBranch
 * * apArray elements are SpatialIndex pointers or zeros
 * * length (of apArray) is 8
 * else
 * * apArray elements are non-zero Triangle pointers
 */


// HEADERBEG
struct SpatialIndex
{
   bool         isBranch;
   float       aBound[6];
   const void** apArray;
   int      length;
};

typedef struct SpatialIndex SpatialIndex;
// HEADEREND


/* constants ---------------------------------------------------------------- */

/* accommodates scene including sun and earth, down to cm cells
   (use 47 for mm) */
static const int MAX_LEVELS = 44;

/* 8 seemed reasonably optimal in casual testing */
static const int MAX_ITEMS  =  8;




/* implementation ----------------------------------------------------------- */

static void construct
(
   Triangle** apItems,
   const int      itemsLength,
   const int      level,
   SpatialIndex*    pS_o
)
{
   /* is branch if items overflow leaf and tree not too deep */
   pS_o->isBranch = (itemsLength > MAX_ITEMS) & (level < (MAX_LEVELS - 1));

   /* make branch: make sub-cells, and recurse construction */
   if( pS_o->isBranch )
   {
      int s, q;

      /* make subcells */
      pS_o->length  = 8;
      assert( pS_o->apArray = (const void**)calloc( pS_o->length, sizeof(const void*)));
      for( s = pS_o->length, q = 0;  s-- > 0; )
      {
         float aSubBound[6];
         int  subItemsLength = 0;
         Triangle** apSubItems;
         assert( apSubItems = (Triangle**)calloc( subItemsLength, sizeof(Triangle*)));

         int j, d, m, i;

         /* make subcell bound */
         for( j = 0, d = 0, m = 0;  j < 6;  ++j, d = j / 3, m = j % 3 )
         {
            aSubBound[j] = ((s >> m) & 1) ^ d ? (pS_o->aBound[m] +
               pS_o->aBound[m + 3]) * 0.5 : pS_o->aBound[j];
         }

         /* collect items that overlap subcell */
         for( i = itemsLength;  i-- > 0; )
         {
            int isOverlap = 1;

            /* must overlap in all dimensions */
            float aItemBound[6];
            TriangleBound( apItems[i], aItemBound );
            for( j = 0, d = 0, m = 0;  j < 6;  ++j, d = j / 3, m = j % 3 )
            {
               isOverlap &= (aItemBound[(d ^ 1) * 3 + m] >= aSubBound[j]) ^ d;
            }

            /* maybe append to subitems store */
            if( isOverlap )
            {
               assert( apSubItems = (Triangle**)realloc( (Triangle**)apSubItems, ++subItemsLength * sizeof(Triangle*)));
               apSubItems[subItemsLength - 1] = (Triangle*)apItems[i];
            }
         }

         q += subItemsLength == itemsLength ? 1 : 0;

         /* maybe make subcell, if any overlapping subitems */
         if( subItemsLength > 0 )
         {
            SpatialIndex* pS;
            assert( pS = (SpatialIndex*)calloc( 1, sizeof(SpatialIndex)));
            /* curtail degenerate subdivision by adjusting next level
               (degenerate if two or more subcells copy entire contents of
               parent, or if subdivision reaches below mm size)
               (having a model including the sun requires one subcell copying
               entire contents of parent to be allowed) */
            const int nextLevel = (q > 1) | ((aSubBound[3] - aSubBound[0]) <
               (TOLERANCE * 4.0)) ? MAX_LEVELS : level + 1;

            pS_o->apArray[s] = pS;
            for( i = 6;  i-- > 0;  pS->aBound[i] = aSubBound[i] ) {}

            /* recurse */
            construct( apSubItems, subItemsLength, nextLevel, pS );
         }

         free( (Triangle**)apSubItems );
      }
   }
   /* make leaf: store items, and end recursion */
   else
   {
      int i;

      /* alloc */
      pS_o->length  = itemsLength;
      assert( pS_o->apArray = (const void**)calloc( pS_o->length, sizeof(const void*)));

      /* copy */
      for( i = pS_o->length;  i-- > 0;  pS_o->apArray[i] = apItems[i] ) {}
   }
}




/* initialisation ----------------------------------------------------------- */

// HEADERBEG
const SpatialIndex* SpatialIndexConstruct
(
   const V3f* pEyePosition,
   const Triangle* aItems,
   int           itemsLength
)
// HEADEREND
{
   SpatialIndex* pS;
   assert( pS = (SpatialIndex*)calloc( 1, sizeof(SpatialIndex)));

   /* set overall bound (and convert to collection of pointers) */
   Triangle** apItems;
   assert( apItems = (Triangle**)calloc( itemsLength, sizeof(Triangle*)));
   
   {
      int i, j;

      /* accommodate eye position (makes tracing algorithm simpler) */
      for( i = 6;  i-- > 0;  pS->aBound[i] = pEyePosition->v[i % 3] ) {}

      /* accommodate all items */
      for( i = itemsLength;  i-- > 0;  apItems[i] = (Triangle*)&aItems[i] )
      {
         float aItemBound[6];
         TriangleBound( &aItems[i], aItemBound );

         /* accommodate item */
         for( j = 0;  j < 6;  ++j )
         {
            if( (pS->aBound[j] > aItemBound[j]) ^ (j > 2) )
            {
               pS->aBound[j] = aItemBound[j];
            }
         }
      }

      /* make cubical */
      {
         float maxSize = 0.0, *b = 0;
         /* find max dimension */
         for( b = pS->aBound + 3;  b-- > pS->aBound; )
         {
            if( maxSize < (b[3] - b[0]) ) maxSize = b[3] - b[0];
         }
         /* set all dimensions to max */
         for( b = pS->aBound + 3;  b-- > pS->aBound; )
         {
            if( b[3] < (b[0] + maxSize) ) b[3] = b[0] + maxSize;
         }
      }
   }

   /* make subcell tree */
   construct( apItems, itemsLength, 0, pS );

   free( (Triangle**)apItems );

   return pS;
}


// HEADERBEG
void SpatialIndexDestruct
(
   SpatialIndex* pS
)
// HEADEREND
{
   /* recurse through branch subcells */
   int i;
   for( i = pS->length;  pS->isBranch & (i-- > 0); )
   {
      if( pS->apArray[i] )
      {
         SpatialIndexDestruct( (SpatialIndex*)pS->apArray[i] );
      }
   }

   free( (void**)pS->apArray );

   free( pS );
}




/* queries ------------------------------------------------------------------ */

// HEADERBEG
void SpatialIndexIntersection
(
   const SpatialIndex* pS,
   const V3f*     pRayOrigin,
   const V3f*     pRayDirection,
   const void*         lastHit,
   const V3f*     pStart,
   const Triangle**    ppHitObject_o,
   V3f*           pHitPosition_o
)
// HEADEREND
{
   /* is branch: step through subcells and recurse */
   if( pS->isBranch )
   {
      int subCell, i;
      V3f cellPosition;

      pStart = pStart ? pStart : pRayOrigin;

      /* find which subcell holds ray origin (ray origin is inside cell) */
      for( subCell = 0, i = 0;  i<3; ++i )
      {
         /* compare dimension with center */
         subCell |= (pStart->v[i] >= ((pS->aBound[i] + pS->aBound[i+3]) * 0.5)) << i;
      }

      /* step through intersected subcells */
      for( cellPosition = *pStart;  ; )
      {
         int axis = 2, i;
         float step[3];

         if( pS->apArray[subCell] )
         {
            /* intersect subcell (by recursing) */
            SpatialIndexIntersection( (const SpatialIndex*)
               pS->apArray[subCell], pRayOrigin, pRayDirection, lastHit,
               &cellPosition, ppHitObject_o, pHitPosition_o );

            /* exit branch (this function) if item hit */
            if( *ppHitObject_o )
            {
               break;
            }
         }

         /* find next subcell ray moves to
            (by finding which face of the corner ahead is crossed first) */
         for( i = 3;  i-- > 0;  axis = step[i] < step[axis] ? i : axis )
         {
            /* find which face (inter-/outer-) the ray is heading for (in this
               dimension) */
            const bool   high = (subCell >> i) & 1;
            const float face = (pRayDirection->v[i] < 0.0) ^ high ?
               pS->aBound[i + (high * 3)] :
               (pS->aBound[i] + pS->aBound[i + 3]) * 0.5;

            /* calculate distance to face
               (div by zero produces infinity, which is later discarded) */
            step[i] = (face - pRayOrigin->v[i]) / pRayDirection->v[i];
            /* last clause of for-statement notes nearest so far */
         }



         /* leaving branch if: direction is negative and subcell is low,
            or direction is positive and subcell is high */
         if( ((subCell >> axis) & 1) ^ (pRayDirection->v[axis] < 0.0) )
         {
            break;
         }


         /* move to (outer face of) next subcell */
         {
            const V3f rs = *pRayDirection * step[axis];
            cellPosition = *pRayOrigin + rs;
            subCell      = subCell ^ (1 << axis);
         }
      }
   }
   /* is leaf: exhaustively intersect contained items */
   else
   {
      float nearestDistance = DBL_MAX;
      int i;

      *ppHitObject_o = 0;

      /* step through items */
      for( i = pS->length;  i-- > 0; )
      {
         const Triangle* pItem = (const Triangle*)(pS->apArray[i]);

         /* avoid spurious intersection with surface just come from */
         if( pItem != lastHit )
         {
            /* intersect ray with item, and inspect if nearest so far */
            float distance = DBL_MAX;
            if( TriangleIntersection( pItem, pRayOrigin, pRayDirection,
               &distance ) && (distance < nearestDistance) )
            {
               /* check intersection is inside cell bound (with tolerance) */
               const V3f ray = *pRayDirection * distance;
               const V3f hit = *pRayOrigin + ray;
               if( (pS->aBound[0] - hit.X() <= TOLERANCE) &
                   (hit.X() - pS->aBound[3] <= TOLERANCE) &
                   (pS->aBound[1] - hit.Y() <= TOLERANCE) &
                   (hit.Y() - pS->aBound[4] <= TOLERANCE) &
                   (pS->aBound[2] - hit.Z() <= TOLERANCE) &
                   (hit.Z() - pS->aBound[5] <= TOLERANCE) )
               {
                  /* note nearest so far */
                  *ppHitObject_o  = pItem;
                  nearestDistance = distance;
                  *pHitPosition_o = hit;
               }
            }
         }
      }
   }
}
