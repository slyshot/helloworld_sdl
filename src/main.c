//gettimeofday is for accurate time measurements.
//clock() seems unreliable given testing, and that makes sense given it's measuring CPU cycles.

#include <math.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>
#include "handle_modules/handle_modules.h"
int main(int argc, char *argv[]) {
	atexit(cleanup);
    init_all();
    uintmax_t dt = 0;
	while (1) {
        struct timeval tv;
        gettimeofday(&tv,NULL);
        long long starttime = tv.tv_sec*1000LL + tv.tv_usec/1000;
        update((int)(dt));
        gettimeofday(&tv,NULL);
        long long endtime = tv.tv_sec*1000LL + tv.tv_usec/1000;
        dt = (int)(endtime - starttime)*10;
	}
	return 0;
}
