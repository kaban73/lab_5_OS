#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "simplefs.h"

#define DISKNAME "vdisk1.bin"

/**
 * Работа функции main заключается в том, что мы
 * 1. Создаем виртуальный диск
 * 2. Монтируем его
 * 3. Форматируем
 * 4. Создаем файл1
 *      - Записываем в файл1 десять букв А
 *      - Считываем данные с файла1(10 букв А) в my_buffer и выводим в консоль
 * 5. Создаем файл2
 *      - Записываем в файл2 десять массивов букв F G H I
 *      - Считываем данные с файла2(10 массивов букв F G H I) в my_buffer и выводим в консоль
 * 6. Создаем файл3
 *      - Записываем в файл3 десять массивов букв A B C D E F G H
 *      - Считываем данные с файла3(10 массивов букв A B C D E F G H) в my_buffer и выводим в консоль
 * 7. Размонтируем диск
 *
 * Закомментированные строчки служат для дебаггинга и возвращают значения функций
 */

int main()
{
    int ret;
    int fd1, fd2, fd;
    int i;
    char buffer[1024];
    char my_buffer[1024]; // Пустой массив, спецаильно объявленный, чтобы наглядно продемонстрировать корректную работу sfs_read и sfs_append
    //char buffer2[8] = {50, 50, 50, 50, 50, 50, 50, 50}; // 2
    char buffer2[8] = {65, 66, 67, 68, 69, 70, 71, 72}; // abcd...
    int size;
    char c;
    int k;

    printf ("started\n");
    // ****************************************************
    // if this is the first running of app, we can
    // create a virtual disk and format it as below
    ret  = create_vdisk (DISKNAME, 24); // size = 16 MB
    //printf ("create_vdisk ret: %d\n", ret);
    if (ret != 0) {
        printf ("there was an error in creating the disk\n");
        exit(1);
    }
    ret = sfs_mount (DISKNAME);
    //printf ("sfs_mount ret: %d\n", ret);
    if (ret != 0) {
        printf ("could not mount \n");
        exit (1);
    }
    // ****************************************************
    ret = sfs_format (DISKNAME);
    //printf ("sfs_format ret: %d\n", ret);
    if (ret != 0) {
        printf ("there was an error in format\n");
        exit(1);
    }
    printf ("started\n");

    printf ("creating files\n");
    ret = sfs_create ("file1.bin");
    //printf ("sfs_create file1: %d\n", ret);
    ret = sfs_create ("file2.bin");
    //printf ("sfs_create file2: %d\n", ret);
    ret = sfs_create ("file3.bin");
    //printf ("sfs_create file3: %d\n", ret);

    fd1 = sfs_open ("file1.bin", MODE_APPEND);
    //printf ("sfs_open append fd1: %d\n", fd1);
    for (i = 0; i < 10; ++i) {
        buffer[0] =   (char) 65;
        ret = sfs_append (fd1, (void *) buffer, 1);
        //printf ("sfs_append file1: %d\n", ret);
    }
    fd1 = sfs_open("file1.bin", MODE_READ);
    //printf ("sfs_open read fd1: %d\n", fd1);
    size = sfs_getsize (fd1);
    //printf ("sfs_getsize size fd1: %d\n", size);
    printf ("Size of the file1: %d\n", size);
    printf("Output of the file1 contents\n");
    printf("-----------------\n");
    k = 1;
    for (i = 0; i < size; ++i) {
        ret = sfs_read (fd1, (void *) my_buffer, 1);
        //printf ("sfs_read file1: %d\n", ret);
        c = (char) my_buffer[0];
        printf ("%d. char in file1: %c\n", k++, c);
    }
    printf("-----------------\n");
    sfs_close (fd1);

    fd2 = sfs_open ("file2.bin", MODE_APPEND);
    //printf ("sfs_open fd2: %d\n", fd2);
    for (i = 0; i < 10; ++i) {
        buffer[0] = (char) 70;
        buffer[1] = (char) 71;
        buffer[2] = (char) 72;
        buffer[3] = (char) 73;
        ret = sfs_append(fd2, (void *) buffer, 4);
        //printf ("sfs_append file2: %d\n", ret);
    }

    fd2 = sfs_open("file2.bin", MODE_READ);
    //printf ("sfs_open read fd2: %d\n", fd2);
    size = sfs_getsize (fd2);
    //printf ("sfs_getsize size fd2: %d\n", size);

    printf ("Size of the file2: %d\n", size);
    printf("Output of the file2 contents\n");
    printf("-----------------\n");
    k = 1;
    for (i = 0; i < size; i += 4) {
        ret = sfs_read (fd2, (void *) my_buffer, 4);
        //printf ("sfs_read file2: %d\n", ret);
        printf ("%d. char's array in file2: %c %c %c %c\n", k++, my_buffer[0], my_buffer[1], my_buffer[2], my_buffer[3]);
    }
    printf("-----------------\n");
    sfs_close (fd2);


    fd = sfs_open("file3.bin", MODE_APPEND);
    //printf ("sfs_open fd: %d\n", fd);
    for (i = 0; i < 10; ++i) {
        memcpy (buffer, buffer2, 8); // just to show memcpy
        ret = sfs_append(fd, (void *) buffer, 8);
        //printf ("sfs_append file3: %d\n", ret);
    }

    fd = sfs_open("file3.bin", MODE_READ);
    //printf ("sfs_open fd: %d\n", fd);
    size = sfs_getsize (fd);
    //printf ("sfs_getsize size: %d\n", size);
    printf ("Size of the file3: %d\n", size);
    printf("Output of the file3 contents\n");
    printf("-----------------\n");
    k = 1;
    for (i = 0; i < size; i += 8) {
        ret = sfs_read (fd, (void *) my_buffer, 8);
        //printf ("sfs_read file3: %d\n", ret);
        printf ("%d. char's array in file3: %c %c %c %c %c %c %c %c\n", k++, my_buffer[0], my_buffer[1], my_buffer[2], my_buffer[3],
                my_buffer[4], my_buffer[5], my_buffer[6], my_buffer[7]);
    }
    printf("-----------------\n");
    sfs_close (fd);

    ret = sfs_umount();
    //printf ("sfs_unmount ret: %d\n", ret);
}