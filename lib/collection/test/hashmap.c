#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include <util/hashmap.h>

static FILE *testlog; /* Test Log File */
static FILE *testresult; /* Test Result File */

static void stage_log(int stage, char* status);
static void stage_result(int stage, char* operation, bool opresult, HashMap* map);

int hashmap_testmain(int argc, const char *argv[]) {
	/* 0: Test Data/Result Initialization; Test Environment peekup */
	testlog = fopen("hashmaptest_log.txt", "a");
	testresult = fopen("hashmaptest_result.txt", "a");

	int ID0 = 0;
	int ID1 = 1;
	int ID2 = 2;
	int ID3 = 3;
	int ID4 = 4;
	int ID5 = 5;
	int ID6 = 6;
	int ID7 = 7;
	int ID8 = 8;
	int ID9 = 9;

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

	HashMap *map = hashmap_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);

	if(map != NULL) stage_result(1, "create", true, map);
	else stage_result(1, "create", false, map);

	stage_log(1, "END");

	/* 2: No Node Memory Test */
	stage_log(2, "START");

	/* Test Data Setup */
	map = hashmap_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);

	bool stage2_result = map->put(map, string0, string0);
	if(stage2_result == true) stage_result(2, "put", true, map);
	else stage_result(2, "put", false, map);

	hashmap_destroy(map);
	stage_log(2, "END");

	/* 3: Between Node Memory: NULL Data */
	stage_log(3, "START");

	/* Test Data Setup */
	map = hashmap_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);

	/* Test Procedure */

	void* stage31_result = map->get(map, (void *)NULL);
	if(stage31_result != NULL) stage_result(3, "get", true, map);
	else stage_result(3, "get", false, map);

	bool stage32_result = map->put(map, (void *)NULL, (void *)NULL);
	if(stage32_result == true) stage_result(3, "put", true, map);
	else stage_result(3, "put", false, map);

	void* stage33_result = map->remove(map, (void *)NULL);
	if(stage33_result != NULL) stage_result(3, "remove", true, map);
	else stage_result(3, "remove", false, map);

	bool stage34_result = map->contains_key(map, (void *)NULL);
	if(stage34_result == true) stage_result(3, "contains", true, map);
	else stage_result(3, "contains", false, map);

	bool stage35_result = map->update(map, (void *)NULL, (void *)NULL);
	if(stage35_result == true) stage_result(3, "update", true, map);
	else stage_result(3, "update", false, map);

	hashmap_destroy(map);
	stage_log(3, "END");

	/* 4: Between Node Memory: No Duplicated Data */
	stage_log(4, "START");

	/* Test Data Setup */
	map = hashmap_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	map->put(map, string0, string0);
	map->put(map, string1, string1);
	map->put(map, string2, string2);
	map->put(map, string3, string3);
	map->put(map, string4, string4);

	/* Test Procedure */
	void* stage41_result = map->get(map, string0);
	if(stage41_result != NULL) stage_result(4, "get", true, map);
	else stage_result(4, "get", false, map);

	bool stage42_result = map->put(map, string0, string5);
	if(stage42_result == true) stage_result(4, "put", true, map);
	else stage_result(4, "put", false, map);

	void* stage43_result = map->remove(map, string0);
	if(stage43_result != NULL) stage_result(4, "remove", true, map);
	else stage_result(4, "remove", false, map);

	bool stage44_result = map->contains_key(map, string0);
	if(stage44_result == true) stage_result(4, "contains", true, map);
	else stage_result(4, "contains", false, map);

	bool stage45_result = map->update(map, string1, string6);
	if(stage45_result == true) stage_result(4, "update", true, map);
	else stage_result(4, "update", false, map);

	/* abnormal case */
	void* stage46_result = map->get(map, string9);
	if(stage46_result != NULL) stage_result(4, "abnormal get", true, map);
	else stage_result(4, "abnormal get", false, map);

	void* stage47_result = map->remove(map, string8);
	if(stage47_result != NULL) stage_result(4, "abnormal remove", true, map);
	else stage_result(4, "abnormal remove", false, map);

	bool stage48_result = map->contains_key(map, string8);
	if(stage48_result == true) stage_result(4, "abnormal contains_key", true, map);
	else stage_result(4, "abnormal contains_key", false, map);

	bool stage49_result = map->update(map, string8, string8);
	if(stage49_result == true) stage_result(4, "abnormal update", true, map);
	else stage_result(4, "abnormal update", false, map);

	hashmap_destroy(map);
	stage_log(4, "END");

	/* 5: Between Node Memory: Duplicated Data */
	stage_log(5, "START");

	/* Test Data Setup */
	map = hashmap_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	map->put(map, string0, string0);
	map->put(map, string1, string1);
	map->put(map, string2, string2);
	map->put(map, string3, string3);
	map->put(map, string4, string4);

	/* Test Procedure */
	void* stage51_result = map->get(map, string1);
	if(stage51_result != NULL) stage_result(5, "get", true, map);
	else stage_result(5, "get", false, map);

	bool stage52_result = map->put(map, string1, string1);
	if(stage52_result == true) stage_result(5, "put", true, map);
	else stage_result(5, "put", false, map);

	void* stage53_result = map->remove(map, string0);
	if(stage53_result != NULL) stage_result(5, "remove", true, map);
	else stage_result(5, "remove", false, map);

	bool stage54_result = map->contains_key(map, string1);
	if(stage54_result == true) stage_result(5, "contains", true, map);
	else stage_result(5, "contains", false, map);

	bool stage55_result = map->update(map, string1, string6);
	if(stage55_result == true) stage_result(5, "update", true, map);
	else stage_result(5, "update", false, map);

	hashmap_destroy(map);
	stage_log(5, "END");

	/* 6: Max Node Memory: NULL Data */
	stage_log(6, "START");

	/* Test Data Setup */
	map = hashmap_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	map->put(map, string0, string0);
	map->put(map, string1, string1);
	map->put(map, string2, string2);
	map->put(map, string3, string3);
	map->put(map, string4, string4);
	map->put(map, string5, string5);
	map->put(map, string6, string6);
	map->put(map, string7, string7);
	map->put(map, string8, string8);
	map->put(map, string9, string9);

	/* Test Procedure */

	void* stage61_result = map->get(map, (void *)NULL);
	if(stage61_result != NULL) stage_result(6, "get", true, map);
	else stage_result(6, "get", false, map);

	bool stage62_result = map->put(map, (void *)NULL, (void *)NULL);
	if(stage62_result == true) stage_result(6, "put", true, map);
	else stage_result(6, "put", false, map);

	void* stage63_result = map->remove(map, (void *)NULL);
	if(stage63_result != NULL) stage_result(6, "remove", true, map);
	else stage_result(6, "remove", false, map);

	bool stage64_result = map->contains_key(map, (void *)NULL);
	if(stage64_result == true) stage_result(6, "contains", true, map);
	else stage_result(6, "contains", false, map);

	bool stage65_result = map->update(map, (void *)NULL, (void *)NULL);
	if(stage65_result == true) stage_result(6, "update", true, map);
	else stage_result(6, "update", false, map);

	hashmap_destroy(map);
	stage_log(6, "END");

	/* 7: Max Node Memory: No Duplicated Data */
	stage_log(7, "START");

	/* Test Data Setup */
	map = hashmap_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	map->put(map, string0, string0);
	map->put(map, string1, string1);
	map->put(map, string2, string2);
	map->put(map, string3, string3);
	map->put(map, string4, string4);
	map->put(map, string5, string5);
	map->put(map, string6, string6);
	map->put(map, string7, string7);
	map->put(map, string8, string8);
	map->put(map, string9, string9);

	/* Test Procedure */
	void* stage71_result = map->get(map, string0);
	if(stage71_result != NULL) stage_result(7, "get", true, map);
	else stage_result(7, "get", false, map);

	bool stage72_result = map->put(map, string6, string6);
	if(stage72_result == true) stage_result(7, "put", true, map);
	else stage_result(7, "put", false, map);

	void* stage73_result = map->remove(map, string0);
	if(stage73_result != NULL) stage_result(7, "remove", true, map);
	else stage_result(7, "remove", false, map);

	bool stage74_result = map->contains_key(map, string1);
	if(stage74_result == true) stage_result(7, "contains", true, map);
	else stage_result(7, "contains", false, map);

	bool stage75_result = map->update(map, string1, string6);
	if(stage75_result == true) stage_result(7, "update", true, map);
	else stage_result(7, "update", false, map);

	hashmap_destroy(map);
	stage_log(7, "END");

	/* 8: Max Node Memory: Duplicated Data */
	stage_log(8, "START");

	/* Test Data Setup */
	map = hashmap_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	map->put(map, string0, string0);
	map->put(map, string1, string1);
	map->put(map, string2, string2);
	map->put(map, string3, string3);
	map->put(map, string4, string4);
	map->put(map, string5, string0);
	map->put(map, string6, string1);
	map->put(map, string7, string2);
	map->put(map, string8, string3);
	map->put(map, string9, string4);

	/* Test Procedure */
	void* stage81_result = map->get(map, string0);
	if(stage81_result != NULL) stage_result(8, "get", true, map);
	else stage_result(8, "get", false, map);

	bool stage82_result = map->put(map, string6, string1);
	if(stage82_result == true) stage_result(8, "put", true, map);
	else stage_result(8, "put", false, map);

	void* stage83_result = map->remove(map, string0);
	if(stage83_result != NULL) stage_result(8, "remove", true, map);
	else stage_result(8, "remove", false, map);

	bool stage84_result = map->contains_key(map, string1);
	if(stage84_result == true) stage_result(8, "contains", true, map);
	else stage_result(8, "contains", false, map);

	bool stage85_result = map->update(map, string1, string6);
	if(stage85_result == true) stage_result(8, "update", true, map);
	else stage_result(8, "update", false, map);

	hashmap_destroy(map);
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


static void stage_result(int stage, char* operation, bool opresult, HashMap* map) {
	time_t current_time;
	char* c_time_string;

	current_time = time(NULL);
	c_time_string = ctime(&current_time);

	if(opresult == true)
		fprintf(testresult, "(stage %d) %s is success.: %s", stage, operation, c_time_string);
	else
		fprintf(testresult, "(stage %d) %s is failed.: %s", stage, operation, c_time_string);

	/* hashmap content */
	fprintf(testresult, "hashmap content\n");
	Iterator* iter;
	void* element;
	for_each(element, map->key_set) {
		fprintf(testresult, "%s ", (char *)element);
	}
	fprintf(testresult, "\n");
}
