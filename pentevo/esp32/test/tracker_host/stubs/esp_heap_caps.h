#pragma once
#include <stdlib.h>
#define heap_caps_free(p) free(p)
#define heap_caps_malloc(s, c) malloc(s)
#define MALLOC_CAP_SPIRAM 0
#define MALLOC_CAP_8BIT 0
