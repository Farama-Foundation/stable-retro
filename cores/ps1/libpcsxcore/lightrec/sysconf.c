#include <errno.h>
#include <unistd.h>

/* Implement the sysconf() symbol which is needed by GNU Lightning */
long sysconf(int name)
{
	switch (name) {
	case _SC_PAGE_SIZE:
		return 4096;
	default:
		return -EINVAL;
	}
}
