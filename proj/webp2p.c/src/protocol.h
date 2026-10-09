/**
 * protocol.h - WebP2P index protocol declarations.
 * Summary: Declares index request types and public protocol operations.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: https://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef WEBP2P_PROTOCOL_H
#define WEBP2P_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define WEBP2P_MAX_PENDING 4

typedef struct {
    char *type;
    char *sdp;
} webp2p_description_t;

typedef struct {
    char connection[33];
    char capability[65];
    uint64_t expires_at;
} webp2p_connection_t;

typedef struct {
    char connection[33];
    webp2p_description_t offer;
} webp2p_pending_t;

typedef struct {
    const char *index;
    char secret[17];
    uint64_t seq;
} webp2p_publisher_t;

int webp2p_protocol_init(void);
void webp2p_protocol_cleanup(void);
uint64_t webp2p_now_ms(void);
int webp2p_validate_id(const char *id);
void webp2p_description_free(webp2p_description_t *description);
void webp2p_pending_free(webp2p_pending_t *pending);

int webp2p_pub_register(webp2p_publisher_t *publisher, const char *index,
    const char *id, const char *pass);
int webp2p_pub_heartbeat(webp2p_publisher_t *publisher, const char *id);
int webp2p_pub_poll(webp2p_publisher_t *publisher, const char *id,
    webp2p_pending_t pending[WEBP2P_MAX_PENDING], size_t *count);
int webp2p_pub_answer(webp2p_publisher_t *publisher, const char *id,
    const char *connection, const webp2p_description_t *answer);
int webp2p_pub_deregister(webp2p_publisher_t *publisher, const char *id);

int webp2p_con_connect(const char *index, const char *id,
    const webp2p_description_t *offer, webp2p_connection_t *connection);
int webp2p_con_poll(const char *index, const webp2p_connection_t *connection,
    webp2p_description_t *answer, int *has_answer);

#endif
