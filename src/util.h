#ifndef __subst_util_h
#define __subst_util_h

#include <stdlib.h>

#include "log.h"

#define PANIC(message, ...)                                                    \
  subst_log("\n\e[1;31mPANIC\e[0m in \e[0;93m%s\e[0m: ", __func__);            \
  subst_log(message, ##__VA_ARGS__);                                           \
  exit(1);

#endif
