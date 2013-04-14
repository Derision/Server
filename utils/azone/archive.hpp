#ifndef __OPENEQ_ARCHIVE_API__
#define __OPENEQ_ARCHIVE_API__

#include "global.hpp"

#include <stdio.h>

class Archive {
public:
  Archive() {}
  virtual ~Archive() {}

  virtual int32 Open(FILE *fp) = 0;
  virtual int32 Close() = 0;

  virtual int32 GetFile(char *name, uchar **buf, int32 *len) = 0;
  virtual const char *FindExtension(const char *ext) = 0;

  char **filenames;
  int32 count;

protected:
  uchar *buffer;
  int32 buf_len;

  int32 status;

  uint32 *files;
  FILE *fp;
};

#endif
