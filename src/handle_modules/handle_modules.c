#include <stddef.h>
#include "handle_modules/module_lists.h"
void init_all(void) {
	for (int i=0;init_modules[i];
		init_modules[i++]()
	);
}
void cleanup(void) {
    for (int i=0;cleanup_modules[i];
    	cleanup_modules[i++]()
    );
}
void update(int dt) {
    for (int i=0;update_modules[i];
    	update_modules[i++](dt)
    );
}
