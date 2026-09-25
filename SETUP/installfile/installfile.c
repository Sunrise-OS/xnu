/*
 * Copyright (c) 2012 Apple, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

#include <sys/fcntl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#ifdef __APPLE__
#include <copyfile.h>
#endif

void usage(void);
static const char *program_name;

static int copy_data(int srcfd, int dstfd) {
#ifdef __APPLE__
  return fcopyfile(srcfd, dstfd, NULL, COPYFILE_DATA);
#else
  char buffer[64 * 1024];
  ssize_t nread;

  while ((nread = read(srcfd, buffer, sizeof(buffer))) > 0) {
    char *cursor = buffer;
    ssize_t remaining = nread;
    while (remaining > 0) {
      ssize_t nwritten = write(dstfd, cursor, remaining);
      if (nwritten < 0) {
        return -1;
      }
      cursor += nwritten;
      remaining -= nwritten;
    }
  }
  return nread < 0 ? -1 : 0;
#endif
}

int main(int argc, char *argv[]) {
  program_name = argv[0];
  struct stat sb;
  void *mset;
  mode_t mode;
  bool gotmode = false;
  int ch;
  int ret;
  int srcfd, dstfd;
  const char *src = NULL;
  const char *dst = NULL;
  char dsttmpname[MAXPATHLEN];

  while ((ch = getopt(argc, argv, "cSm:")) != -1) {
    switch (ch) {
    case 'c':
    case 'S':
      /* ignored for compatibility */
      break;
    case 'm':
      gotmode = true;
#ifdef __APPLE__
      mset = setmode(optarg);
      if (!mset) {
        errx(EX_USAGE, "Unrecognized mode %s", optarg);
      }

      mode = getmode(mset, 0);
      free(mset);
#else
      char *end;
      errno = 0;
      unsigned long parsed_mode = strtoul(optarg, &end, 8);
      if (errno != 0 || *optarg == '\0' || *end != '\0' ||
          parsed_mode > 07777) {
        errx(EX_USAGE, "Unrecognized mode %s", optarg);
      }
      mode = (mode_t)parsed_mode;
#endif
      break;
    case '?':
    default:
      usage();
    }
  }

  argc -= optind;
  argv += optind;

  if (argc < 2) {
    usage();
  }

  src = argv[0];
  dst = argv[1];

  srcfd = open(src, O_RDONLY, 0);
  if (srcfd < 0) {
    err(EX_NOINPUT, "open(%s)", src);
  }

  ret = fstat(srcfd, &sb);
  if (ret < 0) {
    err(EX_NOINPUT, "fstat(%s)", src);
  }

  if (!S_ISREG(sb.st_mode)) {
    err(EX_USAGE, "%s is not a regular file", src);
  }

  snprintf(dsttmpname, sizeof(dsttmpname), "%s.XXXXXX", dst);

  dstfd = mkstemp(dsttmpname);
  if (dstfd < 0) {
    err(EX_UNAVAILABLE, "mkstemp(%s)", dsttmpname);
  }

  ret = copy_data(srcfd, dstfd);
  if (ret < 0) {
    err(EX_UNAVAILABLE, "fcopyfile(%s, %s)", src, dsttmpname);
  }

  ret = futimes(dstfd, NULL);
  if (ret < 0) {
    err(EX_UNAVAILABLE, "futimes(%s)", dsttmpname);
  }

  if (gotmode) {
    ret = fchmod(dstfd, mode);
    if (ret < 0) {
      err(EX_NOINPUT, "fchmod(%s, %ho)", dsttmpname, mode);
    }
  }

  ret = rename(dsttmpname, dst);
  if (ret < 0) {
    err(EX_NOINPUT, "rename(%s, %s)", dsttmpname, dst);
  }

  ret = close(dstfd);
  if (ret < 0) {
    err(EX_NOINPUT, "close(dst)");
  }

  ret = close(srcfd);
  if (ret < 0) {
    err(EX_NOINPUT, "close(src)");
  }

  return 0;
}

void usage(void) {
  fprintf(stderr, "Usage: %s [-c] [-S] [-m <mode>] <src> <dst>\n",
          program_name);
  exit(EX_USAGE);
}
