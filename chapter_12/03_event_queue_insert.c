/* --------------------- event_queue_insert 源码 --------------------- */
static void event_queue_insert(struct event_base *base, struct event *ev, int queue) {
    EVENT_BASE_ASSERT_LOCKED(base);
    /* 避免重复插入 */
    if (ev->ev_flags & queue) {
        /* Double insertion is possible for active events */
        if (queue & EVLIST_ACTIVE)
            return;
        event_errx(1, "%s: %p(fd %d) already on queue %x",
                   __func__, ev, ev->ev_fd, queue);
        return;
    }

    if (!(ev->ev_flags & EVLIST_INTERNAL))
        base->event_count++;  /* 将 event_base 拥有的事件处理器总数加 1 */

    ev->ev_flags |= queue;    /* 标记此事件已被添加过 */

    switch (queue) {
        /* 将 I/O 事件处理器或信号事件处理器插入注册事件队列 */
        case EVLIST_INSERTED:
            TAILQ_INSERT_TAIL(&base->eventqueue, ev, ev_next);
            break;

        /* 将就绪事件插入活动事件队列 */
        case EVLIST_ACTIVE:
            base->event_count_active++;
            TAILQ_INSERT_TAIL(&base->activequeues[ev->ev_pri],
                              ev, ev_active_next);
            break;

        /* 将定时器插入通用定时器队列或时间堆 */
        case EVLIST_TIMEOUT:
            if (is_common_timeout(&ev->ev_timeout, base)) {
                struct common_timeout_list *ctl =
                    get_common_timeout_list(base, &ev->ev_timeout);
                insert_common_timeout_inorder(ctl, ev);
            } else {
                min_heap_push(base->timeheap, ev);
            }
            break;

        default:
            event_errx(1, "%s: unknown queue %x", __func__, queue);
    }
}
