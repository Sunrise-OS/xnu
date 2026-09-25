/*
 * Board support header for QEMU's aarch64 "virt" machine
 * (`qemu-system-aarch64 -M virt -cpu cortex-a53`), booted the same way as
 * SUPERBIRD: u-boot's generic `bootxnu` command (../u-boot/cmd/bootxnu.c -
 * board-agnostic, just parses the Mach-O this build produces and jumps
 * into it with a boot_args struct) run from u-boot built with
 * qemu_arm64_defconfig.
 *
 * `-cpu cortex-a53` is plain ARMv8.0-A, same silicon class as SUPERBIRD:
 * no PAC, no PPL/SPTM, no KTRR/AMCC, no Apple Interrupt Controller.
 * Everything gated behind those features must stay off, same as
 * SUPERBIRD.h.
 */

#ifndef _PEXPERT_ARM64_QEMU_H
#define _PEXPERT_ARM64_QEMU_H

#define NO_MONITOR                1
#define QEMU                      1
/*
 * Generic flag for non-Apple-silicon boards: no Tightbeam/exclaves (no
 * SEP/secure-enclave coprocessor to talk to), no per-SoC blob.
 */
#define OSS_HARDWARE               1
#define NO_ECORE                  1

/*
 * No Apple Silicon PAC/CTRR/SME/PAN3 - `-cpu cortex-a53` is plain
 * ARMv8.0-A. Deliberately do NOT define CPU_HAS_APPLE_PAC,
 * HAS_PARAVIRTUALIZED_PAC, HAS_PARAVIRTUALIZED_CTRR,
 * HAS_ARM_FEAT_SSBS2/SME/SME2/PAN3: none exist on this CPU model and
 * defining them would emit instructions that #UD here.
 */

/*
 * 16K pages, as on VMAPPLE. The MacOSX platform selects ARM_LARGE_MEMORY
 * (VM_KERNEL_LINK_ADDRESS 0xfffffe0007004000, 41 significant bits), which
 * only fits the 16K-page TTBR1 (T1SZ_BOOT 17); with 4K pages T1SZ_BOOT is
 * 25 (39-bit TTBR1 from 0xffffff8000000000) and the first kernel-VA fetch
 * after SCTLR_EL1.M takes a level-0 translation fault. Needs a guest CPU
 * that implements the 16K granule (ID_AA64MMFR0_EL1.TGran16), e.g. QEMU
 * `-cpu cortex-a76`, `neoverse-n1` or `max`; cortex-a53/a57 do not.
 */
#define __ARM_16K_PG__            1
#define __ARM_RANGE_TLBI__        0

#define ARM_PARAMETERIZED_PMAP    1

#include <pexpert/arm64/apple_arm64_common.h>
#undef  BTI_ENFORCED
#define BTI_ENFORCED 0
#undef  __ARM64_PMAP_SUBPAGE_L1__
#undef  __ARM64_PMAP_KERN_SUBPAGE_L1__
/* Crypto extensions unconfirmed for QEMU's cortex-a53 model - use generic C. */
#undef  __ARM_V8_CRYPTO_EXTENSIONS__
/* This is not Apple silicon: no coherent-fabric assumptions, no Apple
 * cache-maintenance helpers (nopreempt/mva_ops), no eng-fused sysctl. */
#undef  APPLE_ARM64_ARCH_FAMILY

/*
 * Console: PL011 UART at 0x09000000 (`-M virt`'s pl011@9000000). The
 * driver already exists in pexpert/arm/pe_serial.c, gated on
 * PL011_UART; it just needed a board that enables it and an AFDT
 * describing the device (arm-io/defaults/pl011 nodes - see
 * tools/qemu-boot/boot.zig's AFDT builder, the only source of the
 * device tree this board boots with, and boot_args.command_line's
 * `serial-device-name=uart0` selecting it via pe_serial.c's
 * get_serial_device_phandle()).
 */
#define PL011_UART 1

#ifndef ASSEMBLER
#define PLATFORM_PANIC_LOG_DISABLED
#endif /* ! ASSEMBLER */

/*
 * GICv2, `-M virt`'s default (`gic-version=2`; `gic-version=3` would need
 * the ICC_*-system-register driver in pexpert/arm/pe_fiq.c instead, not
 * this MMIO one):
 *   GICD @ 0x08000000 (distributor)
 *   GICC @ 0x08010000 (CPU interface, MMIO - no ICC_* system registers)
 * Same architecture as SUPERBIRD's GIC-400 - HAS_GIC_V3 must stay
 * undefined here. The physical bases are injected into
 * pexpert/gic/gic.zig at build-graph time (tools/zig/boards.zig
 * GicBases), not read from this header; they are restated here only for
 * documentation.
 */
#define GIC_SPURIOUS_IRQ          1023

#define GICD_PHYS_BASE            0x08000000ULL
#define GICD_SIZE                 0x1000
#define GICC_PHYS_BASE            0x08010000ULL
#define GICC_SIZE                 0x1000

#define GICD_CTLR                 0x0
#define GICD_CTLR_ENABLEGRP0      0x1
#define GICD_CTLR_ENABLEGRP1      0x2

#define GICC_CTLR                 0x0
#define GICC_PMR                  0x4
#define GICC_BPR                  0x8
#define GICC_IAR                  0xc
#define GICC_EOIR                 0x10
#define GICC_CTLR_ENABLEGRP0      0x1
#define GICC_CTLR_ENABLEGRP1      0x2

#endif /* ! _PEXPERT_ARM64_QEMU_H */
