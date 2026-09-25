#ifndef XNU_EXTERNAL_HEADERS_MACH_O_ARCH_H
#define XNU_EXTERNAL_HEADERS_MACH_O_ARCH_H

#include <architecture/byte_order.h>
#include <mach/machine.h>
#include <stddef.h>

typedef struct NXArchInfo {
  const char *name;
  cpu_type_t cputype;
  cpu_subtype_t cpusubtype;
  enum NXByteOrder byteorder;
  const char *description;
} NXArchInfo;

static inline const NXArchInfo *NXGetLocalArchInfo(void) {
#if defined(__x86_64__)
  static const NXArchInfo arch = {"x86_64", 0x01000007, 3, NX_LittleEndian,
                                  NULL};
#elif defined(__aarch64__) || defined(__arm64__)
  static const NXArchInfo arch = {"arm64", 0x0100000c, 0, NX_LittleEndian,
                                  NULL};
#else
#error unsupported host architecture
#endif
  return &arch;
}

#endif
