# STANlite

STANlite is an in-memory database engine for SGX-enabled secure data processing.
STANlite performs efficient user-level paging, whenever a database workload requires more space than the
performance-friendly in-memory state size.

### Cite the work: 

The original paper is avaliable [here](https://www.ibr.cs.tu-bs.de/users/sartakov/papers/sartakov18stanlite.pdf)

```
@inproceedings {sartakov2018stanlite,
    url = {https://www.ibr.cs.tu-bs.de/users/sartakov/papers/sartakov18stanlite.pdf},
    title={STANlite--a database engine for secure data processing at rack-scale level},
    author={Sartakov, Vasily and Weichbrodt, Nico and Krieter, Sebastian and Leich, Thomas and Kapitza, R\"{u}diger},
    booktitle={Cloud Engineering (IC2E), 2018 IEEE International Conference on},
    pages={23--33},
    year={2018},
    organization={IEEE}
}
```

## Build

* Change configuration of the engine in `vme_config.h`
* run `SGX_SDK=/opt/intel/sgxsdk make run`

## Source code

* vme_config.h -- configuration file
* App/App.cpp -- untrusted main, spawns all threads
* Enclave/Enclave.config.xml -- enclave config
* Enclave/sqlite -- pure SQLite
* Enclave/vmem.c -- Virtual Memory Engine (VME)
* Enclave/inc -- includes for AES-NI CBC
* Enclave/os-sgx.c -- SQLite SGX OS layer
* Enclave/sgxvfs.c -- virtual file system, glue between VME and SQLite
* Enclave/speedtest1.c -- build-in speed test cases
* Enclave/synthetic.c -- synthetic test
* Enclave/xxhash.c -- hash funtion

## Note

Intel adds protection against various side-channel attacks. As a result, the performance of enclaved software degrates
after each new patch. Here some values measured on the same hardware, but with different versions of software.

Hardware: Intel(R) Xeon(R) CPU E3-1230 v5 @ 3.40GHz

|data		|Native|Vanilla	|nnI|CnI|CFI|
|----		|------	|-------|---|---|---|
|[xx.12.2017](results/initial.md)	|136	|554	|373|305|371|
|[05.04.2020](results/20.04.05.md)	|148	|889	|564|498|565|
