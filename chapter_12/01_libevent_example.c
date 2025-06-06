#include <event.h>
#include <stdio.h>
#include <sys/signal.h>
#include <sys/time.h>

static int timeout_count = 0;
static struct event *global_timeout_event = NULL;
static struct event_base *global_base = NULL;

void signal_cb(int fd, short event, void *argc) {
    struct event_base *base = (struct event_base *)argc;
    struct timeval delay = {2, 0};
    printf("Caught an interrupt signal; exiting cleanly in two seconds...\n");
    event_base_loopexit(base, &delay);
}

void timeout_cb(int fd, short event, void *argc) {
    timeout_count++;
    printf("timeout %d\n", timeout_count);

    // 5次timeout后自动退出
    if (timeout_count >= 5) {
        printf("Reached 5 timeouts, exiting...\n");
        event_base_loopbreak(global_base);
        return;
    }

    // 重新添加定时器，让它继续运行
    struct timeval tv = {1, 0};
    event_add(global_timeout_event, &tv);
}

int main() {
    global_base = event_init();

    struct event *signal_event =
        evsignal_new(global_base, SIGINT, signal_cb, global_base);
    event_add(signal_event, NULL);

    struct timeval tv = {1, 0};
    global_timeout_event = evtimer_new(global_base, timeout_cb, NULL);
    event_add(global_timeout_event, &tv);

    printf(
        "Starting event loop... Press Ctrl+C to exit or wait for 5 timeouts\n");
    event_base_dispatch(global_base);

    event_free(global_timeout_event);
    event_free(signal_event);
    event_base_free(global_base);

    printf("Program finished.\n");
    return 0;
}
