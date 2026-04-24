#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "port.h"

typedef struct {
	ULONG ReparseTag;
	USHORT ReparseDataLength;
	USHORT Reserved;
	union {
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			ULONG Flags;
			WCHAR PathBuffer[1];
		} SymbolicLinkReparseBuffer;
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			WCHAR PathBuffer[1];
		} MountPointReparseBuffer;
		struct {
			UCHAR DataBuffer[1];
		} GenericReparseBuffer;
	};
} W32_REPARSE_DATA_BUFFER;

static wchar_t *w32_utf8_to_utf16(const char *path) {
	int length;
	wchar_t *wide_path;

	if (path == NULL) {
		errno = EINVAL;
		return NULL;
	}

	length = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
	if (length == 0) {
		errno = EINVAL;
		return NULL;
	}

	wide_path = (wchar_t *)malloc((size_t)length * sizeof(*wide_path));
	if (wide_path == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	if (MultiByteToWideChar(CP_ACP, 0, path, -1, wide_path, length) == 0) {
		free(wide_path);
		errno = EINVAL;
		return NULL;
	}

	return wide_path;
}

static HANDLE w32_open_reparse_point(const char *path, DWORD desired_access) {
	wchar_t *wide_path;
	DWORD flags = FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS;
	HANDLE handle;

	wide_path = w32_utf8_to_utf16(path);
	if (wide_path == NULL)
		return INVALID_HANDLE_VALUE;

	handle =
	    CreateFileW(wide_path, desired_access,
	                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	                NULL, OPEN_EXISTING, flags, NULL);
	free(wide_path);

	if (handle == INVALID_HANDLE_VALUE) {
		errno = ENOENT;
		return INVALID_HANDLE_VALUE;
	}

	return handle;
}

static int w32_path_is_reparse_link(const char *path) {
	wchar_t *wide_path;
	DWORD attrs;

	wide_path = w32_utf8_to_utf16(path);
	if (wide_path == NULL)
		return 0;

	attrs = GetFileAttributesW(wide_path);
	free(wide_path);
	if (attrs == INVALID_FILE_ATTRIBUTES)
		return 0;

	return (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

int lstat(const char *path, struct stat *buffer) {
	if (stat(path, buffer) != 0)
		return -1;

	if (w32_path_is_reparse_link(path)) {
		buffer->st_mode &= ~_S_IFMT;
		buffer->st_mode |= S_IFLNK;
	}

	return 0;
}

ssize_t readlink(const char *path, char *buf, size_t bufsiz) {
	union {
		W32_REPARSE_DATA_BUFFER buffer;
		char raw[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
	} reparse_data;
	DWORD bytes_returned;
	HANDLE handle;
	WCHAR *target;
	USHORT target_length;
	char target_utf8[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
	int utf8_length;
	size_t copy_length;

	if (buf == NULL || bufsiz == 0) {
		errno = EINVAL;
		return -1;
	}

	handle = w32_open_reparse_point(path, 0);
	if (handle == INVALID_HANDLE_VALUE)
		return -1;

	if (!DeviceIoControl(handle, FSCTL_GET_REPARSE_POINT, NULL, 0,
	                     &reparse_data, sizeof(reparse_data),
	                     &bytes_returned, NULL)) {
		CloseHandle(handle);
		errno = EINVAL;
		return -1;
	}
	CloseHandle(handle);

	switch (reparse_data.buffer.ReparseTag) {
	case IO_REPARSE_TAG_SYMLINK:
		target = (WCHAR *)((char *)reparse_data.buffer
		                       .SymbolicLinkReparseBuffer.PathBuffer +
		                   reparse_data.buffer.SymbolicLinkReparseBuffer
		                       .PrintNameOffset);
		target_length = reparse_data.buffer.SymbolicLinkReparseBuffer
		                    .PrintNameLength /
		                sizeof(WCHAR);
		break;
	case IO_REPARSE_TAG_MOUNT_POINT:
		target = (WCHAR *)((char *)reparse_data.buffer
		                       .MountPointReparseBuffer.PathBuffer +
		                   reparse_data.buffer.MountPointReparseBuffer
		                       .PrintNameOffset);
		target_length = reparse_data.buffer.MountPointReparseBuffer
		                    .PrintNameLength /
		                sizeof(WCHAR);
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	utf8_length =
	    WideCharToMultiByte(CP_ACP, 0, target, target_length, target_utf8,
	                        (int)sizeof(target_utf8) - 1, NULL, NULL);
	if (utf8_length <= 0) {
		errno = EINVAL;
		return -1;
	}
	target_utf8[utf8_length] = '\0';

	copy_length = (size_t)utf8_length;
	if (copy_length >= bufsiz)
		copy_length = bufsiz - 1;

	memcpy(buf, target_utf8, copy_length);
	buf[copy_length] = '\0';
	return (ssize_t)copy_length;
}
