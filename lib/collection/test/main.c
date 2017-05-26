#include <stdio.h>

#define RUN(testfunc)						\
{											\
	extern int testfunc(int, char**);		\
	int exit_value = testfunc(argc, argv);	\
	if(exit_value) {						\
		puts(#testfunc" failed");			\
		return exit_value;					\
	}										\
}

int main(int argc, char** argv) {
	RUN(arraylist_testmain)
	RUN(arrayqueue_testmain)
	RUN(cache_testmain)
	RUN(hashmap_testmain)
	RUN(hashset_testmain)
	RUN(linkedlist_testmain)
	return 0;
}
