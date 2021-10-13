// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

#include <time.h>
#include <sys/time.h>
#include <assert.h>

//for sqlite
#include "sqlite3.h"
static int sq_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	for (i=0; i<argc; i++) {
		fprintf(stderr,"%s = %s\n",
			azColName[i], argv[i] ? argv[i] : "NULL");
	}
	fprintf(stderr,"\n");
	return 0;
}

static inline int smp_processor_id(void)
{
#if 1
	return -1;
#else
	unsigned long eax, ebx,ecx, edx;
	__asm __volatile ("cpuid"
			  : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
			  : "a" (1));
	return ((ebx >> 24) & 0xff);
#endif
}

#define DEFAULT_TIMES 10000
#define INSERT_TIMES  (DEFAULT_TIMES)
//#define INSERT_TIMES 50
#define UPDATE_TIMES  (DEFAULT_TIMES)
#define DELETE_TIMES  (DEFAULT_TIMES)
#define QUERY_TIMES   (DEFAULT_TIMES)

#define LOG_ERR(fmt...) fprintf(stderr, fmt)

#define CHECK_SQLITE_OK(rc, zErrMsg)                                   \
do {                                                                   \
	if (rc != SQLITE_OK) {                                         \
		LOG_ERR("SQL error: %s\n", zErrMsg);                   \
		sqlite3_free(zErrMsg);                                 \
		LOG_ERR("sqlite3_exec error, extended errcode:%d\n",   \
			sqlite3_extended_errcode(db));                 \
		return -1;                                             \
	}                                                              \
} while (0)

const char *sql_create = "create table tb11(one varchar(10), two smallint);";
const char *sql_insert_tpl = "insert into tb11 values('hello%d',%d);";
const char *sql_update_tpl = "update tb11 set two = %d where one = 'hello%d';";
const char *sql_delete_tpl = "delete from tb11 where one = 'hello%d';";
const char *sql_query = "select * from tb11;\0";

#define BEGIN_TIMING() gettimeofday(&begin_time, NULL)
#define END_TIMING_REPORT(type, N)                                         \
do {                                                                       \
	long duration = 0;                                                 \
	gettimeofday(&end_time, NULL);                                     \
	duration = 1000000 * (end_time.tv_sec - begin_time.tv_sec) +       \
	           (end_time.tv_usec - begin_time.tv_usec);                \
	LOG_ERR("%s sqlite3 " #type "_test success:%d "                    \
	        #type ":time: %lu us, current_cpu = %d\n",                 \
		__func__, N, duration, smp_processor_id());                \
} while (0)

int sqlite3_test(sqlite3 *db)
{
	//sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char real_sql[128];

	rc = sqlite3_exec(db, sql_create, sq_callback, 0, &zErrMsg);
	CHECK_SQLITE_OK(rc, zErrMsg);

	struct timeval begin_time, end_time;

	//insert test
	BEGIN_TIMING();
	for (int i=0; i<INSERT_TIMES; i++) {
		int ret = snprintf(real_sql, 128, sql_insert_tpl, i, i);
		if (ret <= 0) {
			LOG_ERR("%s, snprintf error:ret:%d\n", __func__, ret);
			return -1;
		}
		rc = sqlite3_exec(db, real_sql, sq_callback, 0, &zErrMsg);
		CHECK_SQLITE_OK(rc, zErrMsg);
	}
	END_TIMING_REPORT(insert, INSERT_TIMES);

#if 1
	//update test
	BEGIN_TIMING();
	for (int i=0; i<UPDATE_TIMES; i++) {
		int ret = snprintf(real_sql, 128, sql_update_tpl, 0xd, i);
		if (ret <= 0) {
			LOG_ERR("%s, snprintf error:ret:%d\n", __func__, ret);
			return -1;
		}
		rc = sqlite3_exec(db, real_sql, sq_callback, 0, &zErrMsg);
		CHECK_SQLITE_OK(rc, zErrMsg);
	}
	END_TIMING_REPORT(update, UPDATE_TIMES);
#endif
	//query test
	BEGIN_TIMING();
	for (int i=0;i<QUERY_TIMES;i++){
		rc = sqlite3_exec(db, sql_query, NULL, 0, &zErrMsg);
		CHECK_SQLITE_OK(rc, zErrMsg);
	}
	END_TIMING_REPORT(query, QUERY_TIMES);
#if 1
	//delete test
	BEGIN_TIMING();
	for (int i=0; i<DELETE_TIMES; i++){
		int ret = snprintf(real_sql, 128, sql_delete_tpl, i);
		if (ret <= 0) {
			LOG_ERR("%s, snprintf error:ret:%d\n", __func__, ret);
			return -1;
		}
		rc = sqlite3_exec(db, real_sql, sq_callback, 0, &zErrMsg);
		CHECK_SQLITE_OK(rc, zErrMsg);
	}
	END_TIMING_REPORT(delete, DELETE_TIMES);
#endif

	return 0;
}

void *test_thread(void *arg)
{
	int i = *(int *)arg;
	char db_file[80] = { 0 };

	sprintf(db_file, "/fs-sqlite-test-%d.db", i);

	fprintf(stderr, " open = %lx, db_file = %s\n",
		open, db_file);

	sqlite3 *db;
	int rc = sqlite3_open(db_file, &db);
	if (rc) {
		fprintf(stderr, "Can't open database: %s\n",
			sqlite3_errmsg(db));
		sqlite3_close(db);
		return 0;
	}
	sqlite3_test(db);
	sqlite3_close(db);
	unlink(db_file);
	//    speed_test_main(sql_argc, (char**)sql_argv1);
	return 0;
}

int main(int argc, char** argv)
{
	printf("hello!\n");
	if (argc < 2) {
		LOG_ERR(" Usage: %s <thread_num>\n", argv[0]);
		exit(-1);
	}
#if 0
	pthread_t thread[20];
	int index[20];
	int nthreads = atoi(argv[1]);

	long long time[2];
	struct timespec tstart={0,0}, tend={0,0};
	clock_gettime(CLOCK_MONOTONIC, &tstart);
	clock_gettime(CLOCK_MONOTONIC, &tend);

	for (int i = 0; i < nthreads; i++) {
		index[i] = i;
		int res = pthread_create(&thread[i], NULL, test_thread,
					 index + i);
		if (res != 0) {
			fprintf(stderr, "create thread fail. status = %d\n",
				res);
		}
	}
	fprintf(stderr, "time address = %lx, open = %lx\n", time, open);

	for (int i = 0; i < nthreads; ++i) {
		pthread_join(thread[i], NULL);
	}
	clock_gettime(CLOCK_MONOTONIC, &tend);
	time[0] = ((long)tstart.tv_sec * 1e9 + tstart.tv_nsec);
	time[1] = ((long)tend.tv_sec * 1e9 + tend.tv_nsec);
	printf("Start time: %lld, end time: %lld latency: %lld.\n",
	       time[0], time[1], time[1]-time[0]);
	fprintf(stderr, "Done.\n");
#else
	test_thread((void *)1);
#endif
	return 0;
}
