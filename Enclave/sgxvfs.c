//based on spmemvfs.c


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "sgxvfs.h"

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))

#define MAX_PATH 256

static void Debug(const char *format, ...){

#if defined(DEBUG)

	char logTemp[ 1024 ] = { 0 };

	va_list vaList;
	va_start( vaList, format );
	vsnprintf( logTemp, sizeof( logTemp ), format, vaList );
	va_end ( vaList );

	if( strchr( logTemp, '\n' ) ) {
		printf( "%s", logTemp,1);
	} else {
		printf( "%s\n", logTemp,1);
	}
#endif

}

//===========================================================================
extern void *page_up(int page); 


extern void ext_encrypt(char *, char *,int size);
extern void ext_decrypt(char *, char *,int size);

extern int copy_from(char *, int,int size);
extern int copy_to(int, char *,int size);

extern char *cold;
//===========================================================================
typedef struct sgxVmem_t {
	sqlite3_file base;
	char * path;
	int flags;
	sgxbuffer_t * mem;
} sgxVmem_t;

static int sgxVmemClose( sqlite3_file * file );
static int sgxVmemRead( sqlite3_file * file, void * buffer, int len, sqlite3_int64 offset );
static int sgxVmemWrite( sqlite3_file * file, const void * buffer, int len, sqlite3_int64 offset );
static int sgxVmemTruncate( sqlite3_file * file, sqlite3_int64 size );
static int sgxVmemSync( sqlite3_file * file, int flags );
static int sgxVmemFileSize( sqlite3_file * file, sqlite3_int64 * size );
static int sgxVmemLock( sqlite3_file * file, int type );
static int sgxVmemUnlock( sqlite3_file * file, int type );
static int sgxVmemCheckReservedLock( sqlite3_file * file, int * result );
static int sgxVmemFileControl( sqlite3_file * file, int op, void * arg );
static int sgxVmemSectorSize( sqlite3_file * file );
static int sgxVmemDeviceCharacteristics( sqlite3_file * file );
static int sgxVmemShmLock(sqlite3_file*,int,int,int);
static int sgxVmemShmMap(sqlite3_file*,int,int,int, void volatile **);
static void sgxVmemShmBarrier(sqlite3_file*);
static int sgxVmemShmUnmap(sqlite3_file*,int);
static int sgxVmemFetch(sqlite3_file *pFile, sqlite3_int64 iOfst, int iAmt, void **pp);
static int sgxVmemUnfetch(sqlite3_file *pFile, sqlite3_int64 iOfst, void *pPage);


static sqlite3_io_methods g_sgxVmem_io_memthods = {
	3,                                  /* iVersion */
	sgxVmemClose,                     /* xClose */
	sgxVmemRead,                      /* xRead */
	sgxVmemWrite,                     /* xWrite */
	sgxVmemTruncate,                  /* xTruncate */
	sgxVmemSync,                      /* xSync */
	sgxVmemFileSize,                  /* xFileSize */
	sgxVmemLock,                      /* xLock */
	sgxVmemUnlock,                    /* xUnlock */
	sgxVmemCheckReservedLock,         /* xCheckReservedLock */
	sgxVmemFileControl,               /* xFileControl */
	sgxVmemSectorSize,                /* xSectorSize */
	sgxVmemDeviceCharacteristics,      /* xDeviceCharacteristics */
	sgxVmemShmMap,                     /* xShmMap */
	sgxVmemShmLock,                    /* xShmLock */
	sgxVmemShmBarrier,                 /* xShmBarrier */
	sgxVmemShmUnmap,                    /* xShmUnmap */
	sgxVmemFetch,                      /* xFetch */
	sgxVmemUnfetch                     /* xUnfetch */
};

int sgxVmemClose( sqlite3_file * file )
{
	sgxVmem_t * memfile = (sgxVmem_t*)file;

	Debug( "call %s( %p )", __func__, memfile );

	if( SQLITE_OPEN_MAIN_DB & memfile->flags ) {
		// noop
	} else {
		if( NULL != memfile->mem ) {
			if( memfile->mem->data ) free( memfile->mem->data );
			free( memfile->mem );
		}
	}

	free( memfile->path );

	return SQLITE_OK;
}

int sgxVmemRead( sqlite3_file * file, void * buffer, int len, sqlite3_int64 offset )
{
	sgxVmem_t * memfile = (sgxVmem_t*)file;

	Debug( "call %s( %p, ..., %d, %lld ), len %d",
		__func__, memfile, len, offset, memfile->mem->used );

	if( ( offset + len ) > memfile->mem->used ) {
		return SQLITE_IOERR_SHORT_READ;
	}

	if( SQLITE_OPEN_MAIN_DB & memfile->flags ) {
		if(copy_from(buffer, offset, len)) {
			printf("intrusion detected at address %x\n", offset);
			while(1);
		}
	} else
		memcpy( buffer, memfile->mem->data + offset, len );


	return SQLITE_OK;
}

sgxbuffer_t *gmem = NULL;

int sgxVmemWrite( sqlite3_file * file, const void * buffer, int len, sqlite3_int64 offset )
{
	sgxVmem_t * memfile = (sgxVmem_t*)file;
	sgxbuffer_t * mem = memfile->mem;

//	printf( "[%d] %s( %p, ..., %d, %lld ), len %d\n", SQLITE_OPEN_MAIN_DB & memfile->flags,
//		__func__, memfile, len, offset, mem->used );

	if( ( offset + len ) > mem->total ) {
		if( SQLITE_OPEN_MAIN_DB & memfile->flags ) {
//			printf("mem->total = %x\n", mem->total);
			if(mem->total == 0) {
				mem->total = vmem_max();
				gmem = mem;
			} else {
				printf("oom\n",1);while(1);
			}
		} else {
			int newTotal = 2 * ( offset + len + mem->total );
			char * newBuffer = (char*)realloc( mem->data, newTotal );
			if( NULL == newBuffer ) {
				return SQLITE_NOMEM;
			}

			mem->total = newTotal;
			mem->data = newBuffer;
		}
	}



	if( SQLITE_OPEN_MAIN_DB & memfile->flags ) {
		if (copy_to(offset, buffer, len)) {
			printf("intrusion detected at address %x\n", offset);
			while(1);
		}
	} else
		memcpy( mem->data + offset, buffer, len );


	mem->used = MAX( mem->used, offset + len );

	return SQLITE_OK;
}

int sgxVmemTruncate( sqlite3_file * file, sqlite3_int64 size )
{
	sgxVmem_t * memfile = (sgxVmem_t*)file;

	Debug( "call %s( %p )", __func__, memfile );

	memfile->mem->used = MIN( memfile->mem->used, size );

	return SQLITE_OK;
}

int sgxVmemSync( sqlite3_file * file, int flags )
{
	Debug( "call %s( %p )", __func__, file );

	return SQLITE_OK;
}

int sgxVmemFileSize( sqlite3_file * file, sqlite3_int64 * size )
{
	sgxVmem_t * memfile = (sgxVmem_t*)file;

	Debug( "call %s( %p )", __func__, memfile );

	* size = memfile->mem->used;

	return SQLITE_OK;
}

int sgxVmemLock( sqlite3_file * file, int type )
{
	Debug( "call %s( %p )", __func__, file );

	return SQLITE_OK;
}

int sgxVmemUnlock( sqlite3_file * file, int type )
{
	Debug( "call %s( %p )", __func__, file );

	return SQLITE_OK;
}

int sgxVmemCheckReservedLock( sqlite3_file * file, int * result )
{
	Debug( "call %s( %p )", __func__, file );

	*result = 0;

	return SQLITE_OK;
}

int sgxVmemFileControl( sqlite3_file * file, int op, void * arg )
{
	Debug( "call %s( %p ), op = %d", __func__, file , op);
  return SQLITE_NOTFOUND;
//	return SQLITE_OK;
}

unsigned int sector_size = 0;

int sgxVmemSectorSize( sqlite3_file * file )
{
	Debug( "call %s( %p )", __func__, file );
	sector_size = vmem_sector_size();
	return sector_size;
//	return 0;
}

int sgxVmemDeviceCharacteristics( sqlite3_file * file )
{
	Debug( "call %s( %p )", __func__, file );

	return 0;
}

//===========================================================================
/*
** Shared-memory methods are all pass-thrus.
*/
static int sgxVmemShmLock(sqlite3_file *pFile, int ofst, int n, int flags){
printf("%s\t%d\n",__func__,__LINE__);while(1);
return SQLITE_OK;
}
static int sgxVmemShmMap(
  sqlite3_file *pFile, 
  int iRegion, 
  int szRegion, 
  int isWrite, 
  void volatile **pp
){
printf("%s\t%d\n",__func__,__LINE__);while(1);
return SQLITE_OK;

}
static void sgxVmemShmBarrier(sqlite3_file *pFile){
printf("%s\t%d\n",__func__,__LINE__);while(1);
return;

}
static int sgxVmemShmUnmap(sqlite3_file *pFile, int delFlag){
printf("%s\t%d\n",__func__,__LINE__);while(1);
return SQLITE_OK;
}

///

///
static int sgxVmemFetch(
  sqlite3_file *pFile,
  sqlite3_int64 iOfst,
  int iAmt,
  void **pp
){
	sgxVmem_t * memfile = (sgxVmem_t*)pFile;
	sgxbuffer_t * mem = memfile->mem;

	*pp = 0;

	if( SQLITE_OPEN_MAIN_DB & memfile->flags ) {
//		printf("%s\t%d, chunk size = %d, offset = %d\n",__func__,__LINE__, iAmt, iOfst/sector_size);
		if(iOfst + iAmt <= mem->total) {
			*pp = (void*)(page_up(iOfst/sector_size));
//			*pp = (void*)(&cold[iOfst]);
		}
	}

	return SQLITE_OK;
}


/* Release a memory-mapped page */
static int sgxVmemUnfetch(sqlite3_file *pFile, sqlite3_int64 iOfst, void *pPage){
	page_down(iOfst/sector_size);
return SQLITE_OK;

}

//===========================================================================

typedef struct sgxvfs_cb_t {
	void * arg;
	sgxbuffer_t * ( * load ) ( void * args, const char * path );
} sgxvfs_cb_t;

typedef struct sgxvfs_t {
	sqlite3_vfs base;
	sgxvfs_cb_t cb;
} sgxvfs_t;

static int sgxvfsOpen( sqlite3_vfs * vfs, const char * path, sqlite3_file * file, int flags, int * outflags );
static int sgxvfsDelete( sqlite3_vfs * vfs, const char * path, int syncDir );
static int sgxvfsAccess( sqlite3_vfs * vfs, const char * path, int flags, int * result );
static int sgxvfsFullPathname( sqlite3_vfs * vfs, const char * path, int len, char * fullpath );
static void * sgxvfsDlOpen( sqlite3_vfs * vfs, const char * path );
static void sgxvfsDlError( sqlite3_vfs * vfs, int len, char * errmsg );
static void ( * sgxvfsDlSym ( sqlite3_vfs * vfs, void * handle, const char * symbol ) ) ( void );
static void sgxvfsDlClose( sqlite3_vfs * vfs, void * handle );
static int sgxvfsRandomness( sqlite3_vfs * vfs, int len, char * buffer );
static int sgxvfsSleep( sqlite3_vfs * vfs, int microseconds );
static int sgxvfsCurrentTime( sqlite3_vfs * vfs, double * result );

static sgxvfs_t g_sgxvfs = {
	{
		1,                                           /* iVersion */
		0,                                           /* szOsFile */
		0,                                           /* mxPathname */
		0,                                           /* pNext */
		NAME,                               /* zName */
		0,                                           /* pAppData */
		sgxvfsOpen,                                /* xOpen */
		sgxvfsDelete,                              /* xDelete */
		sgxvfsAccess,                              /* xAccess */
		sgxvfsFullPathname,                        /* xFullPathname */
		sgxvfsDlOpen,                              /* xDlOpen */
		sgxvfsDlError,                             /* xDlError */
		sgxvfsDlSym,                               /* xDlSym */
		sgxvfsDlClose,                             /* xDlClose */
		sgxvfsRandomness,                          /* xRandomness */
		sgxvfsSleep,                               /* xSleep */
		sgxvfsCurrentTime                          /* xCurrentTime */
	}, 
	{ 0 },
};

int sgxvfsOpen( sqlite3_vfs * vfs, const char * path, sqlite3_file * file, int flags, int * outflags )
{
	sgxvfs_t * memvfs = (sgxvfs_t*)vfs;
	sgxVmem_t * memfile = (sgxVmem_t*)file;

//	printf( "call %s( %p(%p), %s, %p, %x, %p )\n",
//			__func__, vfs, &g_sgxvfs, path, file, flags, outflags );

	memset( memfile, 0, sizeof( sgxVmem_t ) );
	memfile->base.pMethods = &g_sgxVmem_io_memthods;
	memfile->flags = flags;

	if(path != NULL)
		memfile->path = strdup( path );

	if( SQLITE_OPEN_MAIN_DB & memfile->flags ) {
		memfile->mem = memvfs->cb.load( memvfs->cb.arg, path );
	} else {
		memfile->mem = (sgxbuffer_t*)calloc( sizeof( sgxbuffer_t ), 1 );
	}

	return memfile->mem ? SQLITE_OK : SQLITE_ERROR;
}

int sgxvfsDelete( sqlite3_vfs * vfs, const char * path, int syncDir )
{
	Debug( "call %s( %p(%p), %s, %d )\n",
			__func__, vfs, &g_sgxvfs, path, syncDir );

	return SQLITE_OK;
}

int sgxvfsAccess( sqlite3_vfs * vfs, const char * path, int flags, int * result )
{
	* result = 0;
	return SQLITE_OK;
}

int sgxvfsFullPathname( sqlite3_vfs * vfs, const char * path, int len, char * fullpath )
{
	strncpy( fullpath, path, len );
	fullpath[ len - 1 ] = '\0';

	return SQLITE_OK;
}

void * sgxvfsDlOpen( sqlite3_vfs * vfs, const char * path )
{
	return NULL;
}

void sgxvfsDlError( sqlite3_vfs * vfs, int len, char * errmsg )
{
	// noop
}

void ( * sgxvfsDlSym ( sqlite3_vfs * vfs, void * handle, const char * symbol ) ) ( void )
{
	return NULL;
}

void sgxvfsDlClose( sqlite3_vfs * vfs, void * handle )
{
	// noop
}

int sgxvfsRandomness( sqlite3_vfs * vfs, int len, char * buffer )
{
	return SQLITE_OK;
}

int sgxvfsSleep( sqlite3_vfs * vfs, int microseconds )
{
	return SQLITE_OK;
}

int sgxvfsCurrentTime( sqlite3_vfs * vfs, double * result )
{
	int64_t time;
	ocall_get_time(&time);
	*result = (double)time/86400000.0;
	return SQLITE_OK;
}

//===========================================================================

int sgxvfs_init( sgxvfs_cb_t * cb )
{
	g_sgxvfs.base.mxPathname = MAX_PATH;
	g_sgxvfs.base.szOsFile = sizeof( sgxVmem_t );

	g_sgxvfs.cb = * cb;

	return sqlite3_vfs_register( (sqlite3_vfs*)&g_sgxvfs, 0 );
}

//===========================================================================

typedef struct sgxbuffer_link_t {
	char * path;
	sgxbuffer_t * mem;
	struct sgxbuffer_link_t * next;
} sgxbuffer_link_t;

sgxbuffer_link_t * sgxbuffer_link_remove( sgxbuffer_link_t ** head, const char * path )
{
	sgxbuffer_link_t * ret = NULL;

	sgxbuffer_link_t ** iter = head;
	for( ; NULL != *iter; ) {
		sgxbuffer_link_t * curr = *iter;

		if( 0 == strcmp( path, curr->path ) ) {
			ret = curr;
			*iter = curr->next;
			break;
		} else {
			iter = &( curr->next );
		}
	}

	return ret;
}

void sgxbuffer_link_free( sgxbuffer_link_t * iter )
{
	free( iter->path );
	free( iter->mem->data );
	free( iter->mem );
	free( iter );
}

//===========================================================================

typedef struct sgxvfs_env_t {
	sgxbuffer_link_t * head;
	sqlite3_mutex * mutex;
} sgxvfs_env_t;

static sgxvfs_env_t * g_sgxvfs_env = NULL;

static sgxbuffer_t * load_cb( void * arg, const char * path )
{
	sgxbuffer_t * ret = NULL;

	sgxvfs_env_t * env = (sgxvfs_env_t*)arg;

#if SQLITE_THREADSAFE
	sqlite3_mutex_enter( env->mutex );
#endif
	{
		sgxbuffer_link_t * toFind = sgxbuffer_link_remove( &( env->head ), path );

		if( NULL != toFind ) {
			ret = toFind->mem;
			free( toFind->path );
			free( toFind );
		}
	}
#if SQLITE_THREADSAFE
	sqlite3_mutex_leave( env->mutex );
#endif

	return ret;
}

int sgxvfs_env_init()
{
	int ret = 0;

	if( NULL == g_sgxvfs_env ) {
		sgxvfs_cb_t cb;

		g_sgxvfs_env = (sgxvfs_env_t*)calloc( sizeof( sgxvfs_env_t ), 1 );
#if SQLITE_THREADSAFE
		g_sgxvfs_env->mutex = sqlite3_mutex_alloc( SQLITE_MUTEX_FAST );
#endif

		cb.arg = g_sgxvfs_env;
		cb.load = load_cb;

		ret = sgxvfs_init( &cb );
	}

	return ret;
}

void sgxvfs_env_fini()
{
	if( NULL != g_sgxvfs_env ) {
		sgxbuffer_link_t * iter = NULL;

		sqlite3_vfs_unregister( (sqlite3_vfs*)&g_sgxvfs );

#if SQLITE_THREADSAFE
		sqlite3_mutex_free( g_sgxvfs_env->mutex );
#endif

		iter = g_sgxvfs_env->head;
		for( ; NULL != iter; ) {
			sgxbuffer_link_t * next = iter->next;

			sgxbuffer_link_free( iter );

			iter = next;
		}

		free( g_sgxvfs_env );
		g_sgxvfs_env = NULL;
	}
}

extern void cache_reset();

int sgxvfs_open_db( sgxvfs_db_t * db, const char * path, sgxbuffer_t * mem )
{
	int ret = 0;

	sgxbuffer_link_t * iter = NULL;

	memset( db, 0, sizeof( sgxvfs_db_t ) );

	iter = (sgxbuffer_link_t*)calloc( sizeof( sgxbuffer_link_t ), 1 );
	iter->path = strdup( path );
	iter->mem = mem;

	cache_reset();

#if SQLITE_THREADSAFE
	sqlite3_mutex_enter( g_sgxvfs_env->mutex );
#endif
	{
		iter->next = g_sgxvfs_env->head;
		g_sgxvfs_env->head = iter;
	}
#if SQLITE_THREADSAFE
	sqlite3_mutex_leave( g_sgxvfs_env->mutex );
#endif

	ret = sqlite3_open_v2( path, &(db->handle),
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NAME );

	if( 0 == ret ) {
		db->mem = mem;
	} else {
#if SQLITE_THREADSAFE
		sqlite3_mutex_enter( g_sgxvfs_env->mutex );
#endif
		{
			iter = sgxbuffer_link_remove( &(g_sgxvfs_env->head), path );
			if( NULL != iter ) sgxbuffer_link_free( iter );
		}
#if SQLITE_THREADSAFE
		sqlite3_mutex_leave( g_sgxvfs_env->mutex );
#endif
	}

	return ret;
}

int sgxvfs_close_db( sgxvfs_db_t * db )
{
	int ret = 0;

	if( NULL == db ) return 0;

	if( NULL != db->handle ) {
		ret = sqlite3_close( db->handle );
		db->handle = NULL;
	}



	if( NULL != db->mem ) {
		if( NULL != db->mem->data ) free( db->mem->data );
		free( db->mem );
		db->mem = NULL;
	}

	return ret;
}

