#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "simplefs.h"

#define DISKNAME "vdisk1.bin"

int main()
{
    int ret;
    int fd1, fd2, fd;
    int i;
    char buffer[1024];
    //char buffer2[8] = {50, 50, 50, 50, 50, 50, 50, 50}; // 2
    char buffer2[8] = {65, 66, 67, 68, 69, 70, 71, 72}; // abcd...
    int size;
    char c;

    printf ("started\n");
    // ****************************************************
    // if this is the first running of app, we can
    // create a virtual disk and format it as below
    ret  = create_vdisk (DISKNAME, 24); // size = 16 MB
    printf ("create_vdisk ret: %d\n", ret);
    if (ret != 0) {
        printf ("there was an error in creating the disk\n");
        exit(1);
    }
    ret = sfs_mount (DISKNAME);
    printf ("sfs_mount ret: %d\n", ret);
    if (ret != 0) {
        printf ("could not mount \n");
        exit (1);
    }
    // ****************************************************
    ret = sfs_format (DISKNAME);
    printf ("sfs_format ret: %d\n", ret);
    if (ret != 0) {
        printf ("there was an error in format\n");
        exit(1);
    }
    printf ("started\n");

    printf ("creating files\n");
    ret = sfs_create ("file1.bin");
    printf ("sfs_create file1: %d\n", ret);
    ret = sfs_create ("file2.bin");
    printf ("sfs_create file2: %d\n", ret);
    ret = sfs_create ("file3.bin");
    printf ("sfs_create file3: %d\n", ret);

    fd1 = sfs_open ("file1.bin", MODE_APPEND);
    printf ("sfs_open fd1: %d\n", fd1);
    fd2 = sfs_open ("file2.bin", MODE_APPEND);
    printf ("sfs_open fd2: %d\n", fd2);
    for (i = 0; i < 10; ++i) {
        buffer[0] =   (char) 65;
        ret = sfs_append (fd1, (void *) buffer, 1);
        printf ("sfs_append file1: %d\n", ret);
    }

    for (i = 0; i < 10; ++i) {
        buffer[0] = (char) 70;
        buffer[1] = (char) 71;
        buffer[2] = (char) 72;
        buffer[3] = (char) 73;
        ret = sfs_append(fd2, (void *) buffer, 4);
        printf ("sfs_append file2: %d\n", ret);
    }

    sfs_close(fd1);
    sfs_close(fd2);

    fd = sfs_open("file3.bin", MODE_APPEND);
    printf ("sfs_open fd: %d\n", fd);
    for (i = 0; i < 10; ++i) {
        memcpy (buffer, buffer2, 8); // just to show memcpy
        ret = sfs_append(fd, (void *) buffer, 8);
        printf ("sfs_append file3: %d\n", ret);
    }
    sfs_close (fd);

    fd = sfs_open("file3.bin", MODE_READ);
    printf ("sfs_open fd: %d\n", fd);
    size = sfs_getsize (fd);
    for (i = 0; i < size; ++i) {
        ret = sfs_read (fd, (void *) buffer, 1);
        printf ("sfs_read file3: %d\n", ret);
        c = (char) buffer[0];
        printf ("char in file3: %c\n", c);
    }
    sfs_close (fd);

    ret = sfs_umount();
    printf ("sfs_unmount ret: %d\n", ret);
}