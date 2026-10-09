#ifndef WEBP2P_WIN32_COMPAT_H
#define WEBP2P_WIN32_COMPAT_H

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stddef.h>
#include <stdint.h>

#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifndef ssize_t
typedef SSIZE_T ssize_t;
#endif

static inline int webp2p_win_close(SOCKET fd)
{
    return closesocket(fd);
}

static inline int webp2p_win_send(SOCKET fd, const void *data,
    size_t size, int flags)
{
    int count = size > INT_MAX ? INT_MAX : (int)size;
    return send(fd, (const char *)data, count, flags);
}

static inline int webp2p_win_recv(SOCKET fd, void *data,
    size_t size, int flags)
{
    int count = size > INT_MAX ? INT_MAX : (int)size;
    return recv(fd, (char *)data, count, flags);
}

static inline int webp2p_win_setsockopt(SOCKET fd, int level, int option,
    const void *value, int size)
{
    return setsockopt(fd, level, option, (const char *)value, size);
}

#define close(fd) webp2p_win_close((SOCKET)(fd))
#define send(fd, data, size, flags) \
    webp2p_win_send((SOCKET)(fd), (data), (size), (flags))
#define recv(fd, data, size, flags) \
    webp2p_win_recv((SOCKET)(fd), (data), (size), (flags))
#define setsockopt(fd, level, option, value, size) \
    webp2p_win_setsockopt((SOCKET)(fd), (level), (option), (value), (int)(size))

#endif

#endif
