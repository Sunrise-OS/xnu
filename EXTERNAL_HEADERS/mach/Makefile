export MakeInc_cmd=${SRCROOT}/makedefs/MakeInc.cmd
export MakeInc_def=${SRCROOT}/makedefs/MakeInc.def
export MakeInc_rule=${SRCROOT}/makedefs/MakeInc.rule
export MakeInc_dir=${SRCROOT}/makedefs/MakeInc.dir

include $(MakeInc_cmd)
include $(MakeInc_def)

INSTINC_SUBDIRS = machine

EXPORT_FILES = \
	mach_init.h \
	machine.h \
	vm_prot.h

INSTALL_MI_LIST =
INSTALL_MI_DIR =
EXPORT_MI_LIST = ${EXPORT_FILES}
EXPORT_MI_DIR = mach

include $(MakeInc_rule)
include $(MakeInc_dir)
