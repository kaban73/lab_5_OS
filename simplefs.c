#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "simplefs.h"
#include <string.h>

#define SUPERBLOCK_SIZE sizeof(SuperBlock)
#define FAT_SIZE 1024
#define DIR_ENTRY_SIZE 128
#define NUM_DIR_ENTRIES 56

// Структуры данных
typedef struct {
    int total_blocks;    // общее количество блоков
    int free_blocks;     // количество свободных блоков
    int fat_blocks;      // количество блоков для FAT
    int root_dir_blocks; // количество блоков для корневого каталога
} SuperBlock;

// Запись в корневом каталоге
typedef struct {
    char filename[32];   // имя файла (с учетом завершающего нуля)
    int size;            // размер файла в байтах
    int first_block;     // номер первого блока данных
} DirectoryEntry;


int vdisk_fd; // global virtual disk file descriptor
              // will be assigned with the sfs_mount call
              // any function in this file can use this.


// This function is simply used to a create a virtual disk
// (a simple Linux file including all zeros) of the specified size.
// You can call this function from an app to create a virtual disk.
// There are other ways of creating a virtual disk (a Linux file)
// of certain size.
// size = 2^m Bytes
int create_vdisk (char *vdiskname, int m)
{
    char command[BLOCKSIZE];
    int size;
    int num = 1;
    int count;
    size  = num << m;
    count = size / BLOCKSIZE;
    printf ("%d %d", m, size);
    sprintf (command, "dd if=/dev/zero of=%s bs=%d count=%d", vdiskname, BLOCKSIZE, count);
    printf ("executing command = %s\n", command);
    system (command);
    return (0);
}



// read block k from disk (virtual disk) into buffer block.
// size of the block is BLOCKSIZE.
// space for block must be allocated outside of this function.
// block numbers start from 0 in the virtual disk.
int read_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = read (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
        printf ("read error\n");
        return -1;
    }
    return (0);
}

// write block k into the virtual disk.
int write_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = write (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
        printf ("write error\n");
        return (-1);
    }
    return 0;
}


/**********************************************************************
   The following functions are to be called by applications directly.
***********************************************************************/

// В функции sfs_format
int sfs_format(char *vdiskname) {
    // Определим глобальные параметры файловой системы
    SuperBlock superblock;
    DirectoryEntry dir_entries[NUM_DIR_ENTRIES] = {0}; // все нули
    char fat[FAT_SIZE * BLOCKSIZE] = {0}; // все нули, для FAT
    int i;

    // Заполнить суперблок информацией
    superblock.total_blocks = (1 + 7 + 1024 + (128 * 1024)) / BLOCKSIZE; // расчет общего количества блоков
    superblock.free_blocks = superblock.total_blocks; // все блоки свободны в начале
    superblock.fat_blocks = 1024; // фиксированное количество блоков для FAT
    superblock.root_dir_blocks = 7; // фиксированное количество блоков для корневого каталога

    // Записать суперблок на диск
    if (write_block(&superblock, 0) == -1) {
        return -1; // Ошибка записи
    }

    int dir_block_index = 0; // Индекс для записи в корневой каталог
    // Записать корневой каталог (7 блоков)
    for (i = 0; i < superblock.root_dir_blocks; ++i) {
        if (write_block(&dir_entries[dir_block_index], i + 1) == -1) {
            return -1; // Ошибка записи
        }
        dir_block_index += BLOCKSIZE / DIR_ENTRY_SIZE; // Перемещение к следующему блоку
    }

    // Записать FAT (1024 блока)
    for (i = 0; i < FAT_SIZE; ++i) {
        if (write_block(fat, i + superblock.root_dir_blocks + 1) == -1) {
            return -1; // Ошибка записи
        }
    }

    return 0; // Успешное форматирование
}

int sfs_mount (char *vdiskname)
{
    // simply open the Linux file vdiskname and in this
    // way make it ready to be used for other operations.
    // vdisk_fd is global; hence other function can use it.
    vdisk_fd = open(vdiskname, O_RDWR);
    return(0);
}

int sfs_umount ()
{
    fsync (vdisk_fd);
    close (vdisk_fd);
    return (0);
}


int sfs_create(char *filename)
{
    return (0);
}


int sfs_open(char *file, int mode)
{
    return (0);
}

int sfs_close(int fd){
    return (0);
}

int sfs_getsize (int  fd)
{
    return (0);
}

int sfs_read(int fd, void *buf, int n){
    return (0);
}


int sfs_append(int fd, void *buf, int n)
{
    return (0);
}

int sfs_delete(char *filename)
{
    return (0);
}

