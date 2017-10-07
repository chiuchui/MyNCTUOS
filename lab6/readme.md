### nctuOS

A tiny OS that used for course OSDI in National Chiao Tung University, Computer Science Dept.

This OS only supports x86

### Lab 6

In this lab, you will learn about how to make os support symmertric multiprocessing (SMP) and simple scheduling policy for SMP.

**Source Tree**

`kernel/*`: Includes all the file implementation needed by kernel only.
`lib/*`: Includes libraries that should be in user space.
`inc/*`: Header files for user.
`user/*`: Files for user program.
`boot/*`: Files for booting the kernel.

You can leverage `grep` to find out where to fill up to finish this lab.

`$ grep -R TODO .`

To run this kernel

    $ make
    $ make qemu

To debug

    $ make debug

### Acknowledgement

This is forked and modified from MIT's Xv6
