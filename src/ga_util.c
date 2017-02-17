#include <execinfo.h>
#include "ga_core.h"

void ga_assert(const char *cond, const char *file, int line, int panic) {
    ga_log(ERROR, "assert %s failed @(%s,%d)", cond, file, line);
    if (panic) {
        ga_stacktrace(1);
        abort();
    }
}

void ga_stacktrace(int skip_count) {
    void *stack[64];
    char **symbols;
    int size, i, j;

    size = backtrace(stack, 64);
    symbols = backtrace_symbols(stack, size);

    if (symbols == NULL) {
        return;
    }

    skip_count++;

    for (i = skip_count, j = 0; i < size; i++, j++) {
        ga_log(WARN, "[%d] %s", j, symbols[i]);
    }
    ga_free(symbols);
}
