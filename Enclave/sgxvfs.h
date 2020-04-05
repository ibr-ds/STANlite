#ifdef __cplusplus
extern "C" {
#endif

#include "sqlite/sqlite3.h"

#define NAME "sgx"

typedef struct sgxbuffer_t {
	char * data;
	int used;
	int total;
} sgxbuffer_t;

typedef struct sgxvfs_db_t {
	sqlite3 * handle;
	sgxbuffer_t * mem;
} sgxvfs_db_t;

int sgxvfs_env_init();

void sgxvfs_env_fini();

int sgxvfs_open_db( sgxvfs_db_t * db, const char * path, sgxbuffer_t * mem );

int sgxvfs_close_db( sgxvfs_db_t * db );

#ifdef __cplusplus
}
#endif

