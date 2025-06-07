#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Simple error handling functions */

#define handle_error_en(en, msg) \
    do {                         \
        errno = en;              \
        perror(msg);             \
        exit(EXIT_FAILURE);      \
    } while (0)

static void *sig_thread(void *arg) {
    printf("yyyyy, thread id is: %ld\n", pthread_self());
    sigset_t aset;
    int s, sig;
    sigemptyset(&aset);
    sigaddset(&aset, SIGQUIT);
    sigaddset(&aset, SIGUSR1);
    sigset_t *set = (sigset_t *)arg;

    for (;;) {
        s = sigwait(set, &sig);
        if (s != 0) handle_error_en(s, "sigwait");
        printf("Signal handling thread got signal %d\n", sig);
    }
}

static void handler(int arg) {
    printf("xxxxx, thread id is: %ld\n", pthread_self());
}

int main(int argc, char *argv[]) {
    pthread_t thread;
    sigset_t set;
    int s;

    signal(SIGQUIT, handler);
    s = pthread_create(&thread, NULL, &sig_thread, (void *)&set);
    sigemptyset(&set);
    sigaddset(&set, SIGQUIT);
    sigaddset(&set, SIGUSR1);
    if (s != 0) handle_error_en(s, "pthread_create");
    printf("sub thread with id: %ld\n", thread);
    pause();
}
