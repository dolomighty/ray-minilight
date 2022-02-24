

//
// importer per wavefront obj
//



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <unordered_map>





// non voglio esportare strutture
// quindi uso solo 

// HEADERBEG
#define TRI_CB_DEF(NAME) void NAME( float a[], float b[], float c[], float d[], float e[] )
// HEADEREND




union FLAT3 {
    float flat[3];
    struct { float r,g,b; } rgb;
    struct { float x,y,z; } xyz;
};


struct MTL {
    union FLAT3 diff;
    union FLAT3 emit;
};

static std::unordered_map<std::string,struct MTL> mtl_list;
// le liste di vertici/uv/ecc sono globali nel file
// inoltre son 1-based, gestito
static std::vector<union FLAT3> vert_list;

static TRI_CB_DEF((*global_tri_cb));
static float global_emit_gain = 1000;
static std::string global_obj_path;



static void obj_mtl_parser( FILE *f );  // fwd

static void import_mtl( std::string &mtllib_path ){
    FILE *f;
    assert( f = fopen( mtllib_path.c_str(), "rb" ));
    obj_mtl_parser(f);   // usiamo lo stesso parser, i tokens son univoci (ci han pensato)
    fclose(f);
}


static char *tok( char *line = 0 ){
    // giusto per wrappare lo split sep 
    return strtok(line," \n");
}



static void out_raw_tri( struct MTL &mtl ){
    // ogni token letto descrive un vertice
    // "vert_id/uv_id/norm_id"
    // non sappiamo in anticipop quanti vertici ci sono, ma sono minimo 3
    // vogliamo cmq esportare triangoli

    struct VUN {
        int v,u,n;
    };

    struct VUN vun[3];
    int i=0;

    while(1){
        char *t = tok();
        if(!t)break;
        assert( 3 == sscanf( t, "%d/%d/%d", &vun[i].v, &vun[i].u, &vun[i].n ));
        if(i<2){ ++i; continue; }
        global_tri_cb(
            vert_list[vun[0].v].flat,
            vert_list[vun[1].v].flat,
            vert_list[vun[2].v].flat,
            mtl.diff.flat,
            mtl.emit.flat
        );
        vun[1] = vun[2];
    }
}



static void obj_mtl_parser( FILE *f ){

    std::string usemtl;

    // usate sono nel parsing dell'mtl
    std::string mtl_name;
    struct MTL mtl;


    bool store_mtl = false;

    while(1){
        // visto che obj non è freeform
        // processiamo per linea
        char line[256];
        if(!fgets( line, sizeof(line), f ))break;

        // brucio i commenti
        char *c = strchr(line,'#');
        if(c)*c=0;  // termino la stringa

        // parse
        c = tok(line);
        if(!c)continue;

        std::string item = c;

        if( item == "newmtl" ){
            if(store_mtl) mtl_list[mtl_name]=mtl;
            assert(c=tok());
            mtl_name = c;
            store_mtl=true;
            continue;
        }

        if( item == "mtllib" ){
            // il path del .mtl è relativo al .obj
            // quindi giochiamo con le stringhe
            std::string mtllib_path;
            std::string::size_type last_slash = global_obj_path.rfind("/");
            if( last_slash != std::string::npos ) mtllib_path = global_obj_path.substr(0,last_slash+1);
            assert(c=tok());
            mtllib_path += c;
            import_mtl(mtllib_path);
            continue;
        }

        if( item == "usemtl" ){
            assert(c=tok());
            usemtl = c;
            continue;
        }

        if( item == "Kd" ){
            mtl.diff.rgb.r = atof(tok());
            mtl.diff.rgb.g = atof(tok());
            mtl.diff.rgb.b = atof(tok());
            continue;
        }

        if( item == "Ke" ){
            mtl.emit.rgb.r = atof(tok()) * global_emit_gain;
            mtl.emit.rgb.g = atof(tok()) * global_emit_gain;
            mtl.emit.rgb.b = atof(tok()) * global_emit_gain;
            continue;
        }

        if( item == "vt" ){
            float u = atof(tok());
            float v = atof(tok());
            continue;
        }

        if( item == "vn" ){
            float x = atof(tok());
            float y = atof(tok());
            float z = atof(tok());
            continue;
        }

        if( item == "v" ){
            union FLAT3 xyz;
            xyz.xyz.x = atof(tok());
            xyz.xyz.y = atof(tok());
            xyz.xyz.z = atof(tok());
            vert_list.push_back(xyz);
            continue;
        }

        if( item == "o" ){
            assert(c=tok());
            std::string obj_name = c;
            // ripartiamo con una lista di vertici
            // obj è 1-based, inseriamo un dummy vert per non far aritmetica dopo
            continue;
        }

        if( item == "f" ){
            out_raw_tri( mtl_list[usemtl]);
            continue;
        }

//        std::cout << "* item " << item << " skippato" << std::endl;
    }

    if(store_mtl) mtl_list[mtl_name]=mtl;
}



void obj_import( const char *path, TRI_CB_DEF((*tri_cb)), float emit_gain=1000 ){    // HEADER
    assert( path );
    assert( tri_cb );

    global_obj_path  = path;
    global_emit_gain = emit_gain;
    global_tri_cb    = tri_cb;

    FILE *f;
    assert( f = fopen( path, "rb" ));

    // nel .obj gli indici son tutti 1-based, quindi aggiungo un dummy
    // per evitare di fare brutta aritmetica dopo
    union FLAT3 dummy;
    vert_list.push_back(dummy);

    obj_mtl_parser(f);

    mtl_list.clear();
    vert_list.clear();

    fclose(f);
}


