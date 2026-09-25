export MakeInc_cmd=${SRCROOT}/makedefs/MakeInc.cmd
export MakeInc_def=${SRCROOT}/makedefs/MakeInc.def
export MakeInc_rule=${SRCROOT}/makedefs/MakeInc.rule
export MakeInc_dir=${SRCROOT}/makedefs/MakeInc.dir

include $(MakeInc_cmd)
include $(MakeInc_def)

OBJS = kextsymboltool.o

CFLAGS = $(HOST_SYSROOT_FLAGS) -g -O0 -I$(SOURCE) -I. \
	-I$(SRCROOT)/EXTERNAL_HEADERS

WARNFLAGS = -Wall

ifeq ($(HOST_OS),Darwin)
LDFLAGS = $(HOST_SYSROOT_FLAGS) -lstdc++
HOST_LD = $(HOST_CC)
else
LDFLAGS = $(HOST_SYSROOT_FLAGS)
HOST_LD = $(HOST_CXX)
endif

kextsymboltool: $(OBJS)
	@$(LOG_HOST_LD) "$@"
	$(_v)$(HOST_LD) $(LDFLAGS) -o $@ $^
	@$(LOG_HOST_CODESIGN) "$@"
	$(_v)env CODESIGN_ALLOCATE=$(HOST_CODESIGN_ALLOCATE) $(HOST_CODESIGN) -s - $@

%.o: %.c
	@$(LOG_HOST_CC) "$@"
	$(_v)$(HOST_CC) $(WARNFLAGS) $(CFLAGS) -c -o $@ $<

do_build_setup:: kextsymboltool

include $(MakeInc_rule)
include $(MakeInc_dir)
