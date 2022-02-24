

#include <math.h>
#include <string.h>
#include <stdio.h>



// HEADERBEG
union M34 {
    
public:
    float raw [ 3 * 4 ];
    struct {
        float col [ 4 ];
    } row [ 3 ];
    struct {
        float x ; float M01 ; float M02 ; float M03 ;
        float y ; float M11 ; float M12 ; float M13 ;
        float z ; float M21 ; float M22 ; float M23 ;
    } x ;
    struct {
        float M00 ; float x ; float M02 ; float M03 ;
        float M10 ; float y ; float M12 ; float M13 ;
        float M20 ; float z ; float M22 ; float M23 ;
    } y ;
    struct {
        float M00 ; float M01 ; float x ; float M03 ;
        float M10 ; float M11 ; float y ; float M13 ;
        float M20 ; float M21 ; float z ; float M23 ;
    } z ;
    struct {
        float M00 ; float M01 ; float M02 ; float x ;
        float M10 ; float M11 ; float M12 ; float y ;
        float M20 ; float M21 ; float M22 ; float z ;
    } t ;

    M34();
    M34(
        float m00, float m01, float m02, float m03, 
        float m10, float m11, float m12, float m13, 
        float m20, float m21, float m22, float m23
    );

    void identity  ();
    void rotate_z  ( float r );
    void rotate_y  ( float r );
    void rotate_x  ( float r );
    void rotate    ( float x , float y , float z , float r );
    void translate ( float x , float y , float z );
    void multiply  ( M34 * other );
    void normalize ();
    void invert    ( M34 * dst , M34 * src );
    void to_opengl_matrix  ( float * glm );
    void to_opengl_heading ( float * glm );
    char *to_code( char *buf, int len );
    void m_dot_v( float *o, const float *i );
};

// HEADEREND






#define F(i,j)   tmp.row[i].col[j]
#define T(i,j)   row[i].col[j]
    

M34::M34(){
    identity();
}

M34::M34(
    float m00, float m01, float m02, float m03, 
    float m10, float m11, float m12, float m13, 
    float m20, float m21, float m22, float m23
){
    #define C(R,C) raw[R*4+C]=m##R##C
    C(0,0); C(0,1); C(0,2); C(0,3);
    C(1,0); C(1,1); C(1,2); C(1,3);
    C(2,0); C(2,1); C(2,2); C(2,3);
    #undef C
}

void M34::identity(){
    bzero( this , sizeof( M34 ));
    T(0,0) = 
    T(1,1) = 
    T(2,2) = 1;
}



void M34::rotate_z( float r ){
    M34  tmp = *this ;
    float   s = sin( r );
    float   c = cos( r );
    T(0,0) = F(0,0)*c - F(0,1)*s;    T(0,1) = F(0,0)*s + F(0,1)*c;      T(0,2) = F(0,2);
    T(1,0) = F(1,0)*c - F(1,1)*s;    T(1,1) = F(1,0)*s + F(1,1)*c;      T(1,2) = F(1,2);
    T(2,0) = F(2,0)*c - F(2,1)*s;    T(2,1) = F(2,0)*s + F(2,1)*c;      T(2,2) = F(2,2);
}


void M34::rotate_y( float r ){
    M34  tmp = *this ;
    float   s = sin( r );
    float   c = cos( r );
    T(0,0) = F(0,0)*c - F(0,2)*s;    T(0,1) = F(0,1);      T(0,2) = F(0,0)*s + F(0,2)*c;
    T(1,0) = F(1,0)*c - F(1,2)*s;    T(1,1) = F(1,1);      T(1,2) = F(1,0)*s + F(1,2)*c;
    T(2,0) = F(2,0)*c - F(2,2)*s;    T(2,1) = F(2,1);      T(2,2) = F(2,0)*s + F(2,2)*c;
}


void M34::rotate_x( float r ){
    M34  tmp = *this ;
    float   s = sin( r );
    float   c = cos( r );
    T(0,0) = F(0,0);    T(0,1) = F(0,1)*c + F(0,2)*s;   T(0,2) = F(0,2)*c - F(0,1)*s ;
    T(1,0) = F(1,0);    T(1,1) = F(1,1)*c + F(1,2)*s;   T(1,2) = F(1,2)*c - F(1,1)*s ;
    T(2,0) = F(2,0);    T(2,1) = F(2,1)*c + F(2,2)*s;   T(2,2) = F(2,2)*c - F(2,1)*s ;
}


void M34::rotate( float x , float y , float z , float r ){
    
    float   s = sin( r );
    float   c = cos( r );
    float   k = 1 - c ;
    
    M34     rot ;
#define     R(i,j)  rot.row[i].col[j]
    
    M34  tmp = *this ;
    
// http://en.wikipedia.org/wiki/Rotation_matrix#Axis_and_angle
    
    R(0,0) = x*x*k+c   ;    R(0,1) = x*y*k-z*s ;    R(0,2) = x*z*k+y*s ;
    R(1,0) = y*x*k+z*s ;    R(1,1) = y*y*k+c   ;    R(1,2) = y*z*k-x*s ;
    R(2,0) = z*x*k-y*s ;    R(2,1) = z*y*k+x*s ;    R(2,2) = z*z*k+c   ;

#define F(i,j)   tmp.row[i].col[j]
#define T(i,j)   row[i].col[j]
#define M(i,j)   T(i,j) = F(i,0)*R(0,j)+F(i,1)*R(1,j)+F(i,2)*R(2,j)
    M(0,0);     M(0,1);     M(0,2); 
    M(1,0);     M(1,1);     M(1,2); 
    M(2,0);     M(2,1);     M(2,2); 
}



void M34::translate( float x , float y , float z ){
    T(0,3) += T(0,0)*x + T(0,1)*y + T(0,2)*z;
    T(1,3) += T(1,0)*x + T(1,1)*y + T(1,2)*z;
    T(2,3) += T(2,0)*x + T(2,1)*y + T(2,2)*z;
}


#define     A(i,j)  tmp.row[i].col[j]
#define     B(i,j)  other->row[i].col[j]
#undef M
#define     M(i,j)  T(i,j) = A(i,0)*B(0,j)+A(i,1)*B(1,j)+A(i,2)*B(2,j)
                    
                    
void M34::multiply( M34 * other ){
    M34  tmp = *this ;
    M(0,0);     M(0,1);     M(0,2);     M(0,3)+A(0,3);
    M(1,0);     M(1,1);     M(1,2);     M(1,3)+A(1,3);
    M(2,0);     M(2,1);     M(2,2);     M(2,3)+A(2,3);
//    T(0,0) = A(0,0)*B(0,0)+A(0,1)*B(1,0)+A(0,2)*B(2,0);     T(0,1) = A(0,0)*B(0,1)+A(0,1)*B(1,1)+A(0,2)*B(2,1);     T(0,2) = A(0,0)*B(0,2)+A(0,1)*B(1,2)+A(0,2)*B(2,2);     T(0,3) = A(0,0)*B(0,3)+A(0,1)*B(1,3)+A(0,2)*B(2,3)+A(0,3);
//    T(1,0) = A(1,0)*B(0,0)+A(1,1)*B(1,0)+A(1,2)*B(2,0);     T(1,1) = A(1,0)*B(0,1)+A(1,1)*B(1,1)+A(1,2)*B(2,1);     T(1,2) = A(1,0)*B(0,2)+A(1,1)*B(1,2)+A(1,2)*B(2,2);     T(1,3) = A(1,0)*B(0,3)+A(1,1)*B(1,3)+A(1,2)*B(2,3)+A(1,3);
//    T(2,0) = A(2,0)*B(0,0)+A(2,1)*B(1,0)+A(2,2)*B(2,0);     T(2,1) = A(2,0)*B(0,1)+A(2,1)*B(1,1)+A(2,2)*B(2,1);     T(2,2) = A(2,0)*B(0,2)+A(2,1)*B(1,2)+A(2,2)*B(2,2);     T(2,3) = A(2,0)*B(0,3)+A(2,1)*B(1,3)+A(2,2)*B(2,3)+A(2,3);
}



#define SQ(A) ((A)*(A))


void M34::normalize(){
    float   d ;
    
    d = 1.0 / sqrt( SQ( T(0,0)) + SQ( T(1,0)) + SQ( T(2,0)));
    T(0,0) *= d ;
    T(1,0) *= d ;
    T(2,0) *= d ;
    
    d = 1.0 / sqrt( SQ( T(0,1)) + SQ( T(1,1)) + SQ( T(2,1)));
    T(0,1) *= d ;
    T(1,1) *= d ;
    T(2,1) *= d ;
    
    d = 1.0 / sqrt( SQ( T(0,2)) + SQ( T(1,2)) + SQ( T(2,2)));
    T(0,2) *= d ;
    T(1,2) *= d ;
    T(2,2) *= d ;
}


void M34::invert( M34 * dst , M34 * src ){
#undef      S
#define     S(i,j)  src->row[i].col[j]
#undef      D
#define     D(i,j)  dst->row[i].col[j]
        
    D(0,0) = S(0,0);    D(0,1) = S(1,0);    D(0,2) = S(2,0);
    D(1,0) = S(0,1);    D(1,1) = S(1,1);    D(1,2) = S(2,1);
    D(2,0) = S(0,2);    D(2,1) = S(1,2);    D(2,2) = S(2,2);
    
    D(0,3) = -( S(0,0)*S(0,3) + S(1,0)*S(1,3) + S(2,0)*S(2,3));
    D(1,3) = -( S(0,1)*S(0,3) + S(1,1)*S(1,3) + S(2,1)*S(2,3));
    D(2,3) = -( S(0,2)*S(0,3) + S(1,2)*S(1,3) + S(2,2)*S(2,3));
}




void M34::to_opengl_matrix( float * glm ){
#undef      D
#define     D(i,j)  glm[i+4*j]
#undef      C
#define     C(i,j)  D(i,j) = row[i].col[j]
#undef      Z
#define     Z(i,j)  D(i,j) = 0
#undef      O
#define     O(i,j)  D(i,j) = 1
    // matrice opengl column major come richiesta da glMultMatrix et al
    // | 0 4  8 12 |
    // | 1 5  9 13 |
    // | 2 6 10 14 |
    // | 3 7 11 15 |
    C(0,0) ; C(0,1) ; C(0,2) ; C(0,3);
    C(1,0) ; C(1,1) ; C(1,2) ; C(1,3);
    C(2,0) ; C(2,1) ; C(2,2) ; C(2,3);
    Z(3,0) ; Z(3,1) ; Z(3,2) ; O(3,3);
}



void M34::to_opengl_heading( float * glm ){
    // come sopra ma senza traslazione
#undef      D
#define     D(i,j)  glm[i+4*j]
#undef      C
#define     C(i,j)  D(i,j) = row[i].col[j]
#undef      Z
#define     Z(i,j)  D(i,j) = 0
#undef      O
#define     O(i,j)  D(i,j) = 1
    // matrice opengl column major come richiesta da glMultMatrix et al
    // | 0 4  8 12 |
    // | 1 5  9 13 |
    // | 2 6 10 14 |
    // | 3 7 11 15 |
    C(0,0) ; C(0,1) ; C(0,2) ; Z(0,3);
    C(1,0) ; C(1,1) ; C(1,2) ; Z(1,3);
    C(2,0) ; C(2,1) ; C(2,2) ; Z(2,3);
    Z(3,0) ; Z(3,1) ; Z(3,2) ; O(3,3);
}



char *M34::to_code( char *buf , int len ){
#undef  M
#define M(r,c)  this->raw[r*4+c]
  snprintf(
    buf,len, 
    "%f,%f,%f,%f"
    ","
    "%f,%f,%f,%f"
    ","
    "%f,%f,%f,%f"
    ,M(0,0),M(0,1),M(0,2),M(0,3)
    ,M(1,0),M(1,1),M(1,2),M(1,3)
    ,M(2,0),M(2,1),M(2,2),M(2,3)
  );
  return buf;
}


//XYZ view_vector(){
//    static XYZ vv = {
//        globals.camera.mat.view.x ,
//        globals.camera.mat.view.y ,
//        globals.camera.mat.view.z ,
//    };
//    return vv ;
//}


void M34::m_dot_v( float *o, const float *i ){
#undef  M
#define M(r,c)  row[r].col[c]
    o[0]=M(0,0)*i[0]+M(0,1)*i[1]+M(0,2)*i[2]+M(0,3);
    o[1]=M(1,0)*i[0]+M(1,1)*i[1]+M(1,2)*i[2]+M(1,3);
    o[2]=M(2,0)*i[0]+M(2,1)*i[1]+M(2,2)*i[2]+M(2,3);
}


