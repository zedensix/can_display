// The screensaver's logos after the 86's: the parts on the car. They're
// their makers' trademarks, so they stay out of git: `mise run logos
// <folder>` writes them to brands_data.h, which this includes when it
// exists. Without it (a fresh clone, the CI), the 86's logo alone.

#include "ui_internal.h"

// BRANDS_NONE leaves them out anyway: the UI pictures in git stay the same
// whoever renders them.
#if __has_include("brands_data.h") && !defined(BRANDS_NONE)
#include "brands_data.h"
#else
// ISO C has no empty array: one entry, which brand_count leaves out.
const brand_t brands[] = {{0}};
const int brand_count = 0;
#endif
