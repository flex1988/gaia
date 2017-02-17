#ifndef _GAIA_LOG_H_
#define _GAIA_LOG_H_

#define STDERR 0
#define EMERG 1
#define CRITICAL 2
#define ERROR 3
#define WARN 4
#define NOTICE 5
#define INFO 6
#define DEBUG 7

#define GA_LOG_MAX_LEN 400

struct logger {
    int level;
    int fd;
    char *name;
};

#define ga_log(level, ...)                                                        \
    do {                                                                          \
        ga_log_core(__FILE__, __LINE__, (level <= CRITICAL), level, __VA_ARGS__); \
    } while (0)

int ga_log_init(int level, char *filename);
void ga_log_core(char *file, int line, int panic, int32_t level, const char *fmt, ...);
void ga_log_stderr(const char *fmt, ...);
#endif
