#if HAVE_CONFIG_H
    #include "waf_config.h"
#endif

#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/times.h>
#include <sys/time.h>
#include "bsp.h"
#include "htif.h"

#if ipconfigUSE_FAT_LIBDL
    #include <FreeRTOSFATConfig.h>
    #if ffconfigTIME_SUPPORT == 1
        #undef st_atime
        #undef st_mtime
        #undef st_ctime
    #endif
    #include "ff_headers.h"
    #include <ff_stdio.h>
#endif

#ifdef configUART16550_BASE
    #include "uart16550.h"
#endif

void * _sbrk( int nbytes );
int _open( const char * name,
           int flags,
           int mode );
int _write( int file,
            char * ptr,
            int len );
int _close( int fd );
int _stat( const char * name,
           void * buffer );
int _fstat( int fd,
            void * buffer );
long _lseek( int fd,
             long offset,
             int origin );
int _read( int fd,
           void * buffer,
           unsigned int count );
void _exit( int x );
int _isatty( int fd );
int _kill( int pid,
           int sig );
int _getpid( int n );
int _gettimeofday( struct timeval * tv,
                   struct timezone * tz );

#if ipconfigUSE_FAT_LIBDL
#define configFF_FDS_TABLE_SIZE 1024
static int free_fd = 3;

typedef struct fileMap {
    FF_FILE*       ff_file;
    const char*    name;
} fileMap_t;

static fileMap_t ff_map[configFF_FDS_TABLE_SIZE];

int _write( int file,
            char * ptr,
            int len )
{
    if (file == STDOUT_FILENO) {
        #if PLATFORM_SPIKE || PLATFORM_SAIL
            return htif_console_write_polled( ptr, len );
        #elif PLATFORM_QEMU_VIRT || PLATFORM_PICCOLO || PLATFORM_GFE || PLATFORM_FETT
            return uart16550_txbuffer( ( uint8_t * ) ptr, len );
        #elif PLATFORM_RVBS
            return plat_console_write( ptr, len );
        #else
        #error "Unsupported Console for this PLATFORM"
        #endif
    } else {
        if (ff_map[file].ff_file != NULL) {
            size_t written = 0;
            written = ff_fwrite(ptr, 1, (size_t) len, ff_map[file].ff_file);
            if (written < len) {
                errno = stdioGET_ERRNO();
                return -1;
            } else {
                return written;
            }
        } else {
            errno = EBADF;
            return -1;
        }
    }
}

int _open( const char * name,
           int flags,
           int mode )
{
    FF_FILE* ff_file = NULL;
    char ff_strMode[3] = {0};

    if (free_fd == configFF_FDS_TABLE_SIZE) {
        printf("Error, can not open anymore files, increase configFF_FDS_TABLE_SIZE\n");
        return -1;
    }

    switch (flags) {
        case O_RDONLY:
            ff_strMode[0] = 'r';
            break;
        case O_WRONLY:
        case O_RDWR:
            ff_strMode[0] = 'w';
            break;
        default:
            printf("Only O_RDONLY, O_WRONLY or O_RDWR are supported. Implement more if needed\n");
            return -1;
    }

    for (int i = 0; i < free_fd; i++) {
      if (ff_map[i].name && strcmp(name, ff_map[i].name) == 0)
        return i;
    }

    ff_file = ff_fopen(name, (const char *) &ff_strMode);

    if (ff_file == NULL) {
      return -1;
    }

    /* Sucess, return the FD */
    ff_map[free_fd].ff_file = ff_file;
    ff_map[free_fd].name = name;
    return free_fd++;
}

int _close( int fd )
{
    if (fd >= configFF_FDS_TABLE_SIZE || ff_map[fd].ff_file == NULL) {
        errno = EBADF;
        return -1;
    }

    if (ff_fclose(ff_map[fd].ff_file) != 0) {
        errno = stdioGET_ERRNO();
        return -1;
    }

    ff_map[fd].ff_file = NULL;
    ff_map[fd].name = NULL;
    return 0;
}

long _lseek( int fd,
             long offset,
             int origin )
{
    if (fd >= configFF_FDS_TABLE_SIZE || ff_map[fd].ff_file == NULL) {
        errno = EBADF;
        return -1;
    }

    if (ff_fseek(ff_map[fd].ff_file, (int) offset, origin) != 0) {
        errno = stdioGET_ERRNO();
        return -1;
    }

    return 0;
}

int _read( int fd,
           void * buffer,
           unsigned int count )
{
    int read = 0;
    if (fd >= configFF_FDS_TABLE_SIZE || ff_map[fd].ff_file == NULL) {
        errno = EBADF;
        return -1;
    }

    read = ff_fread(buffer, 1, count, ff_map[fd].ff_file);
    if (read < count) {
        errno = stdioGET_ERRNO();
    }

    return read;
}

int _stat( const char * name,
           void * buffer )
{
    struct stat* sb = (struct stat*) buffer;
    FF_Stat_t ff_sb;

    if (ff_stat(name, &ff_sb) != 0) {
        errno = stdioGET_ERRNO();
        return -1;
    }

    sb->st_dev = ff_sb.st_dev;
    sb->st_ino = ff_sb.st_ino;
    sb->st_mode = ff_sb.st_mode;
    sb->st_size = ff_sb.st_size;

    // Not supported by FreeRTOS FAT
    //sb->st_nlink = ff_sb.st_nlink;
    //sb->st_uid = ff_sb.st_uid;
    //sb->st_gid = ff_sb.st_gid;
    //sb->st_rdev = ff_sb.st_rdev;

    #if ffconfigTIME_SUPPORT == 1
        /* Assumption that FreeRTOS bookkeeps time in us */
        sb->st_atim.tv_nsec = (ff_sb.st_atime % 1000000) * 1000;
        sb->st_mtim.tv_nsec = (ff_sb.st_mtime % 1000000) * 1000;
        sb->st_ctim.tv_nsec = (ff_sb.st_ctime % 1000000) * 1000;

        sb->st_atim.tv_sec = ff_sb.st_atime / 1000000;
        sb->st_mtim.tv_sec = ff_sb.st_mtime / 1000000;
        sb->st_ctim.tv_sec = ff_sb.st_ctime / 1000000;
    #endif

    return 0;
}

int _fstat( int fd,
            void * buffer )
{
    if (fd >= configFF_FDS_TABLE_SIZE || ff_map[fd].ff_file == NULL) {
        errno = EBADF;
        return -1;
    }

    return _stat(ff_map[fd].name, buffer);
}
#else

int _write( int file,
            char * ptr,
            int len )
{
#if (configENABLE_MPU == 1 && configMPU_COMPARTMENTALIZATION == 0)
    BaseType_t xRunningPrivileged = xPortRaisePrivilege();
#endif

    ( void ) file;
    #if PLATFORM_SPIKE || PLATFORM_SAIL
        return htif_console_write_polled( ptr, len );
    #elif PLATFORM_QEMU_VIRT || PLATFORM_PICCOLO || PLATFORM_GFE || PLATFORM_FETT
        return uart16550_txbuffer( ( uint8_t * ) ptr, len );
    #elif PLATFORM_RVBS
        return plat_console_write( ptr, len );
    #else
    #error "Unsupported Console for this PLATFORM"
    #endif
#if (configENABLE_MPU == 1 && configMPU_COMPARTMENTALIZATION == 0)
    vPortResetPrivilege( xRunningPrivileged );
#endif
}

int _open( const char * name,
           int flags,
           int mode )
{
    errno = ENOSYS;
    return -1;
}

int _close( int fd )
{
    ( void ) fd;
    errno = EBADF;
    return -1;
}

long _lseek( int fd,
             long offset,
             int origin )
{
    ( void ) fd;
    ( void ) offset;
    ( void ) origin;
    errno = EBADF;
    return -1;
}

int _read( int fd,
           void * buffer,
           unsigned int count )
{
    ( void ) fd;
    ( void ) buffer;
    ( void ) count;
    errno = EBADF;
    return -1;
}

int _stat( const char * name,
           void * buffer )
{
    errno = EACCES;
    return -1;
}

int _fstat( int fd,
            void * buffer )
{
    ( void ) fd;
    ( void ) buffer;
    errno = EBADF;
    return -1;
}
#endif

void * _sbrk( int nbytes )
{
    ( void ) nbytes;
    errno = ENOMEM;
    return ( void * ) -1;
}

int _isatty( int fd )
{
    ( void ) fd;
    errno = EBADF;
    return 0;
}

int _kill( int pid,
           int sig )
{
    ( void ) pid;
    ( void ) sig;
    errno = EBADF;
    return -1;
}

int _getpid( int n )
{
    ( void ) n;
    return 1;
}

int _gettimeofday( struct timeval * tv,
                   struct timezone * tz )
{
    uint64_t us = portGET_RUN_TIME_COUNTER_VALUE();

    tv->tv_sec = us / 1000000;
    tv->tv_usec = ( us % 1000000 );
    return 0;
}

void _exit( int x )
{
    portDISABLE_INTERRUPTS();

    do
    {
        printf("Shutting Down...\n");
        #if PLATFORM_QEMU_VIRT || PLATFORM_SPIKE || PLATFORM_SAIL || PLATFORM_FETT
            vTerminate( x );
        #else
        #warning "Unsupported exit syscall for this PLATFORM"
        #endif
    } while( 1 );
}
