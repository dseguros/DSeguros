/*
  This file is part of ethash.

  ethash is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ethash is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with ethash.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file io.h
 * @author Lefteris Karapetsas <lefteris@ethdev.com>
 * @date 2015
 */
#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#ifdef __cplusplus
#define __STDC_FORMAT_MACROS 1
#endif
#include <inttypes.h>
#include "endian.h"
#include "ethash.h"

#ifdef __cplusplus
extern "C" {
#endif
// Maximum size for mutable part of DAG file name
// 6 is for "full-R", the suffix of the filename
// 10 is for maximum number of digits of a uint32_t (for REVISION)
// 1 is for - and 16 is for the first 16 hex digits for first 8 bytes of
// the seedhash and last 1 is for the null terminating character
// Reference: https://github.com/ethereum/wiki/wiki/Ethash-DAG
#define DAG_MUTABLE_NAME_MAX_SIZE (6 + 10 + 1 + 16 + 1)
/// Possible return values of @see ethash_io_prepare
enum ethash_io_rc {
	ETHASH_IO_FAIL = 0,           ///< There has been an IO failure
	ETHASH_IO_MEMO_SIZE_MISMATCH, ///< DAG with revision/hash match, but file size was wrong.
	ETHASH_IO_MEMO_MISMATCH,      ///< The DAG file did not exist or there was revision/hash mismatch
	ETHASH_IO_MEMO_MATCH,         ///< DAG file existed and revision/hash matched. No need to do anything
};

// small hack for windows. I don't feel I should use va_args and forward just
// to have this one function properly cross-platform abstracted
#if defined(_WIN32) && !defined(__GNUC__)
#define snprintf(...) sprintf_s(__VA_ARGS__)
#endif

/**
 * Logs a critical error in important parts of ethash. Should mostly help
 * figure out what kind of problem (I/O, memory e.t.c.) causes a NULL
 * ethash_full_t
 */
#ifdef ETHASH_PRINT_CRITICAL_OUTPUT
#define ETHASH_CRITICAL(...)							\
	do													\
	{													\
		printf("ETHASH CRITICAL ERROR: "__VA_ARGS__);	\
		printf("\n");									\
		fflush(stdout);									\
	} while (0)
#else
#define ETHASH_CRITICAL(...)          
#endif

/**
 * Prepares io for ethash
 *
 * Create the DAG directory and the DAG file if they don't exist.
 *
 * @param[in] dirname        A null terminated c-string of the path of the ethash
 *                           data directory. If it does not exist it's created.
 * @param[in] seedhash       The seedhash of the current block number, used in the
 *                           naming of the file as can be seen from the spec at:
 *                           https://github.com/ethereum/wiki/wiki/Ethash-DAG
 * @param[out] output_file   If there was no failure then this will point to an open
 *                           file descriptor. User is responsible for closing it.
 *                           In the case of memo match then the file is open on read
 *                           mode, while on the case of mismatch a new file is created
 *                           on write mode
 * @param[in] file_size      The size that the DAG file should have on disk
 * @param[out] force_create  If true then there is no check to see if the file
 *                           already exists
 * @return                   For possible return values @see enum ethash_io_rc
 */
enum ethash_io_rc ethash_io_prepare(
	char const* dirname,
	ethash_h256_t const seedhash,
	FILE** output_file,
	uint64_t file_size,
	bool force_create
);

/**
 * An fopen wrapper for no-warnings crossplatform fopen.
 *
 * Msvc compiler considers fopen to be insecure and suggests to use their
 * alternative. This is a wrapper for this alternative. Another way is to
 * #define _CRT_SECURE_NO_WARNINGS, but disabling all security warnings does
 * not sound like a good idea.
 *
 * @param file_name        The path to the file to open
 * @param mode             Opening mode. Check fopen()
 * @return                 The FILE* or NULL in failure
 */
FILE* ethash_fopen(char const* file_name, char const* mode);






