#ifndef __OPENEQ_ZON__
#define __OPENEQ_ZON__

#include "ter.hpp"

/* Big thanks to jbb on the ZON stuff! */

#pragma pack(1)

struct zon_header {
  char magic[4]; // Constant at EQGZ
  int32 version; // Constant at 2
  int32 list_len; // Length of the list to follow.
  int32 NumberOfModels;
  int32 obj_count; // Placeable object count.
  int32 unk[2]; // Unknown.
} typedef zon_header;

struct zon_placeable {
  int32 id;
  int32 loc;
  float x, y, z;
  float rx, ry, rz;
  float scale;
} typedef zon_placeable;

struct zon_object {
   int32 offset;
   int32 id;
} typedef zon_object;

#pragma pack()

class ZonLoader : public FileLoader {
public:
  ZonLoader();
  ~ZonLoader();
 
  virtual int32 Open(char *base_path, char *zone_name, Archive *archive);
  virtual int32 Close();

private:
  TERLoader terloader;
  TERLoader *model_loaders;
};

#endif
