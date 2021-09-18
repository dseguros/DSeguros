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

 	// assert directory exists
	if (!ethash_mkdir(dirname)) {
		ETHASH_CRITICAL("Could not create the ethash directory");
		goto end;
	}

	ethash_io_mutable_name(ETHASH_REVISION, &seedhash, mutable_name);
	char* tmpfile = ethash_io_create_filename(dirname, mutable_name, strlen(mutable_name));
	if (!tmpfile) {
		ETHASH_CRITICAL("Could not create the full DAG pathname");
		goto end;
	}

	FILE *f;
	if (!force_create) {
		// try to open the file
		f = ethash_fopen(tmpfile, "rb+");
		if (f) {
			size_t found_size;
			if (!ethash_file_size(f, &found_size)) {
				fclose(f);
				ETHASH_CRITICAL("Could not query size of DAG file: \"%s\"", tmpfile);
				goto free_memo;
			}
			if (file_size != found_size - ETHASH_DAG_MAGIC_NUM_SIZE) {
				fclose(f);
				ret = ETHASH_IO_MEMO_SIZE_MISMATCH;
				goto free_memo;
			}
			// compare the magic number, no need to care about endianess since it's local
			uint64_t magic_num;
			if (fread(&magic_num, ETHASH_DAG_MAGIC_NUM_SIZE, 1, f) != 1) {
				// I/O error
				fclose(f);
				ETHASH_CRITICAL("Could not read from DAG file: \"%s\"", tmpfile);
				ret = ETHASH_IO_MEMO_SIZE_MISMATCH;
				goto free_memo;
			}
			if (magic_num != ETHASH_DAG_MAGIC_NUM) {
				fclose(f);
				ret = ETHASH_IO_MEMO_SIZE_MISMATCH;
				goto free_memo;
			}
			ret = ETHASH_IO_MEMO_MATCH;
			goto set_file;
		}
	}
	
	// file does not exist, will need to be created
	f = ethash_fopen(tmpfile, "wb+");
	if (!f) {
		ETHASH_CRITICAL("Could not create DAG file: \"%s\"", tmpfile);
		goto free_memo;
	}
	// make sure it's of the proper size
	if (ethash_fseek(f, file_size + ETHASH_DAG_MAGIC_NUM_SIZE - 1, SEEK_SET) != 0) {
		fclose(f);
		ETHASH_CRITICAL("Could not seek to the end of DAG file: \"%s\". Insufficient space?", tmpfile);
		goto free_memo;
	}
}

