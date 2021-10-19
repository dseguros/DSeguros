#include "io.h"
#include <direct.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <shlobj.h>
#include <io.h>

FILE* ethash_fopen(char const* file_name, char const* mode)
{
	FILE* f;
	return fopen_s(&f, file_name, mode) == 0 ? f : NULL;
}

int ethash_fseek(FILE* f, size_t offset, int origin)
{
	return _fseeki64(f, offset, origin);
}

char* ethash_strncat(char* dest, size_t dest_size, char const* src, size_t count)
{
	return strncat_s(dest, dest_size, src, count) == 0 ? dest : NULL;
}

