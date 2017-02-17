#include "ga_core.h"

static struct logger logger;

int scnprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list args;
    int n;

    va_start(args, fmt);
    n = vsnprintf(buf, size, fmt, args);
    va_end(args);

    return n;
}

void ga_log_stderr(const char *fmt, ...) {
    int len, size;

    char buf[GA_LOG_MAX_LEN];
    va_list args;

    len = 0;
    size = GA_LOG_MAX_LEN;

    va_start(args, fmt);
    len += vsnprintf(buf, size, fmt, args);
    va_end(args);

    buf[len++] = '\n';

    write(STDERR_FILENO, buf, len);
}

int ga_log_init(int level, char *name) {
    struct logger *l = &logger;

    l->level = level;
    l->name = name;

    if (name == NULL || !strlen(name)) {
        l->fd = STDERR_FILENO;
    } else {
        l->fd = open(name, O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (l->fd < 0) {
            return -1;
        }
    }
    return 0;
}

void ga_log_core(char *file, int line, int panic, int32_t level, const char *fmt, ...) {
    struct logger *l = &logger;
    int len, size;
    char buf[GA_LOG_MAX_LEN];
    va_list args;
    ssize_t n;
    struct timeval tv;

    GA_NOT_USED(n);

    if (l->level < level || l->fd < 0)
        return;

    len = 0;
    size = GA_LOG_MAX_LEN;

    gettimeofday(&tv, NULL);
    buf[len++] = '[';
    len += strftime(buf + len, size - len, "%Y-%m-%d %H:%M:%S", localtime(&tv.tv_sec));
    /*len += scnprintf(buf + len, size - len, "%03ld", tv.tv_usec / 1000);*/
    len += scnprintf(buf + len, size - len, "] %s:%d ", file, line);

    va_start(args, fmt);
    len += vsnprintf(buf + len, size - len, fmt, args);
    va_end(args);

    buf[len++] = '\n';

    n = write(l->fd, buf, len);
    if (panic)
        abort();
}
