Hardware barrier C library for fujitsu A64FX
============================================

About
-----
Fujitsu A64FX processor provides hardware barrier mechanism which realizes
synchronization between software threads through hardware registers.

This user library provides a set of library functions to utilize hardware barrier
from Linux C program code.

For more details about hardware barrier, see [A64FX specifiction for HPC extension](https://github.com/fujitsu/A64FX).

Build & Install & Test
----------------------

This project uses cmake. To build:

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make

For corss-compile, add -DCMAKE_C_COMPILER=\<compiler\> option to cmake command (before "..").

To install header file(fujitsu_hwb.h) and shared library(libFJhwb.so):

    $ sudo make install

To run test:

    $ make test

Note that test requires fujitsu hardware barrier driver is loaded.
Also, in order to check barrier register's status after each test,
parallel test run (-j) does not work.

Usage
-----
Hardware barrier synchronization can be performed by threads running on the PEs
which belong to the same CMG (Core Memory Group). CMG is the same as L3 cache domain in A64FX.
The overview of synchronization by hardware barrier is as follows:

 1. Setup barrier blade register which determines PEs joining synchronization (**fhwb_init**)
 2. Setup barrier window register on each PE to set barrier blade to be used (**fhwb_assign**)
 3. Perform synchronization on each PE (**fhwb_sync**)
     * Each PE writes 0/1 to BST_SYNC register
     * Wait LBSY_SYNC register becomes to 0/1 (When all PEs has written 0/1 to BST_SYNC, LBSY_SYNC register value changes to 0/1)
 4. Reset/free barrier window register on each PE (**fhwb_unassign**)
 5. Reset/free barrier blade register (**fhwb_fini**)

There are several barrier blade and barrier window registers.
On A64FX, there are 6 barrier blades per CMG and 4 barrier windows per PE.

Since barrier blade register is a shared resource per CMG, 1. and 5. will be performed only
once while 2,3,4 needs to be performed by each thread running on a different PE.
There also exist functions to get PE's CMG number (**fhwb_get_pe_info** and **fhwb_get_all_pe_info**).

Please see comments in [a header file](include/fujitsu_hwb.h) for information about library API.
Also [examples](examples) folder contains some sample code.

To run hardware barrier program, fujitsu hardware barrier driver needs to be loaded as
this library uses ioctls to setup barrier registers (fhwb_sync() is different; After fhwb_assign(),
BST_SYNC/LBSY_SYNC register becomes accessible from EL0 and therefore fhwb_sync() can perform
synchronization without context switch).
The driver requires thread joining synchronization use the same file descriptor for ioctl and
the library manages open/close of the device file.
Although user applications should free allocated barrier resources by fhwb_unassign()/fhwb_fini() after use,
the driver performs cleanup if remaining resources exist upon file close (process exit).

License
-------
LGPLv3

Copyright 2020 FUJITSU LIMITED
