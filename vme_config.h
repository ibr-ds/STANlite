// Uncomment to enable VME.
//#define SGX_ENGINE

/****** VME configuration *******/

//tested page sizes with SQLite: 2, 4, 8 
//rest requires modification for unaligned access
#define PAGE_SIZE	(1024*4)

// pools
//#define COLD_MEM_SIZE	(8*256*1024*1024)
#define COLD_MEM_SIZE	(0x7fffffff)
#define WARM_MEM_SIZE	(70*1024*1024)
#define COLD_PAGES	(COLD_MEM_SIZE/PAGE_SIZE)
#define WARM_PAGES	(WARM_MEM_SIZE/PAGE_SIZE)

// VME features
// nnI (--I):					COLD_HASH
// CnI (C-I):	WARM_CACHE			COLD_HASH
// CFI (CFI):	WARM_CACHE	WARM_FETCH	COLD_HASH

#define WARM_CACHE
#define WARM_FETCH
#define COLD_HASH


//for testing only, do not use
//#define CPY


/******* SPEED TESTS *******/
// Uncomment to enable
#define SPD_TEST

// Payload size. use NULL_PL if you don't know what it is.
//#define FAT_PL
//#define THIN_PL
#define NULL_PL

#ifdef FAT_PL
#define SMALL	40
#define MEDIUM	150
#define BIG	500
#endif

#ifdef THIN_PL
#define SMALL	100
#define MEDIUM	300
#define BIG	1000
#endif

#ifdef NULL_PL
#define SMALL	200
#define MEDIUM	300
#define BIG	2000
#endif

//#define	SPD_TEST_SIZE SMALL
//#define	SPD_TEST_SIZE MEDIUM
#define	SPD_TEST_SIZE BIG

/******* VNL TESTS *******/
// Instead of speedtest1, you can run these tests
//#define VNL_SYNTH
//#define VME_SYNTH
