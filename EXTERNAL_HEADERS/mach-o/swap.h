#ifndef XNU_EXTERNAL_HEADERS_MACH_O_SWAP_H
#define XNU_EXTERNAL_HEADERS_MACH_O_SWAP_H

#include <mach-o/loader.h>
#include <mach-o/nlist.h>

/* XNU's supported host and target architectures are little-endian. */
static inline void swap_mach_header(struct mach_header *value,
                                    enum NXByteOrder order) {
  (void)value;
  (void)order;
}
static inline void swap_mach_header_64(struct mach_header_64 *value,
                                       enum NXByteOrder order) {
  (void)value;
  (void)order;
}
static inline void swap_segment_command(struct segment_command *value,
                                        enum NXByteOrder order) {
  (void)value;
  (void)order;
}
static inline void swap_segment_command_64(struct segment_command_64 *value,
                                           enum NXByteOrder order) {
  (void)value;
  (void)order;
}
static inline void swap_symtab_command(struct symtab_command *value,
                                       enum NXByteOrder order) {
  (void)value;
  (void)order;
}
static inline void swap_uuid_command(struct uuid_command *value,
                                     enum NXByteOrder order) {
  (void)value;
  (void)order;
}
static inline void swap_nlist(struct nlist *value, uint32_t count,
                              enum NXByteOrder order) {
  (void)value;
  (void)count;
  (void)order;
}
static inline void swap_nlist_64(struct nlist_64 *value, uint32_t count,
                                 enum NXByteOrder order) {
  (void)value;
  (void)count;
  (void)order;
}

#endif
