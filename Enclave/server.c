#include "sqlite/sqlite3.h"

#include "sgxvfs.h"
#include <stdio.h>
#include <stdarg.h>

#include "../vme_config.h"

sqlite3 *gdb = NULL;

void *pHeap = NULL;
void *pPCache = NULL;

sgxvfs_db_t sdb2;

sqlite3 *opendb_sgx2() {
	char path[]="sgx";
	sgxbuffer_t *mem = (sgxbuffer_t*)calloc( sizeof( sgxbuffer_t ), 1 );
	sgxvfs_env_init();

	printf("open db in %s mode\n", path);

	sgxvfs_open_db( &sdb2, path, mem );

	return sdb2.handle;
}

sqlite3 *opendb_vnl2() {
	int rc;
	sqlite3 *db;

	rc = sqlite3_open(":memory:", &db);
	if( rc ) {
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
		while(1);
	} else
	    printf("open db in :memory: mode \n", 1);

	return db;
}

void closedb_sgx2(sqlite3 *db) {
	sgxvfs_close_db( &sdb2 );
	sgxvfs_env_fini();
}

void closedb_vnl2(sqlite3 *db) {
	sqlite3_close(db);
}

int open_db() {
	int rc; 

	printf("hello init gdb\n",0);

	if(gdb == -1 || gdb == NULL) {
#ifndef SGX_ENGINE
		gdb = opendb_vnl2();
#else
		gdb = opendb_sgx2();
#endif
	}

	return 0;
}


int close_db() {
	int rc; 
	printf("hello close gdb\n",0);
#ifndef SGX_ENGINE
	closedb_vnl2(gdb);
#else
	closedb_sgx2(gdb);
#endif
	printf("Database has been closed successfully\n",0);

	if(pHeap)
		free(pHeap);
	if(pPCache)
		free(pPCache);
	gdb = -1;

	return 0;
}

#if 0

#include "tpcc.h"
#include "inc/ippcp.h"

#include "../vme_config.h"
#define BUF_SIZE	(1*1024*1024)

int first = 0;
int shift = 0;

#ifdef JSON
static int callback(void *data, int argc, char **argv, char **azColName) {
	int i;
	int adds;
	if(gdb<= 0)
		return 1;
//there is an overflow here because of cascades of callback. rework is needed
	if(shift > MRQ)
		return 0;

	shift+=snprintf(&data[shift], (MRQ-3)-shift, "{\n");
	for(i = 0; i<argc; i++){
		shift+=snprintf(&data[shift], (MRQ-3)-shift, "\"%s\": \"%s\",\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}

	shift+=snprintf(&data[shift], 3, "}\n");
	return 0;
}
#else
static int callback(void *data, int argc, char **argv, char **azColName) {
	int i;
	int adds;
	if(gdb<= 0)
		return 1;
//there is an overflow here because of cascades of callback. rework is needed
	if(shift > MRQ) {
		printf("shift = %d/%d\n", shift, MRQ);
		return 0;
	}
	for(i = 0; i<argc; i++){
		shift+=snprintf(&data[shift], (MRQ-3)-shift, "%s,",argv[i] ? argv[i] : "NULL");
	}
	shift+=snprintf(&data[shift], (MRQ-3)-shift, " ^ ");
	return 0;
}
#endif

unsigned int sn = 0;

struct s_req {
	char data[MRQ];
	unsigned int data_size;
	unsigned int sn;
	unsigned long long hash;
}  __attribute__ ((aligned (64)));

struct s_req req;

struct packet {
	char data[BUF_SIZE];
	unsigned int size;
};


unpack(char *from, char *to) {

   // secret key
   Ipp8u key[] = "\x00\x01\x02\x03\x04\x05\x06\x07"
                 "\x08\x09\x10\x11\x12\x13\x14\x15";
   // define and setup AES cipher
   int ctxSize;
   ippsAESGetSize(&ctxSize);
	IppsAESSpec *pAES = (IppsAESSpec*)malloc(ctxSize);
   ippsAESInit(key, sizeof(key)-1, pAES, ctxSize);

   // and initial counter
   Ipp8u ctr0[] = "\xff\xee\xdd\xcc\xbb\xaa\x99\x88"
                  "\x77\x66\x55\x44\x33\x22\x11\x00";

   // counter
   Ipp8u ctr[16];

   // init counter before encryption
   memcpy(ctr, ctr0, sizeof(ctr));

   ippsAESDecryptCBC(from, to, sizeof(struct s_req), pAES, ctr);

   // remove secret and release resource
//   ippsAESInit(0, sizeof(key)-1, pAES, ctxSize);
//   delete [] (Ipp8u*)pAES;
    free(pAES);
}

#ifdef SSL
void sqlite_request(void *in, void *out) {
	char *zErrMsg = 0;
	int rc, i;
	char sql[50];

//	printf("incomming %s\n", in);

	unpack(in, &req);

	if((req.hash&0xffffffff) != (XXH64(&req, (unsigned long int) &req.hash-(unsigned long int) &req, 0xabbacaca) & 0xffffffff) || req.sn<sn++) {
		printf("[%d]we are hacked -- hash missmatch or wrong SQ\n",__LINE__);
		printf("req.hash = %llx\t %llx\n", req.hash, (XXH64(&req, (unsigned long int) &req.hash-(unsigned long int) &req, 0xabbacaca) & 0xffffffff));
		while(1);
	}

//	printf("req.data = %s\n", req.data);
	if( !strncmp((const char*)req.data, "open", 4)) {
		open_db();
		snprintf(out, 3, "ok");
		return;
	}

	if( !strncmp((const char*)req.data, "tpcc", 4)) {
		snprintf(sql, 50, "PRAGMA page_size=%d", PAGE_SIZE);
		sqlite3_exec(gdb, sql, NULL, 0, &zErrMsg);

#ifndef SGX_ENGINE
		snprintf(sql, 50, "PRAGMA cache_size=%d", COLD_MEM_SIZE);
		sqlite3_exec(gdb, sql, NULL, 0, &zErrMsg);
#endif

		for(i=0; i<12; i++) {
			rc = sqlite3_exec(gdb, t[i], callback, out, &zErrMsg);
			if( rc )
				printf("Can't exec request [%d] %s: %s\n",i, t[i], zErrMsg);
		}
		snprintf(out, 3, "ok");
		return 0;
	}


	if( !strncmp((const char*)req.data, "close", 5)) {
		close_db();
		snprintf(out, 3, "ok");
		return;
	}


#if 1
	shift +=snprintf(&out[shift], MRQ-shift, "[\n");
//printf("executing %s\n", req.data);
	rc = sqlite3_exec(gdb, req.data, callback, out, &zErrMsg);
	if( rc ) {
		printf("Can't exec request %s: %s\n", req.data, zErrMsg);
	} else {
//		printf("ok \n",0);
	}
	shift +=snprintf(&out[shift], MRQ-shift,"\n]\n");
#else
		snprintf(out, 3, "ok");
#endif
	shift = 0;
}

#else

int sqlite_request(void *in, void *out) {
	char *zErrMsg = 0;
	int rc,i;
	char sql[50];

//	printf("incomming %s\n", in);

	if( !strncmp((const char*)in, "open", 4)) {
		open_db();
		snprintf(out, 3, "ok");
		return 0;
	}

	if( !strncmp((const char*)in, "tpcc", 4)) {
		snprintf(sql, 50, "PRAGMA page_size=%d", PAGE_SIZE);
		sqlite3_exec(gdb, sql, NULL, 0, &zErrMsg);

#ifndef SGX_ENGINE
		snprintf(sql, 50, "PRAGMA cache_size=%d", COLD_MEM_SIZE);
		sqlite3_exec(gdb, sql, NULL, 0, &zErrMsg);
#endif

		for(i=0; i<12; i++) {
			rc = sqlite3_exec(gdb, t[i], callback, out, &zErrMsg);
			if( rc )
				printf("Can't exec request [%d] %s: %s\n",i, t[i], zErrMsg);
		}
		snprintf(out, 3, "ok");
		return 0;
	}

	if( !strncmp((const char*)in, "close", 5)) {
		close_db();
		snprintf(out, 3, "ok");
		return 1;
	}


#if 1
	shift +=snprintf(&out[shift], MRQ-shift, "");
	rc = sqlite3_exec(gdb, in, callback, out, &zErrMsg);
	if( rc ) {
		printf("Can't exec request %s: %s\n", in, zErrMsg);
	} else {
//		printf("ok \n",0);
	}
	shift +=snprintf(&out[shift], MRQ-shift,"\n");
#else
		snprintf(out, 3, "ok");
#endif
	shift = 0;
	return 0;
}
#endif

void  requester(void *rpack_in, void *rpack_out) {
	volatile struct packet *in=(struct packet *) (rpack_in);
	struct packet *out=(struct packet *) (rpack_out);
	while(1) {
		while(!in->size) {
//			asm("nop");
		}
		sqlite_request(in->data, out->data);

		in->size=0;
	}
}

#endif