/*------------------------------------------------------------------------------

   MiniLight C : minimal global illumination renderer
   Harrison Ainsworth / HXA7241 : 2009, 2011, 2013

   http://www.hxa.name/minilight

------------------------------------------------------------------------------*/


#include <string.h>
#include <assert.h>
#include <math.h>    // HEADER
#include <stdio.h>   // HEADER



// HEADERBEG

struct V3f
{
   float v[3];
   
   static const V3f ZERO;
   static const V3f ONE;

   V3f( float x_=0, float y_=0, float z_=0 ){ v[0]=x_; v[1]=y_; v[2]=z_; }

   // accessors per leggibilità (-O3 dovrebbe metterli inline)
   float X() const { return v[0]; }
   float Y() const { return v[1]; }
   float Z() const { return v[2]; }
   float R() const { return v[0]; }
   float G() const { return v[1]; }
   float B() const { return v[2]; }

   V3f operator-() const { return V3f( -v[0], -v[1], -v[2] ); }
   V3f operator-( const V3f &b ) const { return V3f( v[0]-b.v[0], v[1]-b.v[1], v[2]-b.v[2]); }
   V3f operator+( const V3f &b ) const { return V3f( v[0]+b.v[0], v[1]+b.v[1], v[2]+b.v[2]); }
   float dot( const V3f &b ) const { return v[0]*b.v[0] + v[1]*b.v[1] + v[2]*b.v[2]; } 

   V3f scalar( const float b ) const { return V3f( v[0]*b, v[1]*b, v[2]*b ); }
   V3f operator*( const float b ) const { return scalar(b); }

   V3f cross( const V3f &b ) const { return V3f( 
      v[1]*b.v[2]-v[2]*b.v[1],
      v[2]*b.v[0]-v[0]*b.v[2],
      v[0]*b.v[1]-v[1]*b.v[0]
   );}
   V3f operator%( const V3f &b ) const { return cross(b); }

   V3f pointwise( const V3f &b ) const { return V3f( v[0]*b.v[0], v[1]*b.v[1], v[2]*b.v[2] ); }
   V3f hadamard ( const V3f &b ) const { return pointwise(b); }
   V3f operator*( const V3f &b ) const { return pointwise(b); }

   bool is_zero() const { return (v[0] == 0.0) & (v[1] == 0.0) & (v[2] == 0.0); }

   V3f clamped( const V3f &lo, const V3f &hi )
   {
      V3f c;
      c.v[0] = (v[0] < lo.v[0] ? lo.v[0] : (v[0] > hi.v[0] ? hi.v[0] : v[0]));
      c.v[1] = (v[1] < lo.v[1] ? lo.v[1] : (v[1] > hi.v[1] ? hi.v[1] : v[1]));
      c.v[2] = (v[2] < lo.v[2] ? lo.v[2] : (v[2] > hi.v[2] ? hi.v[2] : v[2]));
      return c;
   }

   V3f read( FILE *pIn );

   V3f normalized()
   {
      /* Zero vectors, and vectors of near zero magnitude, produce zero length,
         and (since 1 / 0 is conditioned to 0) ultimately a zero vector result.
         Vectors of extremely large magnitude produce +infinity length, and (since
         1 / inf is 0) ultimately a zero vector result.
         (Perhaps zero vectors should produce infinite results, but pragmatically,
         zeros are probably easier to handle than infinities.) */
      const float length = sqrt( this->dot( *this ));
      const float invlen = (length != 0.0 ? 1.0 / length : 0.0);
      return *this * invlen;
   }
   V3f norm(){ return normalized(); }

   float luma()
   {
      return 0.2126*this->R() + 0.7152*this->G() + 0.0722*this->B();
   }

};

typedef struct V3f V3f;

// HEADEREND


const V3f V3f::ZERO( 0.0, 0.0, 0.0 );
const V3f V3f::ONE( 1.0, 1.0, 1.0 );









V3f V3f::read( FILE* pIn )
{
//   while(1){
//      int c = fgetc(pIn);
//      if(c==EOF)exit(1);
//      fputc(c,stderr);
//   }

   assert( 3 == fscanf( pIn, " ( %f %f %f )", &v[0], &v[1], &v[2] )); // float

   return *this;
}



