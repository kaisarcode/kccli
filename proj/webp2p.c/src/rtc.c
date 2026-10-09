/**
 * rtc.c - WebP2P RTC peer implementation.
 * Summary: Creates WebRTC peers and bridges data-channel events.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: https://www.gnu.org/licenses/gpl-3.0.html
 */

#define _POSIX_C_SOURCE 200809L
#include "rtc.h"

#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <rtc/rtc.h>

#define WEBP2P_RTC_CLOSE_DRAIN_MS 2000

struct webp2p_rtc_peer {
    int pc;
    int dc;
    int gathering_complete;
    int open;
    int failed;
    int closing;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    webp2p_rtc_receive_fn receive;
    webp2p_rtc_close_fn closed;
    void *userdata;
};

/**
 * Signals peer state changes.
 * @param peer RTC peer.
 * @return None.
 */
static void webp2p_rtc_signal(webp2p_rtc_peer_t *peer)
{
    pthread_cond_broadcast(&peer->cond);
}

/**
 * Gets the current time.
 * @return Time in milliseconds.
 */
static uint64_t webp2p_rtc_now_ms(void)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return 0;
    return (uint64_t)now.tv_sec * 1000u + (uint64_t)now.tv_nsec / 1000000u;
}

/**
 * Sleeps for a duration.
 * @param milliseconds Sleep duration.
 * @return None.
 */
static void webp2p_rtc_sleep(unsigned milliseconds)
{
    struct timespec delay;

    delay.tv_sec = (time_t)(milliseconds / 1000u);
    delay.tv_nsec = (long)(milliseconds % 1000u) * 1000000L;
    (void)nanosleep(&delay, NULL);
}

/**
 * Handles peer state changes.
 * @param pc Peer connection handle.
 * @param state New peer state.
 * @param userdata RTC peer.
 * @return None.
 */
static void webp2p_rtc_state(int pc, rtcState state, void *userdata)
{
    webp2p_rtc_peer_t *peer = (webp2p_rtc_peer_t *)userdata;
    webp2p_rtc_close_fn closed = NULL;
    void *closed_data = NULL;

    (void)pc;
    pthread_mutex_lock(&peer->mutex);
    if (state == RTC_FAILED || state == RTC_CLOSED) {
        if (!peer->failed) {
            peer->failed = 1;
            if (!peer->closing) {
                closed = peer->closed;
                closed_data = peer->userdata;
            }
        }
    }
    webp2p_rtc_signal(peer);
    pthread_mutex_unlock(&peer->mutex);
    if (closed) closed(closed_data);
}

/**
 * Handles ICE gathering changes.
 * @param pc Peer connection handle.
 * @param state New gathering state.
 * @param userdata RTC peer.
 * @return None.
 */
static void webp2p_rtc_gathering(int pc, rtcGatheringState state,
    void *userdata)
{
    webp2p_rtc_peer_t *peer = (webp2p_rtc_peer_t *)userdata;

    (void)pc;
    if (state != RTC_GATHERING_COMPLETE) return;
    pthread_mutex_lock(&peer->mutex);
    peer->gathering_complete = 1;
    webp2p_rtc_signal(peer);
    pthread_mutex_unlock(&peer->mutex);
}

/**
 * Handles data-channel opening.
 * @param dc Data-channel handle.
 * @param userdata RTC peer.
 * @return None.
 */
static void webp2p_rtc_open(int dc, void *userdata)
{
    webp2p_rtc_peer_t *peer = (webp2p_rtc_peer_t *)userdata;

    (void)dc;
    pthread_mutex_lock(&peer->mutex);
    peer->open = 1;
    webp2p_rtc_signal(peer);
    pthread_mutex_unlock(&peer->mutex);
}

/**
 * Handles data-channel closure.
 * @param dc Data-channel handle.
 * @param userdata RTC peer.
 * @return None.
 */
static void webp2p_rtc_channel_closed(int dc, void *userdata)
{
    webp2p_rtc_peer_t *peer = (webp2p_rtc_peer_t *)userdata;
    webp2p_rtc_close_fn closed = NULL;
    void *closed_data = NULL;

    (void)dc;
    pthread_mutex_lock(&peer->mutex);
    if (!peer->failed) {
        peer->failed = 1;
        if (!peer->closing) {
            closed = peer->closed;
            closed_data = peer->userdata;
        }
    }
    webp2p_rtc_signal(peer);
    pthread_mutex_unlock(&peer->mutex);
    if (closed) closed(closed_data);
}

/**
 * Handles a data-channel message.
 * @param dc Data-channel handle.
 * @param message Message data.
 * @param size Message size.
 * @param userdata RTC peer.
 * @return None.
 */
static void webp2p_rtc_message(int dc, const char *message, int size,
    void *userdata)
{
    webp2p_rtc_peer_t *peer = (webp2p_rtc_peer_t *)userdata;
    webp2p_rtc_receive_fn receive;
    void *receive_data;
    size_t length;

    (void)dc;
    if (!message || size == 0) return;
    length = (size_t)(size < 0 ? -size : size);
    pthread_mutex_lock(&peer->mutex);
    receive = peer->closing ? NULL : peer->receive;
    receive_data = peer->userdata;
    pthread_mutex_unlock(&peer->mutex);
    if (receive) receive(message, length, receive_data);
}

/**
 * Configures a data channel.
 * @return None.
 */
static void webp2p_rtc_setup_channel(webp2p_rtc_peer_t *peer, int dc)
{
    peer->dc = dc;
    rtcSetUserPointer(dc, peer);
    rtcSetOpenCallback(dc, webp2p_rtc_open);
    rtcSetClosedCallback(dc, webp2p_rtc_channel_closed);
    rtcSetMessageCallback(dc, webp2p_rtc_message);
}

/**
 * Handles a new data channel.
 * @return None.
 */
static void webp2p_rtc_data_channel(int pc, int dc, void *userdata)
{
    webp2p_rtc_peer_t *peer = (webp2p_rtc_peer_t *)userdata;
    int reject = 0;

    (void)pc;
    pthread_mutex_lock(&peer->mutex);
    if (peer->dc < 0 && !peer->closing)
        webp2p_rtc_setup_channel(peer, dc);
    else
        reject = 1;
    webp2p_rtc_signal(peer);
    pthread_mutex_unlock(&peer->mutex);
    if (reject) rtcDeleteDataChannel(dc);
}

/**
 * Waits for an RTC peer condition.
 * @return 0 on success, or -1 on timeout.
 */
static int webp2p_rtc_wait(webp2p_rtc_peer_t *peer, int *flag,
    int timeout_ms)
{
    struct timespec deadline;
    int rc = 0;

    if (clock_gettime(CLOCK_REALTIME, &deadline) != 0) return -1;
    deadline.tv_sec += timeout_ms / 1000;
    deadline.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }

    pthread_mutex_lock(&peer->mutex);
    while (!*flag && !peer->failed && !peer->closing) {
        rc = pthread_cond_timedwait(&peer->cond, &peer->mutex, &deadline);
        if (rc != 0) break;
    }
    rc = *flag ? 0 : -1;
    pthread_mutex_unlock(&peer->mutex);
    return rc;
}

/**
 * Builds an RTC TURN server URI.
 * @return Allocated URI, or NULL on failure.
 */
static char *webp2p_rtc_turn_uri(const char *turn, const char *user,
    const char *pass)
{
    const char *host;
    char *out;
    size_t size;

    if (!turn || !turn[0]) return NULL;
    if (!user || !user[0] || !pass || !pass[0]) return strdup(turn);
    host = turn;
    if (strncmp(host, "turn://", 7) == 0) host += 7;
    else if (strncmp(host, "turn:", 5) == 0) host += 5;
    size = strlen(user) + strlen(pass) + strlen(host) + 8;
    out = (char *)malloc(size);
    if (!out) return NULL;
    (void)snprintf(out, size, "turn:%s:%s@%s", user, pass, host);
    return out;
}

/**
 * Initializes the RTC library.
 * @return 0 on success, or -1 on failure.
 */
int webp2p_rtc_init(void)
{
    rtcInitLogger(RTC_LOG_WARNING, NULL);
    return rtcPreload() ? 0 : -1;
}

/**
 * Cleans up the RTC library.
 * @return None.
 */
void webp2p_rtc_cleanup(void)
{
    rtcCleanup();
}

/**
 * Creates an RTC peer.
 * @return New peer, or NULL on failure.
 */
webp2p_rtc_peer_t *webp2p_rtc_create(int initiator, const char *stun,
    const char *turn, const char *turn_user, const char *turn_pass,
    int force_turn, int ordered, webp2p_rtc_receive_fn receive,
    webp2p_rtc_close_fn closed, void *userdata)
{
    webp2p_rtc_peer_t *peer;
    rtcConfiguration config;
    const char *servers[2];
    int server_count = 0;
    char *turn_uri;

    turn_uri = webp2p_rtc_turn_uri(turn, turn_user, turn_pass);
    if (turn && turn[0] && !turn_uri) return NULL;
    peer = (webp2p_rtc_peer_t *)calloc(1, sizeof(*peer));
    if (!peer) {
        free(turn_uri);
        return NULL;
    }
    peer->pc = -1;
    peer->dc = -1;
    peer->receive = receive;
    peer->closed = closed;
    peer->userdata = userdata;
    if (pthread_mutex_init(&peer->mutex, NULL) != 0) {
        free(turn_uri);
        free(peer);
        return NULL;
    }
    if (pthread_cond_init(&peer->cond, NULL) != 0) {
        pthread_mutex_destroy(&peer->mutex);
        free(turn_uri);
        free(peer);
        return NULL;
    }

    memset(&config, 0, sizeof(config));
    config.disableAutoNegotiation = true;
    config.iceTransportPolicy = force_turn ? RTC_TRANSPORT_POLICY_RELAY :
        RTC_TRANSPORT_POLICY_ALL;
    if (stun && stun[0]) servers[server_count++] = stun;
    if (turn_uri && turn_uri[0]) servers[server_count++] = turn_uri;
    config.iceServers = servers;
    config.iceServersCount = server_count;

    peer->pc = rtcCreatePeerConnection(&config);
    free(turn_uri);
    if (peer->pc < 0) {
        webp2p_rtc_close(peer);
        return NULL;
    }
    rtcSetUserPointer(peer->pc, peer);
    rtcSetStateChangeCallback(peer->pc, webp2p_rtc_state);
    rtcSetGatheringStateChangeCallback(peer->pc, webp2p_rtc_gathering);
    rtcSetDataChannelCallback(peer->pc, webp2p_rtc_data_channel);

    if (initiator) {
        rtcDataChannelInit init;
        int dc;

        memset(&init, 0, sizeof(init));
        init.reliability.unordered = ordered ? false : true;
        dc = rtcCreateDataChannelEx(peer->pc, "redp2p", &init);
        if (dc < 0) {
            webp2p_rtc_close(peer);
            return NULL;
        }
        pthread_mutex_lock(&peer->mutex);
        webp2p_rtc_setup_channel(peer, dc);
        pthread_mutex_unlock(&peer->mutex);
    }
    return peer;
}

/**
 * Sets an RTC remote description.
 * @return 0 on success, or -1 on failure.
 */
int webp2p_rtc_set_remote(webp2p_rtc_peer_t *peer,
    const webp2p_description_t *description)
{
    if (!peer || !description || !description->type || !description->sdp)
        return -1;
    return rtcSetRemoteDescription(peer->pc, description->sdp,
        description->type) < 0 ? -1 : 0;
}

/**
 * Creates an RTC local description.
 * @return 0 on success, or -1 on failure.
 */
int webp2p_rtc_make_local(webp2p_rtc_peer_t *peer, const char *type,
    webp2p_description_t *description, int timeout_ms)
{
    int size;
    char *sdp;
    char type_buffer[32];

    if (!peer || !type || !description || timeout_ms <= 0) return -1;
    memset(description, 0, sizeof(*description));
    pthread_mutex_lock(&peer->mutex);
    peer->gathering_complete = 0;
    pthread_mutex_unlock(&peer->mutex);
    if (rtcSetLocalDescription(peer->pc, type) < 0) return -1;
    if (webp2p_rtc_wait(peer, &peer->gathering_complete, timeout_ms) != 0)
        return -1;

    size = rtcGetLocalDescription(peer->pc, NULL, 0);
    if (size <= 1 || size > 49153) return -1;
    sdp = (char *)malloc((size_t)size);
    if (!sdp) return -1;
    if (rtcGetLocalDescription(peer->pc, sdp, size) < 0 ||
        rtcGetLocalDescriptionType(peer->pc, type_buffer,
            (int)sizeof(type_buffer)) < 0)
    {
        free(sdp);
        return -1;
    }
    description->sdp = sdp;
    description->type = strdup(type_buffer);
    if (!description->type) {
        webp2p_description_free(description);
        return -1;
    }
    return 0;
}

/**
 * Waits for an RTC data channel.
 * @return 0 on success, or -1 on failure.
 */
int webp2p_rtc_wait_open(webp2p_rtc_peer_t *peer, int timeout_ms)
{
    if (!peer || timeout_ms <= 0) return -1;
    return webp2p_rtc_wait(peer, &peer->open, timeout_ms);
}

/**
 * Sends RTC data.
 * @return 0 on success, or -1 on failure.
 */
int webp2p_rtc_send(webp2p_rtc_peer_t *peer, const void *data, size_t size)
{
    int dc;

    if (!peer || (!data && size != 0) || size > INT_MAX) return -1;
    pthread_mutex_lock(&peer->mutex);
    dc = (!peer->closing && peer->open && !peer->failed) ? peer->dc : -1;
    pthread_mutex_unlock(&peer->mutex);
    if (dc < 0) return -1;
    return rtcSendMessage(dc, (const char *)data, (int)size) < 0 ? -1 : 0;
}

/**
 * Drains pending RTC data.
 * @return 0 on success, or -1 on failure.
 */
int webp2p_rtc_drain(webp2p_rtc_peer_t *peer, int timeout_ms)
{
    uint64_t deadline;

    if (!peer || timeout_ms < 0) return -1;
    deadline = webp2p_rtc_now_ms() + (uint64_t)timeout_ms;
    for (;;) {
        int dc;
        int failed;
        int buffered;

        pthread_mutex_lock(&peer->mutex);
        dc = peer->dc;
        failed = peer->failed || peer->closing;
        pthread_mutex_unlock(&peer->mutex);
        if (failed || dc < 0) return -1;
        buffered = rtcGetBufferedAmount(dc);
        if (buffered < 0) return -1;
        if (buffered == 0) return 0;
        if (webp2p_rtc_now_ms() >= deadline) return -1;
        webp2p_rtc_sleep(10);
    }
}

/**
 * Closes an RTC peer.
 * @return None.
 */
void webp2p_rtc_close(webp2p_rtc_peer_t *peer)
{
    int dc;
    int pc;

    if (!peer) return;
    (void)webp2p_rtc_drain(peer, WEBP2P_RTC_CLOSE_DRAIN_MS);
    pthread_mutex_lock(&peer->mutex);
    peer->closing = 1;
    peer->receive = NULL;
    peer->closed = NULL;
    dc = peer->dc;
    pc = peer->pc;
    peer->dc = -1;
    peer->pc = -1;
    webp2p_rtc_signal(peer);
    pthread_mutex_unlock(&peer->mutex);
    if (dc >= 0) rtcDeleteDataChannel(dc);
    if (pc >= 0) rtcDeletePeerConnection(pc);
    pthread_cond_destroy(&peer->cond);
    pthread_mutex_destroy(&peer->mutex);
    free(peer);
}
