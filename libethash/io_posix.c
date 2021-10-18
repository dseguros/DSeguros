#include "io.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>

FILE* ethash_fopen(char const* file_name, char const* mode)
{
	return fopen(file_name, mode);
}

int ethash_fseek(FILE* f, size_t offset, int origin)
{
	return fseeko(f, offset, origin);
}

