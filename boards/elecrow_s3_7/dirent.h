// Minimal <dirent.h> shim for the ESP32-S3 (xtensa-esp32s3-elf) toolchain
// bundled with pioarduino platform-espressif32 55.03.39, whose own <dirent.h>
// is a stub that emits `#error "<dirent.h> not supported"` (see
// xtensa-esp32s3-elf/sys-include/sys/dirent.h). ESP-IDF v5.x normally injects a
// working header via newlib/platform_include/sys/dirent.h, but that file is
// absent from the framework-arduinoespressif32-libs build we pin, so any
// library that includes <dirent.h> (ESP32-PSRamFS/pfs.c, FFat, etc.) fails to
// compile. Providing this shim on the board's include path (-Iboards/elecrow_s3_7,
// which is searched before the toolchain sys-include) restores a compilable
// POSIX-ish dirent surface. Only used at build time; the ESP-IDF VFS layer
// still provides the actual opendir/readdir/... runtime implementations at link
// time, so type-compatibility here is all we need.

#ifndef _ELECROW_S3_7_DIRENT_SHIM_H_
#define _ELECROW_S3_7_DIRENT_SHIM_H_
#define _DIRENT_H_
#define _SYS_DIRENT_H_
#define _DIRENT_H

#include <sys/types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DT_UNKNOWN
#define DT_UNKNOWN 0
#define DT_FIFO    1
#define DT_CHR     2
#define DT_DIR     4
#define DT_BLK     6
#define DT_REG     8
#define DT_LNK     10
#define DT_SOCK    12
#define DT_WHT     14
#endif

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

// Match ESP-IDF esp_vfs.h layout exactly so both sources can coexist across
// the same translation unit (esp_vfs.h skips its own definitions when
// __DIRENT_DEFINED is set).
#ifndef __DIRENT_DEFINED
#define __DIRENT_DEFINED
typedef struct dirent {
    unsigned long  d_ino;
    unsigned char  d_type;
    char           d_name[256];
} dirent;
typedef struct __compat_DIR DIR;
#endif

DIR           *opendir(const char *name);
struct dirent *readdir(DIR *pdir);
int            readdir_r(DIR *pdir, struct dirent *entry, struct dirent **out);
long           telldir(DIR *pdir);
void           seekdir(DIR *pdir, long loc);
void           rewinddir(DIR *pdir);
int            closedir(DIR *pdir);

#ifdef __cplusplus
}
#endif

#endif // _ELECROW_S3_7_DIRENT_SHIM_H_
