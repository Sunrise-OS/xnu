#ifndef XNU_EXTERNAL_HEADERS_MACH_INIT_H
#define XNU_EXTERNAL_HEADERS_MACH_INIT_H

#include <stddef.h>
#include <stdint.h>

typedef uintptr_t vm_offset_t;
typedef size_t vm_size_t;
typedef int boolean_t;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#ifndef MAP_FILE
#define MAP_FILE 0
#endif
#ifndef __private_extern__
#define __private_extern__ extern
#endif

#define mach_vm_round_page(value) (((value) + 4095) & ~(off_t)4095)

#endif
