#pragma once

#include <sys/stat.h>
#include <stddef.h>

int lstat(const char *path, struct stat *buffer);
ssize_t readlink(const char *path, char *buf, size_t bufsiz);
