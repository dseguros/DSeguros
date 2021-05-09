/*
   base64.cpp and base64.h

   Copyright (C) 2004-2008 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
	  claim that you wrote the original source code. If you use this source code
	  in a product, an acknowledgment in the product documentation would be
	  appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
	  misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch
*/
/// Adapted from code found on http://stackoverflow.com/questions/180947/base64-decode-snippet-in-c
/// Originally by René Nyffenegger, modified by some other guy and then devified by Gav Wood.

#include "Base64.h"

using namespace std;
using namespace dev;

static inline bool is_base64(byte c)
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}


static inline byte find_base64_char_index(byte c)
{
	if ('A' <= c && c <= 'Z') return c - 'A';
	else if ('a' <= c && c <= 'z') return c - 'a' + 1 + find_base64_char_index('Z');
	else if ('0' <= c && c <= '9') return c - '0' + 1 + find_base64_char_index('z');
	else if (c == '+') return 1 + find_base64_char_index('9');
	else if (c == '/') return 1 + find_base64_char_index('+');
	else return 1 + find_base64_char_index('/');
}
