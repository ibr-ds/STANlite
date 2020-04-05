#include "sqlite/sqlite3.h"
#include <time.h>

localtime(time_t *t)
{

}

int sqlite3_os_init(void) {
	sgxvfs_env_init();
	return SQLITE_OK; 
}


int sqlite3_os_end(void) {
	return SQLITE_OK; 
}
