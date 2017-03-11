#include "tgm_util.h"

#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/mman.h>
#include <fcntl.h>

#include <stdio.h>

static const char* sharedMemKey = "taptracker_data";
static int fd = 0;
static size_t vSize = 0;
static void* sharedMem = NULL;

void memorymap_create(size_t dataSize)
{
    vSize = dataSize;

    fd = shm_open(sharedMemKey, O_RDWR | O_CREAT | O_TRUNC, S_IRWXO | S_IRWXG | S_IRWXU);

    // Stretch our new file to the suggested size.
    if (lseek(fd, vSize - 1, SEEK_SET) == -1)
    {
        perror("Could not stretch file via lseek");
    }

    // In order to change the size of the file, we need to actually write some
    // data. In this case, we'll be writing an empty string ('\0').
    if (write(fd, "", 1) != 1)
    {
        perror("Could not write the final byte in file");
    }

    sharedMem = mmap(NULL, vSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (sharedMem == MAP_FAILED)
    {
        perror("Could not map memory");
    }

    if(mlock(sharedMem, vSize) != 0)
    {
        perror("mlock failure");
    }
}

void memorymap_destroy()
{
    if (munlock(sharedMem, vSize) != 0)
        perror("Error unlocking memory page");

    if (munmap(sharedMem, vSize) != 0)
        perror("Error unmapping memory pointer");

    if (close(fd) != 0)
        perror("Error closing file");

    shm_unlink(sharedMemKey);
}

void* memorymap_getPointer()
{
    return sharedMem;
}

int createDir(const char* path)
{
    struct stat st = {};
    if (stat(path, &st) == -1)
    {
        return mkdir(path, 0700);
    }
    return 0;
}

#endif
