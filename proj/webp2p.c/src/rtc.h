/**
 * rtc.h - WebP2P RTC peer declarations.
 * Summary: Declares RTC peer lifecycle and data-channel operations.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: https://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef WEBP2P_RTC_H
#define WEBP2P_RTC_H

#include <stddef.h>

#include "protocol.h"

typedef struct webp2p_rtc_peer webp2p_rtc_peer_t;
typedef void (*webp2p_rtc_receive_fn)(const void *data, size_t size,
    void *userdata);
typedef void (*webp2p_rtc_close_fn)(void *userdata);

int webp2p_rtc_init(void);
void webp2p_rtc_cleanup(void);
webp2p_rtc_peer_t *webp2p_rtc_create(int initiator, const char *stun,
    const char *turn, const char *turn_user, const char *turn_pass,
    int force_turn, int ordered, webp2p_rtc_receive_fn receive,
    webp2p_rtc_close_fn closed, void *userdata);
int webp2p_rtc_set_remote(webp2p_rtc_peer_t *peer,
    const webp2p_description_t *description);
int webp2p_rtc_make_local(webp2p_rtc_peer_t *peer, const char *type,
    webp2p_description_t *description, int timeout_ms);
int webp2p_rtc_wait_open(webp2p_rtc_peer_t *peer, int timeout_ms);
int webp2p_rtc_send(webp2p_rtc_peer_t *peer, const void *data, size_t size);
int webp2p_rtc_drain(webp2p_rtc_peer_t *peer, int timeout_ms);
void webp2p_rtc_close(webp2p_rtc_peer_t *peer);

#endif
