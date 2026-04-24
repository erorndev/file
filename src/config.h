#pragma once
#define VERSION "5.45"
#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_STRTOULL 1
#define HAVE_UNISTD_H 1

#undef HAVE_LSTAT
#undef HAVE_READLINK
#undef HAVE_SYS_PARAM_H
#undef HAVE_SYS_TIME_H
#undef HAVE_MMAP

#include "port.h"