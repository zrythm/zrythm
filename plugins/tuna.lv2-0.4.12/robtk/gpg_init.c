#ifdef WITH_SIGNATURE // gpg sign tested releases
#include "lv2_rgext.h"
#ifdef _WIN32
# include <windows.h>
#endif
#include "gp3.h"
#include <sys/types.h>
#include <sys/stat.h>

/* test if file exists and is a regular file - returns 1 if ok */
static int testfile (const char *filename) {
	struct stat s;
	if (!filename || strlen(filename) < 1) return 0;
	int result= stat(filename, &s);
	if (result != 0) return 0; /* stat() failed */
	if (S_ISREG(s.st_mode)) return 1; /* is a regular file - ok */
	return 0;
}

struct license_info {
	char name[64];
	char store[128];
};
#endif
