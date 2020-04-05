#include "Enclave_t.h"
#include "xxhash.h"



#if 0
#ifdef __cplusplus
extern "C" {
#endif
#endif
void pre_push(const unsigned char *data, int len);
unsigned int get_free_warm();
unsigned int get_free_cold(int count);
void *page_up(int page);
void vmem_fill(int size);

#include "../vme_config.h"

#if (defined(WARM_FETCH) && !defined(WARM_CACHE))
#error Fetch requires cache
#endif

#if (defined(COLD_HASH) && !defined(WARM_CACHE))
//#error hash works with cache only because of non-aligned access pattern -- still broken
#endif

char warm[WARM_MEM_SIZE] __attribute__ ((aligned (PAGE_SIZE)));
unsigned long long hashes[COLD_PAGES];
int pcs[COLD_PAGES];

char zero_page[PAGE_SIZE];

unsigned long int hits = 0;
unsigned long int misses = 0;

#if 0
#ifdef __cplusplus
}
#endif
#endif
/////////////////////
#include "inc/ippcp.h"
   Ipp8u key[] = "\x00\x01\x02\x03\x04\x05\x06\x07"
                 "\x08\x09\x10\x11\x12\x13\x14\x15";
IppsAESSpec* pAES;
   Ipp8u ctr0[] = "\xff\xee\xdd\xcc\xbb\xaa\x99\x88"
                  "\x77\x66\x55\x44\x33\x22\x11\x00";

///////////////////
#if 0
void *memset(void *s, int c, size_t n) {
    int i;
    char *dst=(char *) s;
    for(i=0; i< n; i++) {
	    dst[i]=c;
    }
    return NULL;
}
#endif
#if 1
void *memcpy2(void* dst, void* src, size_t size)
{
#if 0
	int i;
	long int *pdst = (long int *)dst;
	long int *psrc = (long int *)src;
	for (i = 0; i < size/sizeof(long int); i++)
		pdst[i] = psrc[i];

    return dst;
#else
    return memcpy(dst, src, size);
#endif
}
#endif
///////////////////
void set_free_warm(unsigned int page);
void cache_init(void *gcold);

/////////



// A Queue Node (Queue is implemented using Doubly Linked List)
typedef struct QNode {
	struct QNode *prev, *next;
	unsigned pageNumber;  // the page number stored in this QNode
	unsigned warm;
	char stick;
	unsigned int crc32;
} QNode;

// A Queue (A FIFO collection of Queue Nodes)
typedef struct Queue {
	unsigned count;  // Number of filled frames
	unsigned numberOfFrames; // total number of frames
	QNode *front, *rear;
} Queue;

// A hash (Collection of pointers to Queue Nodes)
typedef struct vHash {
	int capacity; // how many pages can be there
	QNode* *array; // an array of queue nodes
} vHash;

///

// A utility function to create a new Queue Node. The queue Node
// will store the given 'pageNumber'
QNode* newQNode( unsigned pageNumber ) {
    // Allocate memory and assign 'pageNumber'
	QNode* temp = (QNode *)malloc( sizeof( QNode ) );
	temp->pageNumber = pageNumber;

    // Initialize prev and next as NULL
	temp->prev = temp->next = NULL;

	return temp;
}

//////
Queue* q = NULL;
vHash* hash = NULL;
//////
char *cold = NULL;


unsigned int free_cold=0;
unsigned int free_warm=0;


// A utility function to create an empty Queue.
// The queue can have at most 'numberOfFrames' nodes
Queue* createQueue( int numberOfFrames ) {
	Queue* queue = (Queue *)malloc( sizeof( Queue ) );

    // The queue is empty
	queue->count = 0;
	queue->front = queue->rear = NULL;

    // Number of frames that can be stored in memory
	queue->numberOfFrames = numberOfFrames;

	return queue;
}

// A utility function to create an empty Hash of given capacity
vHash* createHash( int capacity ) {
    // Allocate memory for hash
	vHash* hash = (vHash *) malloc( sizeof( vHash ) );
	hash->capacity = capacity;

    // Create an array of pointers for refering queue nodes
	hash->array = (QNode **) malloc( hash->capacity * sizeof( QNode* ) );

    // Initialize all hash entries as empty
	int i;
	for( i = 0; i < hash->capacity; ++i )
		hash->array[i] = NULL;

	return hash;
}

// A function to check if there is slot available in memory
int AreAllFramesFull( Queue* queue ) {
	return queue->count == queue->numberOfFrames;
}

// A utility function to check if queue is empty
int isQueueEmpty( Queue* queue ) {
	return queue->rear == NULL;
}

// A utility function to delete a frame from queue
void deQueue( Queue* queue ) {
	if( isQueueEmpty( queue ) )
		return;

    // If this is the only node in list, then change front
	if (queue->front == queue->rear)
		queue->front = NULL;

    // Change rear and remove the previous rear
	QNode* temp = queue->rear;
	queue->rear = queue->rear->prev;

	if (queue->rear)
		queue->rear->next = NULL;

#if 0
	memcpy2(&cold[PAGE_SIZE*temp->pageNumber], &warm[PAGE_SIZE*temp->warm],PAGE_SIZE);
#else
	Ipp8u ctr[16];
	memcpy2(ctr, ctr0, sizeof(ctr));

	ippsAESEncryptCTR((const unsigned char*)&warm[PAGE_SIZE*temp->warm], (unsigned char*)&cold[PAGE_SIZE*temp->pageNumber], PAGE_SIZE, pAES, ctr, 64);
#ifdef COLD_HASH
	hashes[temp->pageNumber]=XXH64(&warm[PAGE_SIZE*temp->warm], PAGE_SIZE, 0xaabbacaca);
#endif
#endif

	set_free_warm(temp->warm);

	free( temp );

    // decrement the number of full frames by 1
	queue->count--;
}

// A function to add a page with given 'pageNumber' to both queue
// and hash
QNode* Enqueue( Queue* queue, vHash* hash, unsigned pageNumber ) {
    // If all frames are full, remove the page at the rear
	if ( AreAllFramesFull ( queue ) ) {
#ifdef WARM_FETCH
		if(queue->rear->stick) {
//		we are full of **** and cannot add more
			return 0;
		}
#endif
		hash->array[ queue->rear->pageNumber ] = NULL;
		deQueue( queue );
	}

    // Create a new node with given page number,
    // And add the new node to the front of queue
	QNode* temp = newQNode( pageNumber );
	temp->next = queue->front;

	temp->warm = get_free_warm();
#ifdef WARM_FETCH
	temp->stick = 1;
#endif
#if 0
	memcpy2(&warm[PAGE_SIZE*temp->warm], &cold[PAGE_SIZE*pageNumber], PAGE_SIZE);
#else
   // counter
	Ipp8u ctr[16];

   // init counter before decryption
	memcpy2(ctr, ctr0, sizeof(ctr));
   // decryption
	if(hashes[pageNumber]!=0) {
		ippsAESDecryptCTR((const unsigned char*)&cold[PAGE_SIZE*pageNumber],(unsigned char *) &warm[PAGE_SIZE*temp->warm], PAGE_SIZE, pAES, ctr, 64);
#ifdef COLD_HASH
		if(hashes[pageNumber]!=XXH64((unsigned char *) &warm[PAGE_SIZE*temp->warm], PAGE_SIZE, 0xaabbacaca)) {
			printf("hash missmatch for %d\n", pageNumber);
			while(1);
		}
#else
		hashes[pageNumber] = 1;
#endif
	} else
		memset((unsigned char *) &warm[PAGE_SIZE*temp->warm], 0, PAGE_SIZE);
#endif
    // If queue is empty, change both front and rear pointers
	if ( isQueueEmpty( queue ) )
		queue->rear = queue->front = temp;
	else  // Else change the front
	{
		queue->front->prev = temp;
		queue->front = temp;
	}

    // Add page entry to hash also
	hash->array[ pageNumber ] = temp;

    // increment number of full frames
	queue->count++;

	return temp;
}

QNode* ReferencePage( Queue* queue, vHash* hash, unsigned pageNumber, int page_up) {
	QNode* reqPage = hash->array[ pageNumber ];
    // the page is not in cache, bring it
	if ( reqPage == NULL ) {
		if(page_up)
			reqPage = Enqueue( queue, hash, pageNumber );
		if(reqPage == NULL)
			return 0;
    // page is there and not at front, change pointer
	} else if (reqPage != queue->front) {
        // Unlink rquested page from its current location
        // in queue.
		reqPage->prev->next = reqPage->next;
		if (reqPage->next)
			reqPage->next->prev = reqPage->prev;
        // If the requested page is rear, then change rear
        // as this node will be moved to front
		if (reqPage == queue->rear){
			queue->rear = reqPage->prev;
			queue->rear->next = NULL;
		}
        // Put the requested page before current front
		reqPage->next = queue->front;
		reqPage->prev = NULL;
        // Change prev of current front
		reqPage->next->prev = reqPage;

        // Change front to the requested page
		queue->front = reqPage;
	}
#ifdef WARM_FETCH 
	reqPage->stick = 1;
#endif
	return reqPage;
}

char* DeReferencePage( Queue* queue, vHash* hash, unsigned pageNumber) {
	QNode* reqPage = hash->array[ pageNumber ];
	if(reqPage == NULL) {
		printf("unsticked page does not exist",1);
	}

	if (reqPage != queue->rear) {
		if (reqPage->prev)
			reqPage->prev->next = reqPage->next;

		reqPage->next->prev = reqPage->prev;

		if (reqPage == queue->front){
			queue->front = reqPage->next;
			queue->front->prev = NULL;
		}

		reqPage->prev = queue->rear;
		reqPage->next = NULL;
		reqPage->prev->next = reqPage;
		queue->rear = reqPage;
	}

	reqPage->stick = 0;
}

void *page_up(int page) {
#ifdef WARM_FETCH
	QNode* 	reqPage=ReferencePage( q, hash, page, 1);
	if(reqPage != 0)
		return &warm[reqPage->warm*PAGE_SIZE];
#else
	return 0;
#endif
}

void page_down(int page) {
	DeReferencePage( q, hash, page);
}


void cache_purge(Queue* queue) {
	while(queue->count != 0)
		deQueue(queue);
}

void cache_reset() {
	if(q) {
		cache_purge(q);
		free(q);
	}

	if(hash) {
		free(hash->array);
		free(hash);
	}

	q = createQueue( WARM_PAGES);
	hash = createHash( COLD_PAGES);

	memset(cold, 0, COLD_MEM_SIZE);
	memset(warm, 0, WARM_MEM_SIZE);
	memset(zero_page, 0, PAGE_SIZE);
	memset(hashes, 0, sizeof(hashes));
	memset(pcs, 0, sizeof(pcs));

	free_warm = 0;
}

void ext_encrypt(char *dst, char *src,int size) {
	Ipp8u ctr[16];

	memcpy2(ctr, ctr0, sizeof(ctr));

	ippsAESEncryptCTR((const unsigned char*)src, (unsigned char*)dst, size, pAES, ctr, 64);
}
void ext_decrypt(char *dst, char *src,int size) {
	Ipp8u ctr[16];

	memcpy2(ctr, ctr0, sizeof(ctr));

	ippsAESDecryptCTR((const unsigned char*)src,(unsigned char *) dst, size, pAES, ctr, 64);
}



int copy_to(int dst_offset, char *src, int size) {
//	printf("COPY TO: dst_offset\t%d\tsize = %d\n", dst_offset, size);
#ifdef CPY
	memcpy2(&cold[dst_offset], src, size);
	return 0;
#endif
//SQLite accesses first page unaligned, we cache indepandetly this page. 
	if(dst_offset ==0) {
		memcpy2(&zero_page[dst_offset], src, size);
		return 0;
	}


#ifdef WARM_CACHE
	QNode* 	reqPage=ReferencePage( q, hash, dst_offset/PAGE_SIZE, 1);
	if(reqPage == 0) {
		ext_encrypt(&cold[dst_offset], src, size);

    #ifdef COLD_HASH
		hashes[dst_offset/PAGE_SIZE]=XXH64(src, size, 0xaabbacaca);
    #endif
	} else {
		memcpy2(&warm[reqPage->warm*PAGE_SIZE], src, size);
	}
#else
		ext_encrypt(&cold[dst_offset], src, size);
    #ifdef COLD_HASH
		hashes[dst_offset/PAGE_SIZE]=XXH64(src, size, 0xaabbacaca);
    #endif

#endif
	return 0;
}

int copy_from(char *dst, int src_offset, int size) {
//	printf("COPY FROM: src_offset\t%d\t size = %d, %x, %d\n", src_offset, size, src_offset & (PAGE_SIZE-1), PAGE_SIZE);
#ifdef CPY
	memcpy2(dst, &cold[src_offset], size);
	return 0;
#endif

//SQLite clames that all access is aglined. But this is lie.
	if( src_offset/PAGE_SIZE == 0 && (src_offset/PAGE_SIZE+size)<= PAGE_SIZE) { //unaligned access to the first page. this page is cached in normal memory
		memcpy2(dst, &zero_page[src_offset], size);
		return 0;
	}

//	if(src_offset&(PAGE_SIZE-1) && (src_offset/PAGE_SIZE!=0)) { //we have unaligned access
	if(src_offset&(PAGE_SIZE-1) ) { //we have unaligned access
		printf("unaligned access at line %d\n", __LINE__);
		printf("COPY FROM: src_offset = %d, size = %d, %x, %d\n", src_offset, size, src_offset & (PAGE_SIZE-1), PAGE_SIZE);
		while(1);
	}


#ifdef WARM_CACHE
	QNode* 	reqPage=ReferencePage( q, hash, src_offset/PAGE_SIZE, 1);
	if(reqPage == 0) {
		ext_decrypt(dst, &cold[src_offset], size);

    #ifdef COLD_HASH
		if(hashes[src_offset/PAGE_SIZE]!=XXH64(dst, size, 0xaabbacaca))
			return 1;
    #endif
	} else {
		memcpy2(dst, &warm[reqPage->warm*PAGE_SIZE], size);
	}
#else
	ext_decrypt(dst, &cold[src_offset], size);
    #ifdef COLD_HASH
	if(hashes[src_offset/PAGE_SIZE]!=XXH64(dst, size, 0xaabbacaca)) {
		printf("hash = %lx/%s\n", hashes[src_offset/PAGE_SIZE], dst);
		return 1;
	}
    #endif
#endif
	return 0;
}

unsigned int vmem_max() {
	return COLD_MEM_SIZE;
}

unsigned int vmem_sector_size() {
	return PAGE_SIZE;
}


unsigned int get_free_warm() {
	if (free_warm < WARM_PAGES)
	    return free_warm++;

	return -1;
}

unsigned int get_free_cold(int count) {
	printf("free_cold = %d, asked = %d\n", free_cold, count);
	return (free_cold+=count);
}

void set_free_warm(unsigned int page) {
	free_warm = page;
}

void vmem_init(void *gcold) {
	cold=(char *)gcold;
	printf("gcold = %lx, cold = %lx, warm = %lx\n", (unsigned long int) gcold, cold, (unsigned long int) warm);

	cache_reset();
	printf("[%d]\tinit cache done\n",__LINE__);
}


//////////////////

extern char *cold;

int init(void *gcold) {
	int ctxSize;
	ippsAESGetSize(&ctxSize);

	pAES = (IppsAESSpec*)malloc(ctxSize);
	ippsAESInit(key, sizeof(key)-1, pAES, ctxSize);
	vmem_init(gcold);

	return 0;
}

