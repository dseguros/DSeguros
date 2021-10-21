
#include <io.h>
#include <windows.h>
#include "mmap.h"

#ifdef __USE_FILE_OFFSET64
# define DWORD_HI(x) (x >> 32)
# define DWORD_LO(x) ((x) & 0xffffffff)
#else
# define DWORD_HI(x) (0)
# define DWORD_LO(x) (x)
#endif

#undef DWORD_HI
#undef DWORD_LO
