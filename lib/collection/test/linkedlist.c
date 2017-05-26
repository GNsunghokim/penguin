#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include <util/linkedlist.h>

static FILE *testlog; /* Test Log File */
static FILE *testresult; /* Test Result File */

static void stage_log(int stage, char* status);
static void stage_result(int stage, char* operation, bool opresult, LinkedList* list);

int linkedlist_testmain(int argc, const char *argv[]) {
	/* 0: Test Data/Result Initialization; Test Environment Setup */
	testlog = fopen("linkedlisttest_log.txt", "a");
	testresult = fopen("linkedlisttest_result.txt", "a");

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

	LinkedList *list = linkedlist_create(DATATYPE_STRING, POOLTYPE_LOCAL);

	if(list != NULL) stage_result(1, "create", true, list);
	else stage_result(1, "create", false, list);

	stage_log(1, "END");

	/* 2: No Node Memory Test */
	stage_log(2, "START");

	/* Test Data Setup */
	list = linkedlist_create(DATATYPE_STRING, POOLTYPE_LOCAL);

	bool stage2_result = list->add(list, string0);
	if(stage2_result == true) stage_result(2, "add", true, list);
	else stage_result(2, "add", false, list);

	linkedlist_destroy(list);
	stage_log(2, "END");

	/* 3: Between Node Memory: NULL Data */
	stage_log(3, "START");

	/* Test Data Setup */
	list = linkedlist_create(DATATYPE_STRING, POOLTYPE_LOCAL);

	/* Test Procedure */

	bool stage31_result = list->add(list, (void *)NULL);
	if(stage31_result == true) stage_result(3, "add", true, list);
	else stage_result(3, "add", false, list);

	bool stage32_result = list->remove(list, (ListNode *)NULL);
	if(stage32_result == true) stage_result(3, "remove", true, list);
	else stage_result(3, "remove", false, list);

	void* stage33_result = list->get(list, (size_t) NULL);
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

	bool stage38_result = list->add_first(list, (void *)NULL);
	if(stage38_result == true) stage_result(3, "add_first", true, list);
	else stage_result(3, "add_first", false, list);

	bool stage39_result = list->add_last(list, (void *)NULL);
	if(stage39_result == true) stage_result(3, "add_last", true, list);
	else stage_result(3, "add_last", false, list);

	void* stage310_result = list->remove_first(list);
	if(stage310_result != NULL) stage_result(3, "remove_first", true, list);
	else stage_result(3, "remove_first", false, list);

	void* stage311_result = list->remove_last(list);
	if(stage311_result != NULL) stage_result(3, "remove_last", true, list);
	else stage_result(3, "remove_last", false, list);

	void* stage312_result = list->get_first(list);
	if(stage312_result != NULL) stage_result(3, "get_first", true, list);
	else stage_result(3, "get_first", false, list);

	void* stage313_result = list->get_last(list);
	if(stage313_result != NULL) stage_result(3, "get_last", true, list);
	else stage_result(3, "get_last", false, list);

	list->rotate(list);
	if(list->head != NULL) stage_result(3, "rotate", true, list);
	else stage_result(3, "rotate", false, list);

	linkedlist_destroy(list);
	stage_log(3, "END");

	/* 4: Between Node Memory: No Duplicated Data */
	stage_log(4, "START");

	/* Test Data Setup */
	list = linkedlist_create(DATATYPE_STRING, POOLTYPE_LOCAL);
	list->add(list, string0);
	list->add(list, string1);
	list->add(list, string2);
	list->add(list, string3);
	list->add(list, string4);

	/* Test Procedure */
	bool stage41_result = list->add(list, string5);
	if(stage41_result == true) stage_result(4, "add", true, list);
	else stage_result(4, "add", false, list);

	bool stage42_result = list->remove(list, string5 );
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

	bool stage48_result = list->add_first(list, string3);
	if(stage48_result == true) stage_result(4, "add_first", true, list);
	else stage_result(4, "add_first", false, list);

	bool stage49_result = list->add_last(list, string6);
	if(stage49_result == true) stage_result(4, "add_last", true, list);
	else stage_result(4, "add_last", false, list);

	void* stage410_result = list->remove_first(list);
	if(stage410_result != NULL) stage_result(4, "remove_first", true, list);
	else stage_result(4, "remove_first", false, list);

	void* stage411_result = list->remove_last(list);
	if(stage411_result != NULL) stage_result(4, "remove_last", true, list);
	else stage_result(4, "remove_last", false, list);

	void* stage412_result = list->get_first(list);
	if(stage412_result != NULL) stage_result(4, "get_first", true, list);
	else stage_result(4, "get_first", false, list);

	void* stage413_result = list->get_last(list);
	if(stage413_result != NULL) stage_result(4, "get_last", true, list);
	else stage_result(4, "get_last", false, list);

	list->rotate(list);
	if(list->head != NULL) stage_result(4, "rotate", true, list);
	else stage_result(4, "rotate", false, list);

	/* abnormal test cases */
	void* stage414_result = list->set(list, 20, string9);
        if(stage414_result != NULL) stage_result(4, "abnormal set", true, list);
	else stage_result(4, "abnormal set", false, list);

	void* stage415_result = list->remove_at(list, 21);
	if(stage415_result != NULL) stage_result(4, "abnormal remove_at", true, list);
	else stage_result(4, "abnormal remove_at", false, list);

	int stage416_result = list->index_of(list, string8);
	if(stage416_result == 0) stage_result(4, "abnormal index_of", true, list);
	else stage_result(4, "abnormal index_of", false, list);

	linkedlist_destroy(list);
	stage_log(4, "END");

	/* 5: Between Node Memory: Duplicated Data */
	stage_log(5, "START");

	/* Test Data Setup */
	list = linkedlist_create(DATATYPE_STRING, POOLTYPE_LOCAL);
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

	void* stage53_result = list->get(list, 2);
	if(stage53_result != NULL) stage_result(5, "get", true, list);
	else stage_result(5, "get", false, list);

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

	bool stage58_result = list->add_first(list, string0);
	if(stage58_result == true) stage_result(5, "add_first", true, list);
	else stage_result(5, "add_first", false, list);

	bool stage59_result = list->add_last(list, string4);
	if(stage59_result == true) stage_result(5, "add_last", true, list);
	else stage_result(5, "add_last", false, list);

	void* stage510_result = list->remove_first(list);
	if(stage510_result != NULL) stage_result(5, "remove_first", true, list);
	else stage_result(5, "remove_first", false, list);

	void* stage511_result = list->remove_last(list);
	if(stage511_result != NULL) stage_result(5, "remove_last", true, list);
	else stage_result(5, "remove_last", false, list);

	void* stage512_result = list->get_first(list);
	if(stage512_result != NULL) stage_result(5, "get_first", true, list);
	else stage_result(5, "get_first", false, list);

	void* stage513_result = list->get_last(list);
	if(stage513_result != NULL) stage_result(5, "get_last", true, list);
	else stage_result(5, "get_last", false, list);

	list->rotate(list);
	if(list->head != NULL) stage_result(5, "rotate", true, list);
	else stage_result(5, "rotate", false, list);

	linkedlist_destroy(list);
	stage_log(5, "END");

	/* 6: Max Node Memory: NULL Data */
	stage_log(6, "START");

	/* Test Data Setup */
	list = linkedlist_create(DATATYPE_STRING, POOLTYPE_LOCAL);
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

	bool stage68_result = list->add_first(list, (void *)NULL);
	if(stage68_result == true) stage_result(6, "add_first", true, list);
	else stage_result(6, "add_first", false, list);

	bool stage69_result = list->add_last(list, (void *)NULL);
	if(stage69_result == true) stage_result(6, "add_last", true, list);
	else stage_result(6, "add_last", false, list);

	void* stage610_result = list->remove_first(list);
	if(stage610_result != NULL) stage_result(6, "remove_first", true, list);
	else stage_result(6, "remove_first", false, list);

	void* stage611_result = list->remove_last(list);
	if(stage611_result != NULL) stage_result(6, "remove_last", true, list);
	else stage_result(6, "remove_last", false, list);

	void* stage612_result = list->get_first(list);
	if(stage612_result != NULL) stage_result(6, "get_first", true, list);
	else stage_result(6, "get_first", false, list);

	void* stage613_result = list->get_last(list);
	if(stage613_result != NULL) stage_result(6, "get_last", true, list);
	else stage_result(6, "get_last", false, list);

	list->rotate(list);
	if(list->head != NULL) stage_result(6, "rotate", true, list);
	else stage_result(6, "rotate", false, list);

	linkedlist_destroy(list);
	stage_log(6, "END");

	/* 7: Max Node Memory: No Duplicated Data */
	stage_log(7, "START");

	/* Test Data Setup */
	list = linkedlist_create(DATATYPE_STRING, POOLTYPE_LOCAL);
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

	bool stage78_result = list->add_first(list, string2);
	if(stage78_result == true) stage_result(7, "add_first", true, list);
	else stage_result(7, "add_first", false, list);

	bool stage79_result = list->add_last(list, string3);
	if(stage79_result == true) stage_result(7, "add_last", true, list);
	else stage_result(7, "add_last", false, list);

	void* stage710_result = list->remove_first(list);
	if(stage710_result != NULL) stage_result(7, "remove_first", true, list);
	else stage_result(7, "remove_first", false, list);

	void* stage711_result = list->remove_last(list);
	if(stage711_result != NULL) stage_result(7, "remove_last", true, list);
	else stage_result(7, "remove_last", false, list);

	void* stage712_result = list->get_first(list);
	if(stage712_result != NULL) stage_result(7, "get_first", true, list);
	else stage_result(7, "get_first", false, list);

	void* stage713_result = list->get_last(list);
	if(stage713_result != NULL) stage_result(7, "get_last", true, list);
	else stage_result(7, "get_last", false, list);

	list->rotate(list);
	if(list->head != NULL) stage_result(7, "rotate", true, list);
	else stage_result(7, "rotate", false, list);

	linkedlist_destroy(list);
	stage_log(7, "END");

	/* 8: Max Node Memory: Duplicated Data */
	stage_log(8, "START");

	/* Test Data Setup */
	list = linkedlist_create(DATATYPE_STRING, POOLTYPE_LOCAL);
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

	bool stage88_result = list->add_first(list, string1);
	if(stage88_result == true) stage_result(8, "add_first", true, list);
	else stage_result(8, "add_first", false, list);

	bool stage89_result = list->add_last(list, string1);
	if(stage89_result == true) stage_result(8, "add_last", true, list);
	else stage_result(8, "add_last", false, list);

	void* stage810_result = list->remove_first(list);
	if(stage810_result != NULL) stage_result(8, "remove_first", true, list);
	else stage_result(8, "remove_first", false, list);

	void* stage811_result = list->remove_last(list);
	if(stage811_result != NULL) stage_result(8, "remove_last", true, list);
	else stage_result(8, "remove_last", false, list);

	void* stage812_result = list->get_first(list);
	if(stage812_result != NULL) stage_result(8, "get_first", true, list);
	else stage_result(8, "get_first", false, list);

	void* stage813_result = list->get_last(list);
	if(stage813_result != NULL) stage_result(8, "get_last", true, list);
	else stage_result(8, "get_last", false, list);

	list->rotate(list);
	if(list->head != NULL) stage_result(8, "rotate", true, list);
	else stage_result(8, "rotate", false, list);

	linkedlist_destroy(list);
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


static void stage_result(int stage, char* operation, bool opresult, LinkedList* list) {
	time_t current_time;
	char* c_time_string;

	current_time = time(NULL);
	c_time_string = ctime(&current_time);

	if(opresult == true)
		fprintf(testresult, "(stage %d) %s is success.: %s", stage, operation, c_time_string);
	else
		fprintf(testresult, "(stage %d) %s is failed.: %s", stage, operation, c_time_string);

	/* Linkedlist content */
	fprintf(testresult, "linkedlist content\n");
	Iterator* iter;
	void* element;
	for_each(element, list) {
		fprintf(testresult, "%s ", (char *)element);
	}
	fprintf(testresult, "\n");
}
