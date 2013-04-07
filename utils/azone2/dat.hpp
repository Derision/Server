#ifndef __OPENEQ_DAT__
#define __OPENEQ_DAT__

#include "global.hpp"
#include "file_loader.hpp"

#pragma pack(1)

struct dat_header {
  uint32 version, list_len, mat_count, vert_count, tri_count;
} typedef dat_header;

struct dat_vertex {
  float x, y, z;
  float i, j, k;
  float u, v;
} typedef dat_vertex;

struct dat_vertexV3 {
  float x, y, z;
  float i, j, k;
  int32 unk1;
  float unk2, unk3;
  float u, v;
} typedef dat_vertexV3;

struct dat_triangle {
  int32 v1, v2, v3;
  int32 group;
  int32 unk;
} typedef dat_triangle;

struct dat_object {
  int32 index;
  int32 name_offset, another_name_offset;
  int32 property_count;
} typedef dat_object;

struct dat_property {
  int32 name_offset, type, value;
} typedef dat_property;

struct dat_material {
  char *name;
  char *basetex;
  char var_count;
  char **var_names;
  char **var_vals;
} typedef dat_material;

#pragma pack()

class DATLoader : public FileLoader {
public:
  DATLoader();
  ~DATLoader();
  virtual int32 Open(char *base_path, char *zone_name, Archive *archive);
  virtual int32 Close();
  void ReturnQuads();
private:
  bool GenerateQuads;
};

#endif
