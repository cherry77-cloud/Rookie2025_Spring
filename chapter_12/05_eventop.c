/* struct eventop 结构体定义 */
struct eventop {
    const char *name;
    void (*init)(struct event_base *);
    int (*add)(struct event_base *, evutil_socket_t, short, void *fdinfo);
    int (*del)(struct event_base *, evutil_socket_t, short, void *fdinfo);
    int (*dispatch)(struct event_base *, struct timeval *);
    void (*dealloc)(struct event_base *);
    int need_reinit;
    enum event_method_feature features;
    size_t fdinfo_len;
};

#ifdef _EVENT_HAVE_EVENT_PORTS
extern const struct eventop evportops;
#endif
#ifdef _EVENT_HAVE_SELECT
extern const struct eventop selectops;
#endif
#ifdef _EVENT_HAVE_POLL
extern const struct eventop pollops;
#endif
#ifdef _EVENT_HAVE_EPOLL
extern const struct eventop epollops;
#endif
#ifdef _EVENT_HAVE_WORKING_KQUEUE
extern const struct eventop kqops;
#endif
#ifdef _EVENT_HAVE_DEVPOLL
extern const struct eventop devpollops;
#endif
#ifdef WIN32
extern const struct eventop win32ops;
#endif

static const struct eventop *eventops[] = {
#ifdef _EVENT_HAVE_EVENT_PORTS
    &evportops,
#endif
#ifdef _EVENT_HAVE_WORKING_KQUEUE
    &kqops,
#endif
#ifdef _EVENT_HAVE_EPOLL
    &epollops,
#endif
#ifdef _EVENT_HAVE_DEVPOLL
    &devpollops,
#endif
#ifdef _EVENT_HAVE_POLL
    &pollops,
#endif
#ifdef _EVENT_HAVE_SELECT
    &selectops,
#endif
#ifdef WIN32
    &win32ops,
#endif
    NULL
};


/* struct event_base 结构体定义 */
struct event_base {
    const struct eventop *evsel;
    void *evbase;
    struct event_changelist *changelist;
    const struct eventop *evsigsel;
    struct evsig_info sig;
    int virtual_event_count;
    int event_count;
    int event_count_active;
    int event_gotterm;
    int event_break;
    int event_continue;
    int event_running_priority;
    int running_loop;
    struct event_list *activequeues;
    int nactivequeues;
    struct common_timeout_list **common_timeout_queues;
    int n_common_timeouts;
    int n_common_timeouts_allocated;
    struct deferred_cb_queue defer_queue;
    struct event_io_map io;
    struct event_signal_map sigmap;
    struct event_list eventqueue;
    struct min_heap timeheap;
    struct timeval event_tv;
    struct timeval tv_cache;
#if defined(_EVENT_HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
    struct timeval tv_clock_diff;
    time_t last_updated_clock_diff;
#endif
#ifndef _EVENT_DISABLE_THREAD_SUPPORT
    unsigned long th_owner_id;
    void *th_base_lock;
    struct event *current_event;
    void *current_event_cond;
    int current_event_waiters;
#endif
#ifdef WIN32
    struct event_iocp_port *iocp;
#endif
    enum event_base_config_flag flags;
    int is_notify_pending;
    evutil_socket_t th_notify_fd[2];
    struct event th_notify;
    int (*th_notify_fn)(struct event_base *);
};
