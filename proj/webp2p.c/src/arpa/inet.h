#ifndef WEBP2P_ARPA_INET_COMPAT_H
#define WEBP2P_ARPA_INET_COMPAT_H

#ifdef _WIN32
#include "../win32_compat.h"
#else
#include_next <arpa/inet.h>
#endif

#endif
