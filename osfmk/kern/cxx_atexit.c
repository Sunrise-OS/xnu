/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Itanium C++ ABI exit-time destructor registration.
 *
 * clang emits __cxa_atexit() for namespace-scope C++ statics that have
 * destructors (LIBKERN_ALWAYS_DESTROY), such as libsa/bootstrap.cpp's
 * sBootstrapObject.  Apple's kernel gets __cxa_atexit from libcc_kext,
 * which is built from Apple's Libc stdlib/FreeBSD/atexit.c.  The OSS
 * hardware configurations (QEMU, SUPERBIRD, IPAD41) link no libcc_kext,
 * so this is that implementation ported to kernel primitives:
 *
 *  - a kernel hw_lock replaces pthread's atexit_mutex;
 *  - the head of the registration list is a static object, so no early
 *    allocation is needed, and overflow entries come from kalloc();
 *  - dlfcn/Block support is dropped: the monolithic kernel never
 *    dlcloses a linkage unit and the C++ ABI has no block handlers.
 *
 * The ABI is kept intact: handlers are recorded and __cxa_finalize() /
 * __cxa_finalize_ranges() run them, so __cxa_finalize(NULL) drains the
 * list exactly as an Itanium-ABI runtime does.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <mach/vm_types.h>
#include <kern/kern_types.h>
#include <kern/kalloc.h>
#include <kern/locks.h>
#include <kern/simple_lock.h>
#include <kern/zalloc.h>

/* "must be at least 32 to guarantee ANSI conformance" (Libc atexit.h) */
#define ATEXIT_SIZE     128

#define ATEXIT_FN_EMPTY 0
#define ATEXIT_FN_STD   1
#define ATEXIT_FN_CXA   2

struct __cxa_range_t {
	const void *addr;
	size_t length;
};

struct atexit {
	struct atexit *next;                    /* next in list */
	int ind;                                /* next index in this table */
	struct atexit_fn {
		int fn_type;                    /* ATEXIT_? from above */
		union {
			void (*std_func)(void);
			void (*cxa_func)(void *);
		} fn_ptr;                       /* function pointer */
		void *fn_arg;                   /* argument for CXA callback */
		void *fn_dso;                   /* shared module handle */
	} fns[ATEXIT_SIZE];                     /* the table itself */
};

static hw_lock_data_t atexit_lock;
static struct atexit __atexit_head;             /* points to head of LIFO stack */
static struct atexit *__atexit = &__atexit_head;
static int __atexit_new_registration;

/*
 * Register the function described by 'fptr' to be called at application
 * exit or owning shared object unload time.  This is a helper for atexit()
 * and __cxa_atexit().
 */
static int
atexit_register(struct atexit_fn *fptr)
{
	struct atexit *p = __atexit;

	hw_lock_lock(&atexit_lock, LCK_GRP_NULL);
	while (p->ind >= ATEXIT_SIZE) {
		struct atexit *old__atexit;
		struct atexit *new__atexit;

		old__atexit = __atexit;
		hw_lock_unlock(&atexit_lock);
		new__atexit = kalloc_data(sizeof(*new__atexit), Z_WAITOK);
		if (new__atexit == NULL) {
			return -1;
		}
		memset(new__atexit, 0, sizeof(*new__atexit));
		hw_lock_lock(&atexit_lock, LCK_GRP_NULL);
		if (old__atexit != __atexit) {
			/* Lost race, retry operation */
			hw_lock_unlock(&atexit_lock);
			kfree_data(new__atexit, sizeof(*new__atexit));
			hw_lock_lock(&atexit_lock, LCK_GRP_NULL);
			p = __atexit;
			continue;
		}
		new__atexit->ind = 0;
		new__atexit->next = __atexit;
		__atexit = new__atexit;
		p = new__atexit;
	}
	p->fns[p->ind++] = *fptr;
	__atexit_new_registration = 1;
	hw_lock_unlock(&atexit_lock);
	return 0;
}

/*
 * Register a function to be performed at exit.
 */
int
atexit(void (*func)(void))
{
	struct atexit_fn fn;

	fn.fn_type = ATEXIT_FN_STD;
	fn.fn_ptr.std_func = func;
	fn.fn_arg = NULL;
	fn.fn_dso = NULL;

	return atexit_register(&fn);
}

/*
 * Register a function to be performed at exit or when a shared object
 * with the given dso handle is unloaded dynamically.
 */
int
__cxa_atexit(void (*func)(void *), void *arg, void *dso)
{
	struct atexit_fn fn;

	fn.fn_type = ATEXIT_FN_CXA;
	fn.fn_ptr.cxa_func = func;
	fn.fn_arg = arg;
	fn.fn_dso = dso;

	return atexit_register(&fn);
}

static bool
__cxa_in_range(const struct __cxa_range_t ranges[],
    unsigned int count,
    const void *fn)
{
	uintptr_t addr = (uintptr_t)fn;

#if __has_feature(ptrauth_calls)
	/* Strip the pointer so that range checks work.  If we call the
	 * pointer, it will still be signed though. */
	addr = (uintptr_t)__builtin_ptrauth_strip((void *)addr, ptrauth_key_function_pointer);
#endif

	unsigned int i;
	for (i = 0; i < count; ++i) {
		const struct __cxa_range_t *r = &ranges[i];
		if (addr < (uintptr_t)r->addr) {
			continue;
		}
		if (addr < ((uintptr_t)r->addr + r->length)) {
			return true;
		}
	}
	return false;
}

/*
 * Call handlers registered via __cxa_atexit/atexit that are in the
 * range specified.
 * Note: rangeCount==0 means call all handlers.
 */
void
__cxa_finalize_ranges(const struct __cxa_range_t ranges[], unsigned int count)
{
	struct atexit *p;
	struct atexit_fn *fn;
	int n;

	hw_lock_lock(&atexit_lock, LCK_GRP_NULL);

restart:
	for (p = __atexit; p; p = p->next) {
		for (n = p->ind; --n >= 0;) {
			fn = &p->fns[n];

			if (fn->fn_type == ATEXIT_FN_EMPTY) {
				continue;       /* already been called */
			}

			/* Verify that the entry is within the range being unloaded. */
			if (count > 0) {
				if (fn->fn_type == ATEXIT_FN_CXA) {
					/* for __cxa_atexit(), call if *dso* is in the range being unloaded */
					if (!__cxa_in_range(ranges, count, fn->fn_dso)) {
						continue;       /* not being unloaded yet */
					}
				} else if (fn->fn_type == ATEXIT_FN_STD) {
					/* for atexit, call if *function* is in the range being unloaded */
					if (!__cxa_in_range(ranges, count, fn->fn_ptr.std_func)) {
						continue;       /* not being unloaded yet */
					}
				}
			}

			/* Clear the entry to indicate that this handler has been called. */
			int fn_type = fn->fn_type;
			fn->fn_type = ATEXIT_FN_EMPTY;

			/* Detect recursive registrations. */
			__atexit_new_registration = 0;
			hw_lock_unlock(&atexit_lock);

			/* Call the handler. */
			if (fn_type == ATEXIT_FN_CXA) {
				fn->fn_ptr.cxa_func(fn->fn_arg);
			} else if (fn_type == ATEXIT_FN_STD) {
				fn->fn_ptr.std_func();
			}

			/* Call any recursively registered handlers. */
			hw_lock_lock(&atexit_lock, LCK_GRP_NULL);
			if (__atexit_new_registration) {
				goto restart;
			}
		}
	}
	hw_lock_unlock(&atexit_lock);
}

/*
 * Call all handlers registered with __cxa_atexit for the shared
 * object owning 'dso'.  Note: if 'dso' is NULL, then all remaining
 * handlers are called.
 */
void
__cxa_finalize(const void *dso)
{
	if (dso != NULL) {
		struct __cxa_range_t range;
		range.addr = dso;
		range.length = 1;
		__cxa_finalize_ranges(&range, 1);
	} else {
		__cxa_finalize_ranges(NULL, 0);
	}
}
