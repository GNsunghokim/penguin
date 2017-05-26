#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include <util/arrayqueue.h>

static FILE *testlog; /* Test Log File */
static FILE *testresult; /* Test Result File */

static void stage_log(int stage, char* status);
static void stage_result(int stage, char* operation, bool opresult, ArrayQueue* queue);

int arrayqueue_testmain(int argc, const char *argv[]) {
	/* 0: Test Data/Result Initialization; Test Environment peekup */
	testlog = fopen("arrayqueuetest_log.txt", "a");
	testresult = fopen("arrayqueuetest_result.txt", "a");

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

	ArrayQueue *queue = arrayqueue_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);

	if(queue != NULL) stage_result(1, "create", true, queue);
	else stage_result(1, "create", false, queue);

	stage_log(1, "END");

	/* 2: No Node Memory Test */
	stage_log(2, "START");

	/* Test Data peekup */
	queue = arrayqueue_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);

	bool stage2_result = queue->enqueue(queue, string0);
	if(stage2_result == true) stage_result(2, "enqueue", true, queue);
	else stage_result(2, "enqueue", false, queue);

	arrayqueue_destroy(queue);
	stage_log(2, "END");

	/* 3: Between Node Memory: NULL Data */
	stage_log(3, "START");

	/* Test Data peekup */
	queue = arrayqueue_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);

	/* Test Procedure */

	bool stage31_result = queue->enqueue(queue, (void *)NULL);
	if(stage31_result == true) stage_result(3, "enqueue", true, queue);
	else stage_result(3, "enqueue", false, queue);

	void* stage32_result = queue->dequeue(queue);
	if(stage32_result != NULL) stage_result(3, "dequeue", true, queue);
	else stage_result(3, "dequeue", false, queue);

	void* stage33_result = queue->get(queue, (size_t)NULL);
	if(stage33_result != NULL) stage_result(3, "get", true, queue);
	else stage_result(3, "get", false, queue);

	void* stage34_result = queue->peek(queue);
	if(stage34_result != NULL) stage_result(3, "peek", true, queue);
	else stage_result(3, "peek", false, queue);

	bool stage35_result = queue->is_available(queue);
	if(stage35_result == true) stage_result(3, "is_available", true, queue);
	else stage_result(3, "is_available", false, queue);

	bool stage36_result = queue->resize(queue, (size_t)NULL, (void*)NULL);
	if(stage36_result == true) stage_result(3, "resize", true, queue);
	else stage_result(3, "resize", false, queue);

	arrayqueue_destroy(queue);
	stage_log(3, "END");

	/* 4: Between Node Memory: No Duplicated Data */
	stage_log(4, "START");

	/* Test Data peekup */
	queue = arrayqueue_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	queue->enqueue(queue, string0);
	queue->enqueue(queue, string1);
	queue->enqueue(queue, string2);
	queue->enqueue(queue, string3);
	queue->enqueue(queue, string4);

	/* Test Procedure */
	bool stage41_result = queue->enqueue(queue, string5);
	if(stage41_result == true) stage_result(4, "enqueue", true, queue);
	else stage_result(4, "enqueue", false, queue);

	void* stage42_result = queue->dequeue(queue);
	if(stage42_result != NULL) stage_result(4, "dequeue", true, queue);
	else stage_result(4, "dequeue", false, queue);

	void* stage43_result = queue->get(queue, 0);
	if(stage43_result != NULL) stage_result(4, "get", true, queue);
	else stage_result(4, "get", false, queue);

	void* stage44_result = queue->peek(queue);
	if(stage44_result != NULL) stage_result(4, "peek", true, queue);
	else stage_result(4, "peek", false, queue);

	bool stage45_result = queue->is_available(queue);
	if(stage45_result == true) stage_result(4, "is_available", true, queue);
	else stage_result(4, "is_available", false, queue);

	bool stage46_result = queue->resize(queue, 20, (void*)NULL);
	if(stage46_result == true) stage_result(4, "resize", true, queue);
	else stage_result(4, "resize", false, queue);

	/* abnormal cases */
	void* stage47_result = queue->get(queue, 10);
	if(stage47_result != NULL) stage_result(4, "abnormal get", true, queue);
	else stage_result(4, "abnormal get", false, queue);

	arrayqueue_destroy(queue);
	stage_log(4, "END");

	/* 5: Between Node Memory: Duplicated Data */
	stage_log(5, "START");

	/* Test Data peekup */
	queue = arrayqueue_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	queue->enqueue(queue, string0);
	queue->enqueue(queue, string1);
	queue->enqueue(queue, string2);
	queue->enqueue(queue, string3);
	queue->enqueue(queue, string4);

	/* Test Procedure */
	bool stage51_result = queue->enqueue(queue, string0);
	if(stage51_result == true) stage_result(5, "enqueue", true, queue);
	else stage_result(5, "enqueue", false, queue);

	void* stage52_result = queue->dequeue(queue);
	if(stage52_result != NULL) stage_result(5, "dequeue", true, queue);
	else stage_result(5, "dequeue", false, queue);

	void* stage53_result = queue->get(queue, 1);
	if(stage53_result != NULL) stage_result(5, "get", true, queue);
	else stage_result(5, "get", false, queue);

	void* stage54_result = queue->peek(queue);
	if(stage54_result != NULL) stage_result(5, "peek", true, queue);
	else stage_result(5, "peek", false, queue);

	bool stage55_result = queue->is_available(queue);
	if(stage55_result == true) stage_result(5, "is_available", true, queue);
	else stage_result(5, "is_available", false, queue);

	bool stage56_result = queue->resize(queue, 20, (void*)NULL);
	if(stage56_result == true) stage_result(5, "resize", true, queue);
	else stage_result(5, "resize", false, queue);

	arrayqueue_destroy(queue);
	stage_log(5, "END");

	/* 6: Max Node Memory: NULL Data */
	stage_log(6, "START");

	/* Test Data peekup */
	queue = arrayqueue_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	queue->enqueue(queue, string0);
	queue->enqueue(queue, string1);
	queue->enqueue(queue, string2);
	queue->enqueue(queue, string3);
	queue->enqueue(queue, string4);
	queue->enqueue(queue, string5);
	queue->enqueue(queue, string6);
	queue->enqueue(queue, string7);
	queue->enqueue(queue, string8);
	queue->enqueue(queue, string9);

	/* Test Procedure */

	bool stage61_result = queue->enqueue(queue, (void *)NULL);
	if(stage61_result == true) stage_result(6, "enqueue", true, queue);
	else stage_result(6, "enqueue", false, queue);

	void* stage62_result = queue->dequeue(queue);
	if(stage62_result != NULL) stage_result(6, "dequeue", true, queue);
	else stage_result(6, "dequeue", false, queue);

	void* stage63_result = queue->get(queue, (size_t)NULL);
	if(stage63_result != NULL) stage_result(6, "get", true, queue);
	else stage_result(6, "get", false, queue);

	void* stage64_result = queue->peek(queue);
	if(stage64_result != NULL) stage_result(6, "peek", true, queue);
	else stage_result(6, "peek", false, queue);

	bool stage65_result = queue->is_available(queue);
	if(stage65_result == true) stage_result(6, "enqueue_first", true, queue);
	else stage_result(6, "enqueue_first", false, queue);

	bool stage66_result = queue->resize(queue, (size_t)NULL, (void*)NULL);
	if(stage66_result == true) stage_result(6, "resize", true, queue);
	else stage_result(6, "resize", false, queue);

	arrayqueue_destroy(queue);
	stage_log(6, "END");

	/* 7: Max Node Memory: No Duplicated Data */
	stage_log(7, "START");

	/* Test Data peekup */
	queue = arrayqueue_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	queue->enqueue(queue, string0);
	queue->enqueue(queue, string1);
	queue->enqueue(queue, string2);
	queue->enqueue(queue, string3);
	queue->enqueue(queue, string4);
	queue->enqueue(queue, string5);
	queue->enqueue(queue, string6);
	queue->enqueue(queue, string7);
	queue->enqueue(queue, string8);
	queue->enqueue(queue, string9);

	/* Test Procedure */
	bool stage71_result = queue->enqueue(queue, string1);
	if(stage71_result == true) stage_result(7, "enqueue", true, queue);
	else stage_result(7, "enqueue", false, queue);

	void* stage72_result = queue->dequeue(queue);
	if(stage72_result != NULL) stage_result(7, "dequeue", true, queue);
	else stage_result(7, "dequeue", false, queue);

	void* stage73_result = queue->get(queue, 1);
	if(stage73_result != NULL) stage_result(7, "get", true, queue);
	else stage_result(7, "get", false, queue);

	void* stage74_result = queue->peek(queue);
	if(stage74_result != NULL) stage_result(7, "peek", true, queue);
	else stage_result(7, "peek", false, queue);

	bool stage75_result = queue->is_available(queue);
	if(stage75_result == true) stage_result(7, "enqueue_first", true, queue);
	else stage_result(7, "enqueue_first", false, queue);

	bool stage76_result = queue->resize(queue, 20, (void*)NULL);
	if(stage76_result == true) stage_result(7, "resize", true, queue);
	else stage_result(7, "resize", false, queue);

	arrayqueue_destroy(queue);
	stage_log(7, "END");

	/* 8: Max Node Memory: Duplicated Data */
	stage_log(8, "START");

	/* Test Data peekup */
	queue = arrayqueue_create(DATATYPE_STRING, POOLTYPE_LOCAL, 10);
	queue->enqueue(queue, string0);
	queue->enqueue(queue, string1);
	queue->enqueue(queue, string2);
	queue->enqueue(queue, string3);
	queue->enqueue(queue, string4);
	queue->enqueue(queue, string0);
	queue->enqueue(queue, string1);
	queue->enqueue(queue, string2);
	queue->enqueue(queue, string3);
	queue->enqueue(queue, string4);

	/* Test Procedure */
	bool stage81_result = queue->enqueue(queue, string1);
	if(stage81_result == true) stage_result(8, "enqueue", true, queue);
	else stage_result(8, "enqueue", false, queue);

	bool stage82_result = queue->dequeue(queue);
	if(stage82_result == true) stage_result(8, "dequeue", true, queue);
	else stage_result(8, "dequeue", false, queue);

	void* stage83_result = queue->get(queue, 1);
	if(stage83_result != NULL) stage_result(8, "get", true, queue);
	else stage_result(8, "get", false, queue);

	void* stage84_result = queue->peek(queue);
	if(stage84_result != NULL) stage_result(8, "peek", true, queue);
	else stage_result(8, "peek", false, queue);

	bool stage85_result = queue->is_available(queue);
	if(stage85_result == true) stage_result(8, "is_available", true, queue);
	else stage_result(8, "is_available", false, queue);

	bool stage86_result = queue->resize(queue, 20, (void*)NULL);
	if(stage86_result == true) stage_result(8, "resize", true, queue);
	else stage_result(8, "resize", false, queue);

	arrayqueue_destroy(queue);
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


static void stage_result(int stage, char* operation, bool opresult, ArrayQueue* queue) {
	time_t current_time;
	char* c_time_string;

	current_time = time(NULL);
	c_time_string = ctime(&current_time);

	if(opresult == true)
		fprintf(testresult, "(stage %d) %s is success.: %s", stage, operation, c_time_string);
	else
		fprintf(testresult, "(stage %d) %s is failed.: %s", stage, operation, c_time_string);
}
