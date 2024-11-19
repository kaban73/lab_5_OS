#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "simplefs.h"
#include <string.h>
#include <stdint.h>

#define SUPERBLOCK_SIZE sizeof(SuperBlock)
#define FAT_SIZE 1024
#define DIR_ENTRY_SIZE 128
#define NUM_DIR_ENTRIES 56
#define BLOCKSIZE 1024 // Размер блока в байтах

// Структуры данных
typedef struct {
    int total_blocks;    // общее количество блоков
    int free_blocks;     // количество свободных блоков
    int fat_blocks;      // количество блоков для FAT
    int root_dir_blocks; // количество блоков для корневого каталога
} SuperBlock;

#define MAX_FILES 52 // Максимальное количество файлов в файловой системе

// Запись в корневом каталоге
typedef struct {
    char filename[32];   // имя файла (с учетом завершающего нуля)
    int size;            // размер файла в байтах
    int first_block;     // номер первого блока данных
} DirectoryEntry;

static DirectoryEntry directory_entries[NUM_DIR_ENTRIES]; // Записи каталога
static int files_count = 0; // Общее количество файлов в файловой системе

#define MAX_OPEN_FILES 10 // Максимальное количество открытых файлов

typedef struct {
    int fd; // Дескриптор файла
    char filename[32]; // Имя файла
    int mode; // Режим (чтение или добавление)
    int current_size; // Текущий размер файла в байтах
} OpenFileEntry;

// Таблица открытых файлов
static OpenFileEntry open_files[MAX_OPEN_FILES];

// Инициализируем таблицу открытых файлов
void init_open_files() {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        open_files[i].fd = -1; // -1 означает, что запись свободна
    }
}

int vdisk_fd; // global virtual disk file descriptor
// will be assigned with the sfs_mount call
// any function in this file can use this.


// This function is simply used to a create a virtual disk
// (a simple Linux file including all zeros) of the specified size.
// You can call this function from an app to create a virtual disk.
// There are other ways of creating a virtual disk (a Linux file)
// of certain size.
// size = 2^m Bytes
int create_vdisk(char *vdiskname, int m) {
    // Вычисляем размер диска
    int size = 1 << m; // 2^m
    // Открываем файл для записи
    int fd = open(vdiskname, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("Error creating virtual disk");
        return -1; // Ошибка при создании файла
    }

    // Заполняем файл нулями
    char *buffer = calloc(size, sizeof(char)); // Выделяем память для заполнения
    if (buffer == NULL) {
        close(fd);
        perror("Error allocating memory");
        return -1; // Ошибка при выделении памяти
    }

    // Записываем заполненный нулями буфер в файл
    ssize_t written = write(fd, buffer, size);
    if (written != size) {
        free(buffer);
        close(fd);
        perror("Error writing to virtual disk");
        return -1; // Ошибка при записи в файл
    }

    // Освобождаем память и закрываем файл
    free(buffer);
    close(fd);
    return 0; // Успешное создание виртуального диска
}




// read block k from disk (virtual disk) into buffer block.
// size of the block is BLOCKSIZE.
// space for block must be allocated outside of this function.
// block numbers start from 0 in the virtual disk.
int read_block(void *block, int k) {
    int n;
    int offset;

    offset = k * BLOCKSIZE; // Вычисляем смещение для блока k
    lseek(vdisk_fd, (off_t) offset, SEEK_SET); // Устанавливаем указатель на смещение
    n = read(vdisk_fd, block, BLOCKSIZE); // Читаем блок данных
    if (n != BLOCKSIZE) {
        printf("read error\n"); // Ошибка, если не все данные были прочитаны
        return -1; // Возвращаем -1 в случае ошибки
    }
    return 0; // Возвращаем 0 в случае успеха
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

int sfs_mount(char *vdiskname) {
    // Проверка имени диска
    if (vdiskname == NULL) {
        fprintf(stderr, "Error: Disk name is NULL\n");
        return -1; // Ошибка: имя диска не может быть NULL
    }

    // Открыть виртуальный диск с разрешениями чтения и записи
    vdisk_fd = open(vdiskname, O_RDWR);
    if (vdisk_fd < 0) {
        perror("Error opening virtual disk");
        return -1; // Ошибка: не удалось открыть файл
    }

    init_open_files(); // Инициализируем таблицу открытых файлов(Фикс)
    // Успешное открытие диска
    return 0;
}

int sfs_umount ()
{
    fsync (vdisk_fd);
    close (vdisk_fd);
    return (0);
}

int sfs_create(char *filename) {
    // Проверка имени файла
    if (filename == NULL) {
        return -1; // Ошибка: имя файла не может быть NULL
    }
    if (strlen(filename) >= 32) {
        return -1; // Ошибка: имя файла слишком длинное
    }

    // Проверка на переполнение массива записей каталога
    if (files_count >= MAX_FILES) {
        return -1; // Ошибка: превышено максимальное количество файлов
    }

    // Поиск первого доступного места в записях каталога
    int entry_index = -1;
    for (int i = 0; i < NUM_DIR_ENTRIES; i++) {
        if (directory_entries[i].filename[0] == '\0') { // Найдено свободное место
            entry_index = i;
            break;
        }
    }

    if (entry_index == -1) {
        return -1; // Ошибка: нет свободного места в каталоге
    }

    // Создание записи о файле
    strcpy(directory_entries[entry_index].filename, filename);
    directory_entries[entry_index].size = 0; // Новый файл пока пустой
    directory_entries[entry_index].first_block = -1; // Временное значение для первого блока данных

    files_count++; // Увеличение счетчика файлов

    // Запись обновленных данных в корневой каталог на диск
    if (write_block(&directory_entries, (entry_index / 8) + 1) == -1) {
        return -1; // Ошибка записи в диск
    }

    return 0; // Успешное создание файла
}


int sfs_open(char *filename, int mode) {
    // Проверка имени файла
    if (filename == NULL) {
        return -1; // Ошибка: имя файла не может быть NULL
    }

    // Поиск файла в каталоге
    for (int i = 0; i < NUM_DIR_ENTRIES; i++) {
        if (strcmp(directory_entries[i].filename, filename) == 0) {
            // Найден файл
            // Проверка наличия места в таблице открытых файлов
            for (int j = 0; j < MAX_OPEN_FILES; j++) {
                if (open_files[j].fd == -1) { // Свободная запись
                    // Заполнение структуры OpenFileEntry
                    open_files[j].fd = j; // Для простоты используем индекс как fd
                    strcpy(open_files[j].filename, filename);
                    open_files[j].mode = mode;
                    open_files[j].current_size = directory_entries[i].size; // Получаем размер файла

                    // Возвращаем индекс как дескриптор файла
                    return j;
                }
            }
            return -1; // Ошибка: нет места для открытия файла
        }
    }

    return -1; // Ошибка: файл не найден
}

int sfs_close(int fd) {
    // Проверка допустимости дескриптора файла
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return -1; // Ошибка: недопустимый дескриптор
    }

    // Проверка, открыт ли файл
    if (open_files[fd].fd == -1) {
        return -1; // Ошибка: файл не открыт
    }

    // Освобождаем запись в таблице открытых файлов
    open_files[fd].fd = -1; // Устанавливаем в -1, чтобы отметить запись как свободную
    memset(open_files[fd].filename, 0, sizeof(open_files[fd].filename)); // Очистим имя файла
    open_files[fd].mode = 0; // Очистить режим
    open_files[fd].current_size = 0; // Очистить текущий размер

    return 0; // Успешное закрытие файла
}


int sfs_getsize(int fd) {
    // Проверка диапазона дескриптора файла
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return -1; // Ошибка: недопустимый дескриптор
    }

    // Проверка, что запись в таблице открытых файлов существует
    if (open_files[fd].fd == -1) {
        return -1; // Ошибка: файл не открыт
    }

    // Получаем имя файла из открытого файла
    char *filename = open_files[fd].filename;

    // Поиск файла в каталоге, чтобы получить его размер
    for (int i = 0; i < NUM_DIR_ENTRIES; i++) {
        if (strcmp(directory_entries[i].filename, filename) == 0) {
            return directory_entries[i].size; // Возвращаем размер файла
        }
    }

    return -1; // Ошибка: файл не найден в каталоге
}


int sfs_read(int fd, void *buf, int n) {
    // Проверка допустимости дескриптора файла
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return -1; // Ошибка: недопустимый дескриптор
    }

    // Проверка, открыт ли файл
    if (open_files[fd].fd == -1) {
        return -1; // Ошибка: файл не открыт
    }

    // Получаем имя файла из открытого файла
    char *filename = open_files[fd].filename;

    // Поиск файла в каталоге, чтобы получить информацию о его размере и первом блоке данных
    for (int i = 0; i < NUM_DIR_ENTRIES; i++) {
        if (strcmp(directory_entries[i].filename, filename) == 0) {
            int file_size = directory_entries[i].size; // Получаем размер файла
            int first_block = directory_entries[i].first_block; // Получаем номер первого блока
            if (file_size == 0) {
                return 0; // Файл пустой, возвращаем 0
            }

            // Определяем количество байт, которые нужно прочитать
            int bytes_to_read = (n > file_size) ? file_size : n; // Определяем, сколько байт нужно прочитать
            int total_read = 0; // Общее количество прочитанных байтов

            char block[BLOCKSIZE]; // Буфер для загрузки блока
            int current_block = first_block;

            // Читаем последовательно блоки, связанные с файлом
            while (total_read < bytes_to_read) {
                // Читаем текущий блок
                if (read_block(block, current_block) == -1) {
                    return -1; // Ошибка чтения блока
                }

                // Определяем сколько байтов копировать из блока
                int bytes_in_block = (total_read + BLOCKSIZE > bytes_to_read) ?
                                     bytes_to_read - total_read : BLOCKSIZE;

                // Копируем данные в буфер
                memcpy(buf + total_read, block, bytes_in_block);
                total_read += bytes_in_block;

                // Получаем следующий блок из FAT
                uint64_t fat_entry;
                if (read_block(&fat_entry, current_block) == -1) {
                    return -1; // Ошибка чтения FAT
                }
                current_block = fat_entry; // Передаем указатель на следующий блок
                if (current_block == 0) break; // Если reached end of file
            }

            return total_read; // Возвращаем количество успешно прочитанных байтов
        }
    }
    return -1; // Ошибка: файл не найден в каталоге
}


int find_free_block() {
    // Загружаем таблицу FAT из диска в оперативную память
    uint64_t fat[FAT_SIZE];
    if (read_block(fat, 1 + 7) == -1) { // FAT начинается после корневого каталога
        return -1; // Ошибка чтения FAT
    }

    // Ищем первый свободный блок
    for (int i = 0; i < FAT_SIZE; i++) {
        if (fat[i] == 0) { // Если FAT запись равна 0, блок свободен
            return i + 1 + 7; // Возвращаем номер блока; +1 из-за суперблока и +7 из-за корневого каталога
        }
    }

    return -1; // Если свободные блоки не найдены
}


int sfs_append(int fd, void *buf, int n) {
    // Проверка допустимости дескриптора файла
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return -1; // Ошибка: недопустимый дескриптор
    }

    // Проверка, открыт ли файл
    if (open_files[fd].fd == -1) {
        return -1; // Ошибка: файл не открыт
    }

    char *filename = open_files[fd].filename;
    int total_bytes_written = 0; // Общее количество записанных байтов

    // Поиск файла в каталоге
    for (int i = 0; i < NUM_DIR_ENTRIES; i++) {
        if (strcmp(directory_entries[i].filename, filename) == 0) {
            int first_block = directory_entries[i].first_block;

            // Пока есть данные для записи
            while (total_bytes_written < n) {
                int free_block = find_free_block();
                if (free_block == -1) {
                    return total_bytes_written; // Возвращаем то, что было записано
                }

                // Подготовка блока данных для записи
                char data_block[BLOCKSIZE] = {0};

                // Если это первый блок, то прочитаем текущие данные
                if (first_block != -1) {
                    read_block(data_block, first_block);
                }

                // Скопируем данные в блок
                int bytes_to_copy = n - total_bytes_written;
                if (bytes_to_copy > BLOCKSIZE) bytes_to_copy = BLOCKSIZE;

                memcpy(data_block + (open_files[fd].current_size % BLOCKSIZE), (char *)buf + total_bytes_written, bytes_to_copy);

                // Записываем блок обратно на диск
                write_block(data_block, free_block);

                // Обновляем размер файла
                directory_entries[i].size += bytes_to_copy;
                open_files[fd].current_size += bytes_to_copy;

                // Если это первый блок, устанавливаем first_block
                if (first_block == -1) {
                    directory_entries[i].first_block = free_block;
                }

                // Подготовка к следующему блоку
                total_bytes_written += bytes_to_copy;
                first_block = free_block; // Приказы переход к новому блоку
            }
            return total_bytes_written; // Возвращаем количество успешно добавленных байтов
        }
    }
    return -1; // Файл не найден
}




int sfs_delete(char *filename) {
    // Проверка имени файла
    if (filename == NULL) {
        return -1; // Ошибка: имя файла не может быть NULL
    }

    // Поиск файла в каталоге
    for (int i = 0; i < NUM_DIR_ENTRIES; i++) {
        if (strcmp(directory_entries[i].filename, filename) == 0) {
            // Найден файл, который нужно удалить
            int first_block = directory_entries[i].first_block;

            // Освобождение всех блоков, занятых файлом
            while (first_block != -1) {
                // Чтение записи FAT для текущего блока
                uint64_t fat_entry;
                read_block(&fat_entry, first_block);

                // Помечаем блок как свободный, записываем 0 в FAT
                fat_entry = 0; // Освободить блок
                write_block(&fat_entry, first_block); // Записываем изменение в FAT

                // Переход к следующему блоку, согласно записи FAT
                first_block = (first_block + 1) % (FAT_SIZE * BLOCKSIZE / 8); // Пример перехода к следующему блоку в FAT
            }

            // Удаляем запись из каталога
            memset(directory_entries[i].filename, 0, sizeof(directory_entries[i].filename)); // Очищаем имя файла
            directory_entries[i].size = 0; // Обнуляем размер
            directory_entries[i].first_block = -1; // Устанавливаем первый блок в -1

            return 0; // Успешное удаление файла
        }
    }

    return -1; // Ошибка: файл не найден в каталоге
}