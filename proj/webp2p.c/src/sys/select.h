#ifndef WEBP2P_SYS_SELECT_COMPAT_H
#define WEBP2P_SYS_SELECT_COMPAT_H

#ifdef _WIN32
#include "../win32_compat.h"
#else
#include_next <sys/select.h>
#endif

#endif
