#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include "handle_modules/handle_modules.h"
int main(int argc, char *argv[]) {
	atexit(cleanup);
    init_all();
    uintmax_t dt = 0;
    clock_t starttime, endtime;
	while (1) {
        starttime = clock();
        update((int)(dt));
        endtime = clock();
        dt = (endtime - starttime);
	}
	return 0;
}
