#include "sqlite/sqlite3.h"
#include "sgxvfs.h"
#include <stdio.h>      /* vsnprintf */

#include "../vme_config.h"

#define	VANILLA	0
#define	SGX	1

#define	INSERT	0
#define	SELECT	1

sgxvfs_db_t sdb;

sqlite3 *opendb_sgx() {
	char path[]="sgx";
	sgxbuffer_t *mem = (sgxbuffer_t*)calloc( sizeof( sgxbuffer_t ), 1 );
	sgxvfs_env_init();

	sgxvfs_open_db( &sdb, path, mem );

	char *err = 0;
	char sql[50];

	snprintf(sql, 50, "PRAGMA page_size=%d", PAGE_SIZE);
	sqlite3_exec(sdb.handle, sql, NULL, 0, &err);


	return sdb.handle;
}

sqlite3 *opendb_vnl() {
	int rc;
	sqlite3 *db;

	rc = sqlite3_open(":memory:", &db);
	if( rc ) {
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
		while(1);
	}// else
//	    printf("open db %x\n", db);

	char *err = 0;
	char sql[50];

	snprintf(sql, 50, "PRAGMA page_size=%d", PAGE_SIZE);
	sqlite3_exec(db, sql, NULL, 0, &err);

	return db;
}

void closedb_sgx(sqlite3 *db) {
	sgxvfs_close_db( &sdb );
	sgxvfs_env_fini();
}

void closedb_vnl(sqlite3 *db) {
	sqlite3_close(db);
}

static int callback(void *data, int argc, char **argv, char **azColName){
	int i;
	printf("%s: ", (const char*)data);

	for(i = 0; i<argc; i++){
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}

	printf("\n", 1);
	return 0;
}

void synthetic(char engine, char test) {
	sqlite3 *db;
	int rc, i, j;
	char *err = 0;

	char sql[] =	"CREATE TABLE STEST("  \
		"ID INTEGER PRIMARY KEY     AUTOINCREMENT NOT NULL," \
		"BODY        CHAR(1024));";

	char sql2[] = "INSERT INTO STEST (BODY) "  \
		"VALUES ('Lorem ipsum dolor sit amet, consectetur adipiscing elit. " \
		"Donec nunc lacus, suscipit eget dolor vel, sodales sagittis nisl. " \
		"Nulla pellentesque nisi vel nibh pretium bibendum. Integer efficitur " \
		"vehicula dui, sit amet  ametvehicula justo iaculis quis. Ut non egestas purus. " \
		"Sed vel metus in dui vestibulum interdum. Phasellus a tellus venenatis, " \
		"maximus mauris  mauri ac, facilisis tortor. Vivamus pharetra facilisis tempor. " \
		"Vivamus dictum dui vitae erat lobortis consequat. Aenean bibendum dolor a volutpat varius. " \
		"Sed non velit at lacus hendrerit hendrerit ut ac nisl. Donec non tortor neque. " \
		"Nunc sed sapien nunc. Donec ante purus, volutpat at pretium sed, pretium in nibh. " \
		"Vivamus dictum dui vitae erat lobortis consequat. Aenean bibendum dolor a volutpat varius. " \
		"Sed suscipit mattis augue, sit amet viverra elit bibendum a. Pellentesque eu accumsan " \
		"diam, vitae mollis erat. Nulla pharetra massa mauris. Suspendisse fringilla" \
		"sodales at lacinia vitae, mollis vitae posuere.'); ";

	char sql3[] = "DROP TABLE STEST ";

	char sql5[] = "SELECT * FROM STEST ORDER BY RANDOM() LIMIT 1";

	for(j = 1; j <52;j++) {
//open
		if(engine == VANILLA) {
			db = opendb_vnl();
		} else if(engine == SGX) {
			db = opendb_sgx();
		}
//init
		rc = sqlite3_exec(db, sql, NULL, 0, &err);
		if( rc != SQLITE_OK ) {
			printf("SQL error: %s\n", err);
			sqlite3_free(err);
			while(1);
		}
//fill
		if(test == INSERT)
			speedtest1_begin_test(100, "\t%d\t INSERTs into table with no index", j*10000);

		for(i = 0; i < j*10000; i++) {
			rc = sqlite3_exec(db, sql2, NULL, 0, &err);
			if( rc != SQLITE_OK ) {
				printf("SQL error: %s\n", err);
				sqlite3_free(err);
				while(1);
			}
		}

		if(test == INSERT)
		    speedtest1_end_test();
// lookup
		if(test == SELECT)
			speedtest1_begin_test(100, "\t%d\t random lookup", j*10000);
		for(i=0;i <10;i++) {
			rc = sqlite3_exec(db, sql5, NULL, 0, &err);
			if( rc != SQLITE_OK ) {
				printf("SQL error: %s\n", err);
				sqlite3_free(err);
				while(1);
			} 
		}
		if(test == SELECT)
			speedtest1_end_test();
// close
		if(engine == VANILLA) {
			closedb_vnl(db);
		} else if(engine == SGX) {
			closedb_sgx(db);
		}
	}
}
