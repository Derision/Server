#ifndef __OPENEQ_PFS__
#define __OPENEQ_PFS__

#include <stdio.h>
#include "global.hpp"
#include "archive.hpp"


class PFSLoader : public Archive {
public:
  PFSLoader();
  ~PFSLoader();

  virtual int32 Open(FILE *fp);
  virtual int32 Close();

  virtual int32 GetFile(char *name, uchar **buf, int32 *len);
  virtual const char *FindExtension(const char *ext);
};

#endif
