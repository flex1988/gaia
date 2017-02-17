#ifndef _GA_SIGNAL_H_
#define _GA_SIGNAL_H_

struct signal {
    int  signo;
    char *signame;
    int  flags;
    void (*handler)(int signo);
};

int signal_init(void);
void signal_deinit(void);
void signal_handler(int signo);

#endif
