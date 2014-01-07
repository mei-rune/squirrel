
#include <stdlib.h>
#include "squirrel_slab.h"

int main(int argc, char **argv) {
	char *p;
	size_t 	pool_size = 4096000;  //4M 
	shttp_slab_stat_t stat;
	char 	*space;
	space = (char *)malloc(pool_size);

	shttp_slab_pool_t *sp;
	sp = (shttp_slab_pool_t*) space;

	sp->addr = space;
	sp->min_shift = 3;
	sp->end = space + pool_size;

	/*
	 * init
	 */
	shttp_slab_init(sp);

	/*
	 * alloc
	 */
	p = shttp_slab_alloc(sp, 128);

	/*
	 * show stat
	 */
	shttp_slab_stat(sp, &stat);

	/*
	 * free
	 */
	shttp_slab_free(sp, p);

	free(space);

	return 0;
}
