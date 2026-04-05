#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "tournoi.h"

static Tournoi tournoi;
static char error_msg[128];

static void set_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(error_msg, sizeof(error_msg), fmt, ap);
    va_end(ap);
}

const char *tournoi_last_error(void) {
    return error_msg;
}

static void notify_client(int client_sock, const char *payload) {
    if (client_sock >= 0) {
        printf("[tournoi] client %d: %s\n", client_sock, payload);
    } else {
        printf("[tournoi] %s\n", payload);
    }
}
