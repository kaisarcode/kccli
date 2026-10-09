#ifndef WEBP2P_SYS_SOCKET_COMPAT_H
#define WEBP2P_SYS_SOCKET_COMPAT_H

#ifdef _WIN32
#include "../win32_compat.h"
#else
#include_next <sys/socket.h>
#endif

#endif
