#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include <util/cache.h>

static FILE *testlog; /* Test Log File */
static FILE *testresult; /* Test Result File */

static void stage_log(int stage, char* status);
static void stage_result(int stage, char* operation, bool opresult, Cache* cache);

int cache_testmain(int argc, const char *argv[]) {
	/* 0: Test Data/Result Initialization; Test Environment peekup */
	testlog = fopen("cachetest_log.txt", "a");
	testresult = fopen("cachetest_result.txt", "a");

	char *string0 = "tester0";
	char *string1 = "tester1";
	char *string2 = "tester2";
	char *string3 = "tester3";
	char *string4 = "tester4";
	char *string5 = "tester5";
	char *string6 = "tester6";
	char *string7 = "tester7";
	char *string8 = "tester8";
	char *string9 = "tester9";

	/* 1: No Data Structure Memory Test */
	stage_log(1, "START");

	Cache *cache = cache_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);

	if(cache != NULL) stage_result(1, "create", true, cache);
	else stage_result(1, "create", false, cache);

	stage_log(1, "END");

	/* 2: No Node Memory Test */
	stage_log(2, "START");

	/* Test Data peekup */
	cache = cache_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);

	bool stage2_result = cache->put(cache, string0, string0);
	if(stage2_result == true) stage_result(2, "get", true, cache);
	else stage_result(2, "get", false, cache);

	cache_destroy(cache);
	stage_log(2, "END");

	/* 3: Between Node Memory: NULL Data */
	stage_log(3, "START");

	/* Test Data peekup */
	cache = cache_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);

	/* Test Procedure */

	void* stage31_result = cache->get(cache, (void *)NULL);
	if(stage31_result != NULL) stage_result(3, "get", true, cache);
	else stage_result(3, "get", false, cache);

	bool stage32_result = cache->put(cache, (void *)NULL, (void *)NULL);
	if(stage32_result == true) stage_result(3, "add", true, cache);
	else stage_result(3, "add", false, cache);

	bool stage33_result = cache->remove(cache, (void*)NULL);
	if(stage33_result == true) stage_result(3, "remove", true, cache);
	else stage_result(3, "remove", false, cache);

	cache_destroy(cache);
	stage_log(3, "END");

	/* 4: Between Node Memory: No Duplicated Data */
	stage_log(4, "START");

	/* Test Data removeup */
	cache = cache_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	cache->put(cache, string0, string0);
	cache->put(cache, string1, string1);
	cache->put(cache, string2, string2);
	cache->put(cache, string3, string3);
	cache->put(cache, string4, string4);

	/* Test Procedure */
	void* stage41_result = cache->get(cache, string4);
	if(stage41_result != NULL) stage_result(4, "get", true, cache);
	else stage_result(4, "get", false, cache);

	bool stage42_result = cache->put(cache, string1, string5 );
	if(stage42_result == true) stage_result(4, "put", true, cache);
	else stage_result(4, "put", false, cache);

	bool stage43_result = cache->remove(cache, string5);
	if(stage43_result == true) stage_result(4, "remove", true, cache);
	else stage_result(4, "remove", false, cache);

	/* abnormal case */
	void* stage44_result = cache->get(cache, string9);
	if(stage44_result != NULL) stage_result(4, "abnormal get", true, cache);
	else stage_result(4, "abnormal get", false, cache);

	bool stage45_result = cache->remove(cache, string9);
	if(stage45_result == true) stage_result(4, "abnormal remove", true, cache);
	else stage_result(4, "abnormal remove", false, cache);


	cache_destroy(cache);
	stage_log(4, "END");

	/* 5: Between Node Memory: Duplicated Data */
	stage_log(5, "START");

	/* Test Data removeup */
	cache = cache_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	cache->put(cache, string0, string0);
	cache->put(cache, string1, string1);
	cache->put(cache, string2, string2);
	cache->put(cache, string3, string3);
	cache->put(cache, string4, string4);

	/* Test Procedure */
	void* stage51_result = cache->get(cache, string0);
	if(stage51_result != NULL) stage_result(5, "get", true, cache);
	else stage_result(5, "get", false, cache);

	bool stage52_result = cache->put(cache, string1, string1);
	if(stage52_result == true) stage_result(5, "put", true, cache);
	else stage_result(5, "put", false, cache);

	bool stage53_result = cache->remove(cache, string1);
	if(stage53_result == true) stage_result(5, "remove", true, cache);
	else stage_result(5, "remove", false, cache);

	cache_destroy(cache);
	stage_log(5, "END");

	/* 6: Max Node Memory: NULL Data */
	stage_log(6, "START");

	/* Test Data removeup */
	cache = cache_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	cache->put(cache, string0, string0);
	cache->put(cache, string1, string1);
	cache->put(cache, string2, string2);
	cache->put(cache, string3, string3);
	cache->put(cache, string4, string4);
	cache->put(cache, string5, string5);
	cache->put(cache, string6, string6);
	cache->put(cache, string7, string7);
	cache->put(cache, string8, string8);
	cache->put(cache, string9, string9);

	/* Test Procedure */

	void* stage61_result = cache->get(cache, (void *)NULL);
	if(stage61_result != NULL) stage_result(6, "get", true, cache);
	else stage_result(6, "get", false, cache);

	bool stage62_result = cache->put(cache, (void *)NULL, (void *)NULL);
	if(stage62_result == true) stage_result(6, "put", true, cache);
	else stage_result(6, "put", false, cache);

	bool stage63_result = cache->remove(cache, (void *)NULL);
	if(stage63_result == true) stage_result(6, "remove", true, cache);
	else stage_result(6, "remove", false, cache);

	cache_destroy(cache);
	stage_log(6, "END");

	/* 7: Max Node Memory: No Duplicated Data */
	stage_log(7, "START");

	/* Test Data removeup */
	cache = cache_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	cache->put(cache, string0, string0);
	cache->put(cache, string1, string1);
	cache->put(cache, string2, string2);
	cache->put(cache, string3, string3);
	cache->put(cache, string4, string4);
	cache->put(cache, string5, string5);
	cache->put(cache, string6, string6);
	cache->put(cache, string7, string7);
	cache->put(cache, string8, string8);
	cache->put(cache, string9, string9);

	/* Test Procedure */
	void* stage71_result = cache->get(cache, string1);
	if(stage71_result != NULL) stage_result(7, "get", true, cache);
	else stage_result(7, "get", false, cache);

	bool stage72_result = cache->put(cache, string1, string1);
	if(stage72_result == true) stage_result(7, "put", true, cache);
	else stage_result(7, "put", false, cache);

	bool stage73_result = cache->remove(cache, string1);
	if(stage73_result == true) stage_result(7, "remove", true, cache);
	else stage_result(7, "remove", false, cache);

	cache_destroy(cache);
	stage_log(7, "END");

	/* 8: Max Node Memory: Duplicated Data */
	stage_log(8, "START");

	/* Test Data removeup */
	cache = cache_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	cache->put(cache, string0, string0);
	cache->put(cache, string1, string1);
	cache->put(cache, string2, string2);
	cache->put(cache, string3, string3);
	cache->put(cache, string4, string4);
	cache->put(cache, string5, string0);
	cache->put(cache, string6, string1);
	cache->put(cache, string7, string2);
	cache->put(cache, string8, string3);
	cache->put(cache, string9, string4);

	/* Test Procedure */
	void* stage81_result = cache->get(cache, string1);
	if(stage81_result != NULL) stage_result(8, "get", true, cache);
	else stage_result(8, "get", false, cache);

	bool stage82_result = cache->put(cache, string1, string1);
	if(stage82_result == true) stage_result(8, "put", true, cache);
	else stage_result(8, "put", false, cache);

	bool stage83_result = cache->remove(cache, string1);
	if(stage83_result == true) stage_result(8, "remove", true, cache);
	else stage_result(8, "remove", false, cache);

	cache_destroy(cache);
	stage_log(8, "END");

	/* 9: Test Result Store */
	fclose(testlog);
	fclose(testresult);

	return 0;
}

static void stage_log(int stage, char* status) {
	time_t current_time;
	char* c_time_string;

	current_time = time(NULL);
	c_time_string = ctime(&current_time);

	fprintf(testlog, "State %d %s: %s", stage, status, c_time_string);
}


static void stage_result(int stage, char* operation, bool opresult, Cache* cache) {
	time_t current_time;
	char* c_time_string;

	current_time = time(NULL);
	c_time_string = ctime(&current_time);

	if(opresult == true)
		fprintf(testresult, "(stage %d) %s is success.: %s", stage, operation, c_time_string);
	else
		fprintf(testresult, "(stage %d) %s is failed.: %s", stage, operation, c_time_string);

	/* cache content */
	fprintf(testresult, "cache content\n");
	Iterator* iter;
	void* element;
	for_each(element, cache) {
		fprintf(testresult, "%s ", (char *)element);
	}
	fprintf(testresult, "\n");
}
