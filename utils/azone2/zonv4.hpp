#ifndef __OPENEQ_ZONV4__
#define __OPENEQ_ZONV4__

#include "dat.hpp"
#include "ter.hpp"

/* Big thanks to jbb on the ZON stuff! */

#pragma pack(1)

struct zonv4_header {
  char magic[4]; // Constant at EQGZ
} typedef zonv4_header;

struct zonv4_placeable {
  int32 id;
  int32 loc;
  float x, y, z;
  float rx, ry, rz;
  float scale;
} typedef zonv4_placeable;

struct zonv4_object {
   int32 offset;
   int32 id;
} typedef zonv4_object;

#pragma pack()

class Zonv4Loader : public FileLoader {
public:
  Zonv4Loader(bool iGenerateQuads = false);
  ~Zonv4Loader();
 
  virtual int32 Open(char *base_path, char *zone_name, Archive *archive);
  virtual int32 Close();

private:
  DATLoader datloader;
  TERLoader *model_loaders;
  bool GenerateQuads;
};

#endif
