#include <signal.h>
#include <stdlib.h>

#include "ga_core.h"

static struct signal signals[] = {{SIGUSR1, "SIGUSR1", 0, signal_handler},
                                  {SIGUSR2, "SIGUSR2", 0, signal_handler},
                                  {SIGHUP, "SIGHUP", 0, signal_handler},
                                  {SIGINT, "SIGINT", 0, signal_handler},
                                  {SIGSEGV, "SIGSEGV", SA_RESETHAND, signal_handler},
                                  {SIGPIPE, "SIGPIPE", 0, SIG_IGN},
                                  {0, NULL, 0, NULL}};

int signal_init(void) {
    struct signal *sig;

    for (sig = signals; sig->signo != 0; sig++) {
        int status;
        struct sigaction sa;

        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = sig->handler;
        sa.sa_flags = sig->flags;
        sigemptyset(&sa.sa_mask);

        status = sigaction(sig->signo, &sa, NULL);
        if (status < 0) {
            ga_log(ERROR, "sigaction(%s) failed: %s", sig->signame, strerror(errno));
            return GA_ERR;
        }
    }

    return GA_OK;
}

void signal_deinit(void) {}

void signal_handler(int signo) {
    struct signal *sig;
    void (*action)(void);
    char *actionstr;
    bool done;

    for (sig = signals; sig->signo != 0; sig++) {
        if (sig->signo == signo) {
            break;
        }
    }
    ASSERT(sig->signo != 0);

    actionstr = "";
    action = NULL;
    done = false;

    switch (signo) {
        case SIGUSR1:
            break;

        case SIGUSR2:
            break;

        case SIGINT:
            done = true;
            actionstr = ", exiting";
            break;

        case SIGSEGV:
            ga_stacktrace(1);
            actionstr = ", core dumping";
            raise(SIGSEGV);
            break;

        default:
            ASSERT(0);
    }

    ga_log(DEBUG, "signal %d (%s) received%s", signo, sig->signame, actionstr);

    if (action != NULL) {
        action();
    }

    if (done) {
        exit(1);
    }
}
