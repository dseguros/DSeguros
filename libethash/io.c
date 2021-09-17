#include "io.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

enum ethash_io_rc ethash_io_prepare(
	char const* dirname,
	ethash_h256_t const seedhash,
	FILE** output_file,
	uint64_t file_size,
	bool force_create
)
{
	char mutable_name[DAG_MUTABLE_NAME_MAX_SIZE];
	enum ethash_io_rc ret = ETHASH_IO_FAIL;
	// reset errno before io calls
	errno = 0;

}

