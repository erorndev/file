#pragma once

#ifdef __clang__
#define __attribute__(x) __attribute__(x)
#endif

#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>
#include <BaseTsd.h>
#include <errno.h>

#ifndef S_IFIFO
#define S_IFIFO _S_IFIFO
#endif
#ifndef S_IFBLK
#define S_IFBLK 0x6000
#endif
#ifndef S_IFLNK
#define S_IFLNK 0xA000
#endif
#ifndef S_IFSOCK
#define S_IFSOCK 0xC000
#endif

#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#define S_ISFIFO(m) (((m) & _S_IFMT) == S_IFIFO)
#define S_ISLNK(m) (((m) & _S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m) & _S_IFMT) == S_IFSOCK)
#define S_ISCHR(m) (((m) & _S_IFMT) == _S_IFCHR)
#define S_ISBLK(m) (((m) & _S_IFMT) == S_IFBLK)
#endif

typedef SSIZE_T ssize_t;
#include "symlink.h"

typedef unsigned short mode_t;
typedef int pid_t;

extern char *optarg;
extern int optind, opterr, optopt;

#define pipe(fds) _pipe((fds), 4096, _O_BINARY)

#ifndef O_RDONLY
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_RDWR _O_RDWR
#define O_BINARY _O_BINARY
#define O_CREAT _O_CREAT
#define O_TRUNC _O_TRUNC
#endif
