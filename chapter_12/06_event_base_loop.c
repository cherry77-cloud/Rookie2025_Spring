int event_base_loop(struct event_base *base, int flags) {
    const struct eventop *evsel = base->evsel;
    struct timeval tv;
    struct timeval *tv_p;
    int res, done, retval = 0;

    EVBASE_ACQUIRE_LOCK(base, th_base_lock);
    if (base->running_loop) {
        event_warnx(
            "%s: reentrant invocation. Only one event_base_loop "
            "can run on each event_base at once.",
            __func__);
        EVBASE_RELEASE_LOCK(base, th_base_lock);
        return -1;
    }
    base->running_loop = 1;
    clear_time_cache(base);

    if (base->sig.ev_signal_added && base->sig.ev_n_signals_added)
        evsig_set_base(base);

    done = 0;
#ifndef _EVENT_DISABLE_THREAD_SUPPORT
    base->th_owner_id = EVTHREAD_GET_ID();
#endif

    base->event_gotterm = base->event_break = 0;

    while (!done) {
        base->event_continue = 0;

        if (base->event_gotterm)
            break;
        if (base->event_break)
            break;

        timeout_correct(base, &tv);
        tv_p = &tv;

        if (!N_ACTIVE_CALLBACKS(base) &&
            !(flags & EVLOOP_NONBLOCK)) {
            timeout_next(base, tv_p);
        } else {
            evutil_timerclear(&tv);
        }

        if (!event_haveevents(base) && !N_ACTIVE_CALLBACKS(base)) {
            event_debug(("%s: no events registered.", __func__));
            retval = 1;
            goto done;
        }

        gettime(base, &base->event_tv);
        clear_time_cache(base);

        res = evsel->dispatch(base, tv_p);
        if (res == -1) {
            event_debug(("%s: dispatch returned unsuccessfully.", __func__));
            retval = -1;
            goto done;
        }

        update_time_cache(base);
        timeout_process(base);

        if (N_ACTIVE_CALLBACKS(base)) {
            int n = event_process_active(base);
            if ((flags & EVLOOP_ONCE) &&
                N_ACTIVE_CALLBACKS(base) == 0 &&
                n != 0)
                done = 1;
        } else if (flags & EVLOOP_NONBLOCK) {
            done = 1;
        }
    }

    event_debug(("%s: asked to terminate loop.", __func__));

done:
    clear_time_cache(base);
    base->running_loop = 0;
    EVBASE_RELEASE_LOCK(base, th_base_lock);
    return (retval);
}
