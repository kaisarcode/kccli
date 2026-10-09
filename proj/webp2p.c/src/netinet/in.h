#ifndef WEBP2P_NETINET_IN_COMPAT_H
#define WEBP2P_NETINET_IN_COMPAT_H

#ifdef _WIN32
#include "../win32_compat.h"
#else
#include_next <netinet/in.h>
#endif

#endif
