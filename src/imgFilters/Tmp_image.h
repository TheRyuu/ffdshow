#ifndef _TMP_IMAGE_H
#define _TMP_IMAGE_H

#include "yadif/mp_image.h"
#include "TffPict.h"

class Tmp_image
{
 private:
  mp_image_t *mpi;

 public:
  Tmp_image(TffPict &pict, int full, const unsigned char *src[4]);
  ~Tmp_image();
  operator mp_image_t*() const {return mpi;}
};

#endif // _TMP_IMAGE_H