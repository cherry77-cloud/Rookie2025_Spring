/* ------------------------- struct event 定义 ------------------------- */
struct event {
    TAILQ_ENTRY(event) ev_active_next;
    TAILQ_ENTRY(event) ev_next;

    union {
        TAILQ_ENTRY(event) ev_next_with_common_timeout;
        int                   min_heap_idx;
    } ev_timeout_pos;

    evutil_socket_t ev_fd;

    struct event_base *ev_base;

    union {
        struct {
            TAILQ_ENTRY(event) ev_io_next;
            struct timeval      ev_timeout;
        } ev_io;
        struct {
            TAILQ_ENTRY(event) ev_signal_next;
            short                ev_ncalls;
            short               *ev_pncalls;
        } ev_signal;
    } _ev;

    short        ev_events;
    short        ev_res;
    short        ev_flags;
    ev_uint8_t   ev_pri;
    ev_uint8_t   ev_closure;
    struct timeval ev_timeout;

    void (*ev_callback)(evutil_socket_t, short, void *);
    void *ev_arg;
};


/* --------------- event_add_internal 源码（带中文注释） --------------- */
static inline int event_add_internal(struct event *ev, const struct timeval *tv, int tv_is_absolute) {
    struct event_base *base = ev->ev_base;
    int res = 0;
    int notify = 0;

    EVENT_BASE_ASSERT_LOCKED(base);
    _event_debug_assert_is_setup(ev);

    event_debug({
        "event_add: event: %p (fd %d), %s%s%s call %p",
        ev,
        (int)ev->ev_fd,
        ev->ev_events & EV_READ    ? "EV_READ "    : "",
        ev->ev_events & EV_WRITE   ? "EV_WRITE "   : "",
        ev->ev_events & EV_TIMEOUT ? "EV_TIMEOUT " : "",
        ev->ev_callback
    });

    EVUTIL_ASSERT(!(ev->ev_flags & ~EVLIST_ALL));
    if (tv != NULL && !(ev->ev_flags & EVLIST_TIMEOUT)) {
        if (min_heap_reserve(base->timeheap,
                             1 + min_heap_size(base->timeheap)) == -1)
            return (-1);
    }

#ifndef _EVENT_DISABLE_THREAD_SUPPORT
    if (base->current_event == ev &&
        (ev->ev_events & EV_SIGNAL) &&
        !EVBASE_IN_THREAD(base)) {
        ++base->current_event_waiters;
        EVTHREAD_COND_WAIT(base->current_event_cond, base->th_base_lock);
    }
#endif

    if ((ev->ev_events & (EV_READ|EV_WRITE|EV_SIGNAL)) &&
        !(ev->ev_flags & (EVLIST_INSERTED|EVLIST_ACTIVE))) {
        if (ev->ev_events & (EV_READ|EV_WRITE)) {
            /* 添加 I/O 事件到 evmap 映射表 */
            res = evmap_io_add(base, ev->ev_fd, ev);
        } else if (ev->ev_events & EV_SIGNAL) {
            /* 添加 信号 事件到 evmap 信号映射表 */
            res = evmap_signal_add(base, (int)ev->ev_fd, ev);
        }
        if (res == -1)
            return (-1);
        /* 将事件标记为已插入（EVLIST_INSERTED）*/
        notify = 1;
        res = 0;
    }

    if (res != -1 && tv != NULL) {
        struct timeval now;
        int common_timeout;

        if (ev->ev_closure == EV_CLOSURE_PERSIST && !tv_is_absolute) {
            ev->ev_io_timeout = *tv;
        }

        if (ev->ev_flags & EVLIST_TIMEOUT) {
            if (min_heap_elt_is_top(ev))
                notify = 1;
            event_queue_remove(base, ev, EVLIST_TIMEOUT);
        }

        if ((ev->ev_flags & EVLIST_ACTIVE) && (ev->ev_res & EV_TIMEOUT)) {
            if (ev->ev_ncalls && ev->ev_pncalls)
                *ev->ev_pncalls = 0;
        }

        event_queue_remove(base, ev, EVLIST_ACTIVE);
        gettime(base, &now);
        common_timeout = is_common_timeout(tv, base);

        if (tv_is_absolute) {
            ev->ev_io_timeout = *tv;
        }
        if (common_timeout) {
            struct timeval tmp = *tv;
            tmp.tv_usec &= MICROSECONDS_MASK;
            evutil_timerclear(&now, &tmp, &ev->ev_io_timeout);
            ev->ev_timeout.tv_usec = (tv->tv_usec & ~MICROSECONDS_MASK);
        } else {
            evutil_timeradd(&now, tv, &ev->ev_timeout);
        }

        event_debug({
            "event add: timeout in %d seconds, call %p",
            (int)tv->tv_sec, ev->ev_callback
        });

        event_queue_insert(base, ev, EVLIST_TIMEOUT);

        if (common_timeout) {
            struct common_timeout_list *ctl =
                get_common_timeout_list(base, &ev->ev_timeout);
            if (ev == TAILQ_FIRST(&ctl->events))
                common_timeout_schedule(ctl, &now, ev);
        } else {
            if (min_heap_elt_is_top(ev))
                notify = 1;
        }
    }

    if (res != -1 && notify && EVBASE_NEED_NOTIFY(base))
        evthread_notify_base(base);

    _event_debug_note_add(ev);
    return (res);
}
