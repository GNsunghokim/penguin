#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include <util/hashset.h>

static FILE *testlog; /* Test Log File */
static FILE *testresult; /* Test Result File */

static void stage_log(int stage, char* status);
static void stage_result(int stage, char* operation, bool opresult, HashSet* set);

int hashset_testmain(int argc, const char *argv[]) {
	/* 0: Test Data/Result Initialization; Test Environment peekup */
	testlog = fopen("hashsettest_log.txt", "a");
	testresult = fopen("hashsettest_result.txt", "a");

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

	HashSet *set = hashset_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);

	if(set != NULL) stage_result(1, "create", true, set);
	else stage_result(1, "create", false, set);

	stage_log(1, "END");

	/* 2: No Node Memory Test */
	stage_log(2, "START");

	/* Test Data peekup */
	set = hashset_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);

	bool stage2_result = set->add(set, string0);
	if(stage2_result == true) stage_result(2, "get", true, set);
	else stage_result(2, "get", false, set);

	hashset_destroy(set);
	stage_log(2, "END");

	/* 3: Between Node Memory: NULL Data */
	stage_log(3, "START");

	/* Test Data peekup */
	set = hashset_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);

	/* Test Procedure */

	void* stage31_result = set->get(set, (void *)NULL);
	if(stage31_result != NULL) stage_result(3, "get", true, set);
	else stage_result(3, "get", false, set);

	bool stage32_result = set->add(set, (void *)NULL);
	if(stage32_result == true) stage_result(3, "add", true, set);
	else stage_result(3, "add", false, set);

	bool stage33_result = set->remove(set, (void*)NULL);
	if(stage33_result == true) stage_result(3, "remove", true, set);
	else stage_result(3, "remove", false, set);

	bool stage34_result = set->contains(set, (void *)NULL);
	if(stage34_result == true) stage_result(3, "contains", true, set);
	else stage_result(3, "contains", false, set);

	hashset_destroy(set);
	stage_log(3, "END");

	/* 4: Between Node Memory: No Duplicated Data */
	stage_log(4, "START");

	/* Test Data removeup */
	set = hashset_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	set->add(set, string0);
	set->add(set, string1);
	set->add(set, string2);
	set->add(set, string3);
	set->add(set, string4);

	/* Test Procedure */
	void* stage41_result = set->get(set, string5);
	if(stage41_result != NULL) stage_result(4, "get", true, set);
	else stage_result(4, "get", false, set);

	bool stage42_result = set->add(set, string5 );
	if(stage42_result == true) stage_result(4, "add", true, set);
	else stage_result(4, "add", false, set);

	bool stage43_result = set->remove(set, string5);
	if(stage43_result == true) stage_result(4, "remove", true, set);
	else stage_result(4, "remove", false, set);

	bool stage44_result = set->contains(set, string1);
	if(stage44_result == true) stage_result(4, "contains", true, set);
	else stage_result(4, "contains", false, set);

	/* abnormal case */
	void* stage45_result = set->get(set, string9);
	if(stage45_result != NULL) stage_result(4, "abnormal get", true, set);
	else stage_result(4, "abnormal get", false, set);

        bool stage46_result = set->remove(set, string8);
	if(stage46_result == true) stage_result(4, "abnormal remove", true, set);
	else stage_result(4, "abnormal remove", false, set);

	bool stage47_result = set->contains(set, string8);
	if(stage47_result == true) stage_result(4, "abnormal contains", true, set);
	else stage_result(4, "abnormal contains", false, set);

	hashset_destroy(set);
	stage_log(4, "END");

	/* 5: Between Node Memory: Duplicated Data */
	stage_log(5, "START");

	/* Test Data removeup */
	set = hashset_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	set->add(set, string0);
	set->add(set, string1);
	set->add(set, string2);
	set->add(set, string3);
	set->add(set, string4);

	/* Test Procedure */
	void* stage51_result = set->get(set, string0);
	if(stage51_result != NULL) stage_result(5, "get", true, set);
	else stage_result(5, "get", false, set);

	bool stage52_result = set->add(set, string1);
	if(stage52_result == true) stage_result(5, "add", true, set);
	else stage_result(5, "add", false, set);

	bool stage53_result = set->remove(set, string1);
	if(stage53_result == true) stage_result(5, "remove", true, set);
	else stage_result(5, "remove", false, set);

	bool stage54_result = set->contains(set, string1);
	if(stage54_result == true) stage_result(5, "contains", true, set);
	else stage_result(5, "contains", false, set);

	hashset_destroy(set);
	stage_log(5, "END");

	/* 6: Max Node Memory: NULL Data */
	stage_log(6, "START");

	/* Test Data removeup */
	set = hashset_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	set->add(set, string0);
	set->add(set, string1);
	set->add(set, string2);
	set->add(set, string3);
	set->add(set, string4);
	set->add(set, string5);
	set->add(set, string6);
	set->add(set, string7);
	set->add(set, string8);
	set->add(set, string9);

	/* Test Procedure */

	void* stage61_result = set->get(set, (void *)NULL);
	if(stage61_result != NULL) stage_result(6, "get", true, set);
	else stage_result(6, "get", false, set);

	bool stage62_result = set->add(set, (void *)NULL);
	if(stage62_result == true) stage_result(6, "add", true, set);
	else stage_result(6, "add", false, set);

	bool stage63_result = set->remove(set, (void *)NULL);
	if(stage63_result == true) stage_result(6, "remove", true, set);
	else stage_result(6, "remove", false, set);

	bool stage64_result = set->contains(set, (void *)NULL);
	if(stage64_result == true) stage_result(6, "contains", true, set);
	else stage_result(6, "contains", false, set);

	hashset_destroy(set);
	stage_log(6, "END");

	/* 7: Max Node Memory: No Duplicated Data */
	stage_log(7, "START");

	/* Test Data removeup */
	set = hashset_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	set->add(set, string0);
	set->add(set, string1);
	set->add(set, string2);
	set->add(set, string3);
	set->add(set, string4);
	set->add(set, string5);
	set->add(set, string6);
	set->add(set, string7);
	set->add(set, string8);
	set->add(set, string9);

	/* Test Procedure */
	void* stage71_result = set->get(set, string1);
	if(stage71_result != NULL) stage_result(7, "get", true, set);
	else stage_result(7, "get", false, set);

	bool stage72_result = set->add(set, string1);
	if(stage72_result == true) stage_result(7, "add", true, set);
	else stage_result(7, "add", false, set);

	bool stage73_result = set->remove(set, string1);
	if(stage73_result == true) stage_result(7, "remove", true, set);
	else stage_result(7, "remove", false, set);

	bool stage74_result = set->contains(set, string1);
	if(stage74_result == true) stage_result(7, "contains", true, set);
	else stage_result(7, "contains", false, set);

	hashset_destroy(set);
	stage_log(7, "END");

	/* 8: Max Node Memory: Duplicated Data */
	stage_log(8, "START");

	/* Test Data removeup */
	set = hashset_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	set->add(set, string0);
	set->add(set, string1);
	set->add(set, string2);
	set->add(set, string3);
	set->add(set, string4);
	set->add(set, string0);
	set->add(set, string1);
	set->add(set, string2);
	set->add(set, string3);
	set->add(set, string4);

	/* Test Procedure */
	void* stage81_result = set->get(set, string1);
	if(stage81_result != NULL) stage_result(8, "get", true, set);
	else stage_result(8, "get", false, set);

	bool stage82_result = set->add(set, string1);
	if(stage82_result == true) stage_result(8, "add", true, set);
	else stage_result(8, "add", false, set);

	bool stage83_result = set->remove(set, string1);
	if(stage83_result == true) stage_result(8, "remove", true, set);
	else stage_result(8, "remove", false, set);

	bool stage84_result = set->contains(set, string1);
	if(stage84_result == true) stage_result(8, "contains", true, set);
	else stage_result(8, "contains", false, set);

	hashset_destroy(set);
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


static void stage_result(int stage, char* operation, bool opresult, HashSet* set) {
	time_t current_time;
	char* c_time_string;

	current_time = time(NULL);
	c_time_string = ctime(&current_time);

	if(opresult == true)
		fprintf(testresult, "(stage %d) %s is success.: %s", stage, operation, c_time_string);
	else
		fprintf(testresult, "(stage %d) %s is failed.: %s", stage, operation, c_time_string);

	/* hashset content */
	fprintf(testresult, "hashset content\n");
	Iterator* iter;
	void* element;
	for_each(element, set) {
		fprintf(testresult, "%s ", (char *)element);
	}
	fprintf(testresult, "\n");
}
