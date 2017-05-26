#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include <util/arraylist.h>

static FILE *testlog; /* Test Log File */
static FILE *testresult; /* Test Result File */

static void stage_log(int stage, char* status);
static void stage_result(int stage, char* operation, bool opresult, ArrayList* list);

int arraylist_testmain(int argc, const char *argv[]) {
	/* 0: Test Data/Result Initialization; Test Environment Setup */
	testlog = fopen("arraylisttest_log.txt", "a");
	testresult = fopen("arraylisttest_result.txt", "a");

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

	ArrayList *list = arraylist_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);

	if(list != NULL) stage_result(1, "create", true, list);
	else stage_result(1, "create", false, list);

	stage_log(1, "END");

	/* 2: No Node Memory Test */
	stage_log(2, "START");

	/* Test Data Setup */
	list = arraylist_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);

	bool stage2_result = list->add(list, string0);
	if(stage2_result == true) stage_result(2, "add", true, list);
	else stage_result(2, "add", false, list);

	arraylist_destroy(list);
	stage_log(2, "END");

	/* 3: Between Node Memory: NULL Data */
	stage_log(3, "START");

	/* Test Data Setup */
	list = arraylist_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);

	/* Test Procedure */

	bool stage31_result = list->add(list, (void *)NULL);
	if(stage31_result == true) stage_result(3, "add", true, list);
	else stage_result(3, "add", false, list);

	bool stage32_result = list->remove(list, (void *)NULL);
	if(stage32_result == true) stage_result(3, "remove", true, list);
	else stage_result(3, "remove", false, list);

	void* stage33_result = list->get(list, (size_t)NULL);
	if(stage33_result != NULL) stage_result(3, "get", true, list);
	else stage_result(3, "get", false, list);

	void* stage34_result = list->set(list, (size_t)NULL, (void *)NULL);
	if(stage34_result != NULL) stage_result(3, "set", true, list);
	else stage_result(3, "set", false, list);

	void* stage35_result = list->remove_at(list, (size_t)NULL);
	if(stage35_result != NULL) stage_result(3, "remove_at", true, list);
	else stage_result(3, "remove_at", false, list);

	bool stage36_result = list->add_at(list, (size_t)NULL, (void *)NULL);
	if(stage36_result == true) stage_result(3, "add_at", true, list);
	else stage_result(3, "add_at", false, list);

	int stage37_result = list->index_of(list, (void *)NULL);
	if(stage37_result == -1) stage_result(3, "index_of", true, list);
	else stage_result(3, "index_of", false, list);

	bool stage38_result = list->is_available(list);
	if(stage38_result == true) stage_result(3, "is_available", true, list);
	else stage_result(3, "is_available", false, list);

	arraylist_destroy(list);
	stage_log(3, "END");

	/* 4: Between Node Memory: No Duplicated Data */
	stage_log(4, "START");

	/* Test Data Setup */
	list = arraylist_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	list->add(list, string0);
	list->add(list, string1);
	list->add(list, string2);
	list->add(list, string3);
	list->add(list, string4);

	/* Test Procedure */
	bool stage41_result = list->add(list, string5);
	if(stage41_result == true) stage_result(4, "add", true, list);
	else stage_result(4, "add", false, list);

	bool stage42_result = list->remove(list, string5);
	if(stage42_result == true) stage_result(4, "remove", true, list);
	else stage_result(4, "remove", false, list);

	void* stage43_result = list->get(list, 0);
	if(stage43_result != NULL) stage_result(4, "get", true, list);
	else stage_result(4, "get", false, list);

	void* stage44_result = list->set(list, 1, string5);
	if(stage44_result != NULL) stage_result(4, "set", true, list);
	else stage_result(4, "set", false, list);

	void* stage45_result = list->remove_at(list, 4);
	if(stage45_result != NULL) stage_result(4, "remove_at", true, list);
	else stage_result(4, "remove_at", false, list);

	bool stage46_result = list->add_at(list, 1, string5);
	if(stage46_result == true) stage_result(4, "add_at", true, list);
	else stage_result(4, "add_at", false, list);

	int stage47_result = list->index_of(list, string3);
	if(stage47_result == 2) stage_result(4, "index_of", true, list);
	else stage_result(4, "index_of", false, list);

	bool stage48_result = list->is_available(list);
	if(stage38_result == true) stage_result(3, "is_available", true, list);
	else stage_result(3, "is_available", false, list);

	/* abnormal cases */
	void* stage49_result = list->set(list, 20, string9);
	if(stage49_result != NULL) stage_result(4, "abnormal set", true, list);
	else stage_result(4, "abnormal set", false, list);

        void* stage410_result = list->remove_at(list, 21);
	if(stage410_result != NULL) stage_result(4, "abnormal remove_at", true, list);
	else stage_result(4, "abnormal remove_at", false, list);

	int stage411_result = list->index_of(list, string8);
	if(stage411_result == 0) stage_result(4, "abnormal index_of", true, list);
	else stage_result(4, "abnormal index_of", false, list);

	arraylist_destroy(list);
	stage_log(4, "END");

	/* 5: Between Node Memory: Duplicated Data */
	stage_log(5, "START");

	/* Test Data Setup */
	list = arraylist_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	list->add(list, string0);
	list->add(list, string1);
	list->add(list, string2);
	list->add(list, string3);
	list->add(list, string4);

	/* Test Procedure */
	bool stage51_result = list->add(list, string0);
	if(stage51_result == true) stage_result(5, "add", true, list);
	else stage_result(5, "add", false, list);

	bool stage52_result = list->remove(list, string1);
	if(stage52_result == true) stage_result(5, "remove", true, list);
	else stage_result(5, "remove", false, list);

	void* stage53_result = list->get(list, 1);
	if(stage53_result != NULL) stage_result(5, "get", true, list);
	else stage_result(5, "get", false, list), list;

	void* stage54_result = list->set(list, 1, string1);
	if(stage54_result != NULL) stage_result(5, "set", true, list);
	else stage_result(5, "set", false, list);

	void* stage55_result = list->remove_at(list, 1);
	if(stage55_result != NULL) stage_result(5, "remove_at", true, list);
	else stage_result(5, "remove_at", false, list);

	bool stage56_result = list->add_at(list, 1, string1);
	if(stage56_result == true) stage_result(5, "add_at", true, list);
	else stage_result(5, "add_at", false, list);

	int stage57_result = list->index_of(list, string0);
	if(stage57_result == 2) stage_result(5, "index_of", true, list);
	else stage_result(5, "index_of", false, list);

	bool stage58_result = list->is_available(list);
	if(stage58_result == true) stage_result(5, "is_available", true, list);
	else stage_result(5, "is_available", false, list);

	arraylist_destroy(list);
	stage_log(5, "END");

	/* 6: Max Node Memory: NULL Data */
	stage_log(6, "START");

	/* Test Data Setup */
	list = arraylist_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	list->add(list, string0);
	list->add(list, string1);
	list->add(list, string2);
	list->add(list, string3);
	list->add(list, string4);
	list->add(list, string5);
	list->add(list, string6);
	list->add(list, string7);
	list->add(list, string8);
	list->add(list, string9);

	/* Test Procedure */

	bool stage61_result = list->add(list, (void *)NULL);
	if(stage61_result == true) stage_result(6, "add", true, list);
	else stage_result(6, "add", false, list);

	bool stage62_result = list->remove(list, (void *)NULL);
	if(stage62_result == true) stage_result(6, "remove", true, list);
	else stage_result(6, "remove", false, list);

	void* stage63_result = list->get(list, (size_t)NULL);
	if(stage63_result != NULL) stage_result(6, "get", true, list);
	else stage_result(6, "get", false, list);

	void* stage64_result = list->set(list, (size_t)NULL, (void *)NULL);
	if(stage64_result != NULL) stage_result(6, "set", true, list);
	else stage_result(6, "set", false, list);

	void* stage65_result = list->remove_at(list, (size_t)NULL);
	if(stage65_result != NULL) stage_result(6, "remove_at", true, list);
	else stage_result(6, "remove_at", false, list);

	bool stage66_result = list->add_at(list, (size_t)NULL, (void *)NULL);
	if(stage66_result == true) stage_result(6, "add_at", true, list);
	else stage_result(6, "add_at", false, list);

	int stage67_result = list->index_of(list, (void *)NULL);
	if(stage67_result == 2) stage_result(6, "index_of", true, list);
	else stage_result(6, "index_of", false, list);

	bool stage68_result = list->is_available(list);
	if(stage68_result == true) stage_result(6, "is_available", true, list);
	else stage_result(6, "is_available", false, list);

	arraylist_destroy(list);
	stage_log(6, "END");

	/* 7: Max Node Memory: No Duplicated Data */
	stage_log(7, "START");

	/* Test Data Setup */
	list = arraylist_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	list->add(list, string0);
	list->add(list, string1);
	list->add(list, string2);
	list->add(list, string3);
	list->add(list, string4);
	list->add(list, string5);
	list->add(list, string6);
	list->add(list, string7);
	list->add(list, string8);
	list->add(list, string9);

	/* Test Procedure */
	bool stage71_result = list->add(list, string1);
	if(stage71_result == true) stage_result(7, "add", true, list);
	else stage_result(7, "add", false, list);

	bool stage72_result = list->remove(list, string1);
	if(stage72_result == true) stage_result(7, "remove", true, list);
	else stage_result(7, "remove", false, list);

	void* stage73_result = list->get(list, 1);
	if(stage73_result != NULL) stage_result(7, "get", true, list);
	else stage_result(7, "get", false, list);

	void* stage74_result = list->set(list, 1, string1);
	if(stage74_result != NULL) stage_result(7, "set", true, list);
	else stage_result(7, "set", false, list);

	void* stage75_result = list->remove_at(list, 1);
	if(stage75_result != NULL) stage_result(7, "remove_at", true, list);
	else stage_result(7, "remove_at", false, list);

	bool stage76_result = list->add_at(list, 1, string1);
	if(stage76_result == true) stage_result(7, "add_at", true, list);
	else stage_result(7, "add_at", false, list);

	int stage77_result = list->index_of(list, string1);
	if(stage77_result == 2) stage_result(7, "index_of", true, list);
	else stage_result(7, "index_of", false, list);

	bool stage78_result = list->is_available(list);
	if(stage78_result == true) stage_result(7, "is_available", true, list);
	else stage_result(7, "is_available", false, list);

	arraylist_destroy(list);
	stage_log(7, "END");

	/* 8: Max Node Memory: Duplicated Data */
	stage_log(8, "START");

	/* Test Data Setup */
	list = arraylist_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	list->add(list, string0);
	list->add(list, string1);
	list->add(list, string2);
	list->add(list, string3);
	list->add(list, string4);
	list->add(list, string0);
	list->add(list, string1);
	list->add(list, string2);
	list->add(list, string3);
	list->add(list, string4);

	/* Test Procedure */
	bool stage81_result = list->add(list, string1);
	if(stage81_result == true) stage_result(8, "add", true, list);
	else stage_result(8, "add", false, list);

	bool stage82_result = list->remove(list, string1);
	if(stage82_result == true) stage_result(8, "remove", true, list);
	else stage_result(8, "remove", false, list);

	void* stage83_result = list->get(list, 1);
	if(stage83_result != NULL) stage_result(8, "get", true, list);
	else stage_result(8, "get", false, list);

	void* stage84_result = list->set(list, 1, string1);
	if(stage84_result != NULL) stage_result(8, "set", true, list);
	else stage_result(8, "set", false, list);

	void* stage85_result = list->remove_at(list, 1);
	if(stage85_result != NULL) stage_result(8, "remove_at", true, list);
	else stage_result(8, "remove_at", false, list);

	bool stage86_result = list->add_at(list, 1, string1);
	if(stage86_result == true) stage_result(8, "add_at", true, list);
	else stage_result(8, "add_at", false, list);

	int stage87_result = list->index_of(list, string1);
	if(stage87_result == 2) stage_result(8, "index_of", true, list);
	else stage_result(8, "index_of", false, list);

	bool stage88_result = list->is_available(list);
	if(stage88_result == true) stage_result(8, "is_available", true, list);
	else stage_result(8, "is_available", false, list);

	arraylist_destroy(list);
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


static void stage_result(int stage, char* operation, bool opresult, ArrayList* list) {
	time_t current_time;
	char* c_time_string;

	current_time = time(NULL);
	c_time_string = ctime(&current_time);

	if(opresult == true)
		fprintf(testresult, "(stage %d) %s is success.: %s", stage, operation, c_time_string);
	else
		fprintf(testresult, "(stage %d) %s is failed.: %s", stage, operation, c_time_string);

	/* arraylist content */
	fprintf(testresult, "arraylist content\n");
	Iterator* iter;
	void* element;
	for_each(element, list) {
		fprintf(testresult, "%s ", (char *)element);
	}
	fprintf(testresult, "\n");
}

