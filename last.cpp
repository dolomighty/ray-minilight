
#include <stdio.h>
#include <globals.h>
#include <assert.h>


const char *cfg = "last.txt";




void last_save() // HEADER
{
    FILE *f = fopen(cfg,"wb");
    if(!f)return;
    char buf[256];
    fprintf(f,"LAST_CAMERA %s\n",camera.to_code(buf,sizeof(buf)));
    fprintf(f,"LAST_EXPO %f\n",expo);
    fprintf(f,"LAST_SPEED_MULT %d\n",speed_mult);
    fclose(f);
}

void last_load() // HEADER
{
    FILE *f = fopen(cfg,"rb");
    if(!f)return;
#define M(R,C) &camera.row[R].col[C]
    assert( 12 == fscanf(f," LAST_CAMERA %f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f"
        ,M(0,0),M(0,1),M(0,2),M(0,3)
        ,M(1,0),M(1,1),M(1,2),M(1,3)
        ,M(2,0),M(2,1),M(2,2),M(2,3)
    ));
#undef M
    assert( 1 == fscanf(f," LAST_EXPO %f\n", &expo ));
    assert( 1 == fscanf(f," LAST_SPEED_MULT %d\n", &speed_mult ));
    fclose(f);
}

