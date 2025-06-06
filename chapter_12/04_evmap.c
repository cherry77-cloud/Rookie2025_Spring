/* --------------------- event_io_map / evmap 相关定义 --------------------- */

#ifdef EVMAP_USE_HT
#include "ht-internal.h"
struct event_map_entry;
HT_HEAD(event_io_map, event_map_entry);
#else

#define event_io_map event_signal_map
#endif


struct event_signal_map {
    void  **entries;  /* 用于存放 evmap_io 或 evmap_signal 的指针数组 */
    int     nentries; /* entries 数组的大小（有效槽位数） */
};

#ifdef EVMAP_USE_HT
struct event_map_entry {
    HT_ENTRY(event_map_entry) map_node;
    evutil_socket_t           fd;
    union {
        struct evmap_io evmap_io;
    } ent;
};
#endif

TAILQ_HEAD(event_list, event);

struct evmap_io {
    struct event_list  events;
    ev_uint16_t        nread;
    ev_uint16_t        nwrite;
};

struct evmap_signal {
    struct event_list  events;
};
/* ----------------------- evmap_io_add 函数实现 ----------------------- */

int evmap_io_add(struct event_base *base, evutil_socket_t fd, struct event *ev) {
    /* 获取 event_base 后端 I/O 复用机制的实例指针 */
    const struct eventop *evsel = base->evsel;
    /* 获取 event_base 中，fd => I/O 事件队列（哈希表或数组）的映射表 */
    struct event_io_map *io = &base->io;
    /* fd 对应的 I/O 事件队列上下文（evmap_io*） */
    struct evmap_io *ctx = NULL;
    int nread = 0, nwrite = 0, retval = 0;
    short res = 0, old = 0;
    struct event *old_ev;

    EVUTIL_ASSERT(fd == ev->ev_fd);

    if (fd < 0)
        return (0);

#ifndef EVMAP_USE_HT
    if (fd >= io->nentries) {
        if (evmap_make_space(io, fd, sizeof(struct evmap_io)) == -1)
            return (-1);
    }
#endif
    GET_IO_SLOT_AND_CTOR(ctx, io, fd, evmap_io, evmap_io_init, evsel->fdinfo_len);

    nread  = ctx->nread;
    nwrite = ctx->nwrite;

    if (nread)
        old |= EV_READ;
    if (nwrite)
        old |= EV_WRITE;

    /* 如果 ev 请求监听读事件，就令 nread+1；第一次从 0 增为 1，设置 res|=EV_READ */
    if (ev->ev_events & EV_READ) {
        if (++nread == 1)
            res |= EV_READ;
    }
    /* 如果 ev 请求监听写事件，就令 nwrite+1；第一次从 0 增为 1，设置 res|=EV_WRITE */
    if (ev->ev_events & EV_WRITE) {
        if (++nwrite == 1)
            res |= EV_WRITE;
    }

    /* 超过 0xffff 个事件属于异常，拒绝 */
    if (EVUTIL_UNLIKELY(nread > 0xffff || nwrite > 0xffff)) {
        event_warnx("Too many events reading or writing on fd %d", (int)fd);
        return (-1);
    }

    if (EVENT_DEBUG_MODE_IS_ON()
        && (old_ev = TAILQ_FIRST(&ctx->events))
        && ((old_ev->ev_events & EV_ET) != (ev->ev_events & EV_ET))) {
        event_warnx(
            "Tried to mix edge-triggered and non-edge-triggered "
            "events on fd %d", (int)fd);
        return (-1);
    }

    /* 如果 res != 0，表示本次是第一次为该 fd 注册读或写，需要向底层 I/O 复用器 add */
    if (res) {
        void *extra = ((char*)ctx) + sizeof(struct evmap_io);

        /* 调用底层 I/O 复用器的 add 接口，将 old (旧 mask)、ev->ev_events、res 传递 */
        if (evsel->add(base, ev->ev_fd, old, (ev->ev_events & EV_ET), res, extra) == -1)
            return (-1);

        retval = 1;
    }

    ctx->nread  = (ev_uint16_t)nread;
    ctx->nwrite = (ev_uint16_t)nwrite;
    TAILQ_INSERT_TAIL(&ctx->events, ev, ev_io_next);

    return (retval);
}
