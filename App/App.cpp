#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

#include <unistd.h>
#include <pwd.h>
#define MAX_PATH FILENAME_MAX

#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include <netdb.h>
#include <netinet/in.h>

#include <stdio.h>
#include <iostream>
#include "Enclave_u.h"
#include "sgx_urts.h"
#include "sgx_utils/sgx_utils.h"

#include <sys/time.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>

#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "App.h"
#include "../vme_config.h"

sgx_enclave_id_t global_eid = 0;

struct timeval pstart, pend;

void ocall_printf(const char* str) {
	gettimeofday(&pend, NULL);
//	printf("%ld\t%s",((pend.tv_sec-pstart.tv_sec)*1000000 + pend.tv_usec - pstart.tv_usec),str);
	printf("%s",str);
	gettimeofday(&pstart, NULL);
}

void ocall_exit(int exit_status)
{
	printf("enclave exited with status %d\n", exit_status);
}

int64_t ocall_get_time()
{
#if 1
	static const int64_t unixEpoch = 24405875*(int64_t)8640000;
	int64_t piNow;
	struct timeval sNow;
	(void)gettimeofday(&sNow, 0);  /* Cannot fail given valid arguments */
	piNow = unixEpoch + 1000*(unsigned long int)sNow.tv_sec + sNow.tv_usec/1000;
	return piNow;
#endif
}



int main(int argc, char *argv[]) {
	int ret;
	int retval;

	ret = initialize_enclave(&global_eid, "enclave.token", "enclave.signed.so");
	if(ret < 0) {
		printf("Fail to initialize enclave.");
		return 1;
	}

	void *ncold;

#if 1
	printf("posix memalign rets = %d\n", posix_memalign(&ncold, PAGE_SIZE, COLD_MEM_SIZE));
#else
	//check the file
	int fd = open("./cold", O_RDWR | O_CREAT,  S_IRWXU | S_IRGRP | S_IROTH);
	if((ncold = mmap(NULL, COLD_MEM_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0)) == MAP_FAILED) {
		perror("Error in mmap");
		return EXIT_FAILURE;
	}
#endif

	printf("ncold = %lx with size = %d\n", (unsigned long int) ncold, COLD_MEM_SIZE);
	memset(ncold, 0, COLD_MEM_SIZE);

	int ptr;
	sgx_status_t status;
	status = init(global_eid, &ptr, (char *)ncold);
	if (status != SGX_SUCCESS)
		printf("[%d]\tecall error %x\n", __LINE__, status);

	printf("init done\n");

#ifdef SPD_TEST
	printf("-------------------- speedtest -------------- \n");

	ret = ecall_speedtest1_main(global_eid, &retval, argc, argv);
	if (ret != SGX_SUCCESS) {
		printf("no success (open): %d\n", ret);
		return 1;
	}
#endif

#ifdef VNL_SYNTH
	printf("vanilla synthetic benchmark\n");

	ret = synthetic(global_eid, &retval, 0, 1);
	if (ret != SGX_SUCCESS) {
		printf("no success (open): %d\n", ret);
		return 1;
	}
#endif


#ifdef VME_SYNTH
	printf("VME synthetic benchmark\n");

	ret = synthetic(global_eid, &retval, 1, 1);
	if (ret != SGX_SUCCESS) {
		printf("no success (open): %d\n", ret);
		return 1;
	}
#endif

	while(1)
		sleep(100);

	/* Destroy the enclave */
	sgx_destroy_enclave(global_eid);

	return 0;
}
