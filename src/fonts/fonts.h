#pragma once

#include <stdint.h>

#define FONTINFOFLAGS_ITALIC    1
#define FONTINFOFLAGS_UNDERLINE 2
#define FONTINFODLAFS_STRIKEOUT 4
#define FONTINFOFLAGS_VARWIDTH  8

namespace fabgl {

struct FontInfo {
  uint8_t  pointSize;
  uint8_t  width;   // used only for fixed width fonts (FONTINFOFLAGS_VARWIDTH = 0)
  uint8_t  height;
  uint8_t  ascent;
  uint8_t  inleading;
  uint8_t  exleading;
  uint8_t  flags;
  uint16_t weight;
  uint16_t charset;
  // when FONTINFOFLAGS_VARWIDTH = 0:
  //   data[] contains 256 items each one representing a single character
  // when FONTINFOFLAGS_VARWIDTH = 1:
  //   data[] contains 256 items each one representing a single character. First byte contains the
  //   character width. "chptr" is filled with an array of pointers to the single characters.
  uint8_t const *  data;
  uint32_t const * chptr;  // used only for variable width fonts (FONTINFOFLAGS_VARWIDTH = 1)
  uint16_t codepage;
};

} // end of namespace
