#ifndef __OPENEQ_WLD_STRUCTS__
#define __OPENEQ_WLD_STRUCTS__

#pragma pack(1)

struct struct_wld_header {
  int32 magic;
  int32 version;
  int32 fragmentCount;
  int32 header3;
  int32 header4;
  int32 stringHashSize;
  int32 header6;
} typedef struct_wld_header;

struct struct_wld_basic_frag {
  int32 size;
  int32 id;
  int32 nameRef;
} typedef struct_wld_basic_frag;


struct struct_Data15 {
  uint32 ref, flags, fragment1;
  float trans[3], rot[3];
  float scale[3];
  uint32 fragment2, flags2;
} typedef struct_Data15;

struct struct_Data36 {
  int32 flags;
  int32 fragment1;
  int32 fragment2;
  int32 fragment3;
  int32 fragment4;
  float centerX;
  float centerY;
  float centerZ;
  int32 params2[3]; // 48
  float maxDist;
  float minX;
  float minY;
  float minZ;
  float maxX;
  float maxY;
  float maxZ; // 24
  int16 vertexCount;
  int16 texCoordsCount;
  int16 normalsCount;
  int16 colorCount;
  int16 PolygonsCount;
  int16 size6;
  int16 PolygonTexCount;
  int16 vertexTexCount;
  int16 size9;
  int16 scale; // 20
} typedef struct_Data36;

struct struct_Data10 {
  int32 flags, size1, fragment;
} typedef struct_Data10;

struct struct_Data1B {
  uint32 flags, params1;
  uint32 params3b;
  float color[3];
} typedef struct_Data1B;

struct struct_Data21 {
  float normal[3], splitdistance;
  int32 region, node[2];
} typedef struct_Data21;

struct struct_Data22 {
  int32 flags, fragment1, size1, size2, params1, size3, size4, params2, size5, size6;
} typedef struct_Data22;

struct struct_Data28 {
  uint32 flags;
  float x, y, z, rad;
} typedef struct_Data28;

typedef struct struct_Data29 {
  int32 region_count;
  int32 *region_array;
  int32 strlen;
  char *str;
  int32 region_type; // We fill this in with -1 for unknown, 1 for water, 2 for lava
} struct_Data29;


struct struct_Data30 {
  uint32 flags, params1, params2;
  float params3[2];
} typedef struct_Data30;

typedef struct BSP_Node {
  int32 node_number;
  float normal[3], splitdistance;
  int32 region;
  int32 special;
  int32 left, right;
} BSP_Node;


struct Vert {
  int16 x, y, z;
} typedef Vert;

struct TexCoordsNew {
  float tx, tz;
} typedef TexCoordsNew;

struct TexCoordsOld {
  int16 tx, tz;
} typedef TexCoordsOld;

struct VertexNormal {
  signed char nx, ny, nz;
} typedef VertexNormal;

struct VertexColor {
  char color[4];
} typedef VertexColor;

struct Poly {
  int16 flags, v1, v2, v3;
} typedef Poly;

struct TexMap {
  uint16 polycount;
  uint16 tex;
} typedef TexMap;

#pragma pack()

#endif
