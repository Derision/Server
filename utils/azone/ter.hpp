#ifndef __OPENEQ_TER__
#define __OPENEQ_TER__

#include "global.hpp"
#include "file_loader.hpp"

#pragma pack(1)

struct ter_header {
  char magic[4];
  uint32 version, list_len, mat_count, vert_count, tri_count;
} typedef ter_header;

struct ter_vertex {
  float x, y, z;
  float i, j, k;
  float u, v;
} typedef ter_vertex;

struct ter_vertexV3 {
  float x, y, z;
  float i, j, k;
  int32 unk1;
  float unk2, unk3;
  float u, v;
} typedef ter_vertexV3;

struct ter_triangle {
  int32 v1, v2, v3;
  int32 group;
  int32 unk;
} typedef ter_triangle;

struct ter_object {
  int32 index;
  int32 name_offset, another_name_offset;
  int32 property_count;
} typedef ter_object;

struct ter_property {
  int32 name_offset, type, value;
} typedef ter_property;

struct material {
  char *name;
  char *basetex;
  char var_count;
  char **var_names;
  char **var_vals;
} typedef material;

#pragma pack()

class TERLoader : public FileLoader {
public:
  TERLoader();
  ~TERLoader();
  virtual int32 Open(char *base_path, char *zone_name, Archive *archive);
  virtual int32 Close();
};

#endif
