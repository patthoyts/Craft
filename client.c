#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include "client.h"
#include "tinycthread.h"

#define QUEUE_SIZE 65536
#define BUFFER_SIZE 4096
#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif

static int client_enabled = 0;
static int sd = 0;
static char recv_buffer[QUEUE_SIZE] = {0};
static thrd_t recv_thread;
static mtx_t mutex;

void client_enable() {
    client_enabled = 1;
}

void client_disable() {
    client_enabled = 0;
}

int get_client_enabled() {
    return client_enabled;
}

int client_sendall(int sd, char *data, int length) {
    if (!client_enabled) {
        return 0;
    }
    int count = 0;
    while (count < length) {
        int n = send(sd, data + count, length, 0);
        if (n == -1) {
            return -1;
        }
        count += n;
        length -= n;
    }
    return 0;
}

void client_send(char *data) {
    if (!client_enabled) {
        return;
    }
    if (client_sendall(sd, data, strlen(data)) == -1) {
        perror("client_sendall");
        exit(1);
    }
}

void client_position(float x, float y, float z, float rx, float ry) {
    if (!client_enabled) {
        return;
    }
    static float px, py, pz, prx, pry = 0;
    float distance =
        (px - x) * (px - x) +
        (py - y) * (py - y) +
        (pz - z) * (pz - z) +
        (prx - rx) * (prx - rx) +
        (pry - ry) * (pry - ry);
    if (distance < 0.1) {
        return;
    }
    px = x; py = y; pz = z; prx = rx; pry = ry;
    char buffer[1024];
    snprintf(buffer, 1024, "P,%.2f,%.2f,%.2f,%.2f,%.2f\n", x, y, z, rx, ry);
    client_send(buffer);
}

void client_chunk(int p, int q) {
    if (!client_enabled) {
        return;
    }
    char buffer[1024];
    snprintf(buffer, 1024, "C,%d,%d\n", p, q);
    client_send(buffer);
}

void client_block(int p, int q, int x, int y, int z, int w) {
    if (!client_enabled) {
        return;
    }
    char buffer[1024];
    snprintf(buffer, 1024, "B,%d,%d,%d,%d,%d,%d\n", p, q, x, y, z, w);
    client_send(buffer);
}

int client_recv(char *data, int length) {
    if (!client_enabled) {
        return 0;
    }
    int result = 0;
    mtx_lock(&mutex);
    char *p = strstr(recv_buffer, "\n");
    if (p) {
        *p = '\0';
        strncpy(data, recv_buffer, length);
        data[length - 1] = '\0';
        memmove(recv_buffer, p + 1, strlen(p + 1) + 1);
        result = 1;
    }
    mtx_unlock(&mutex);
    return result;
}

int recv_worker(void *arg) {
    while (1) {
        char data[BUFFER_SIZE] = {0};
        if (recv(sd, data, BUFFER_SIZE - 1, 0) == -1) {
            perror("recv");
            exit(1);
        }
        while (1) {
            int done = 0;
            mtx_lock(&mutex);
            if (strlen(recv_buffer) + strlen(data) < QUEUE_SIZE) {
                strcat(recv_buffer, data);
                done = 1;
            }
            mtx_unlock(&mutex);
            if (done) {
                break;
            }
            sleep(0);
        }
    }
    return 0;
}

void client_connect(char *hostname, int port) {
    if (!client_enabled) {
        return;
    }

    char serv[24];
    struct addrinfo hints, *res = NULL, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_socktype = SOCK_STREAM;
    sprintf(serv, "%d", port);
    if (getaddrinfo(hostname, serv, &hints, &res)) {
        perror("getaddrinfo");
        exit(1);
    }

    sd = -1;
    for (p = res; p != NULL; p = p->ai_next) {
        if ((sd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) != -1) {
            if (connect(sd, (const struct sockaddr *)p->ai_addr, p->ai_addrlen) != -1) {
                char name[NI_MAXHOST];
                getnameinfo(p->ai_addr, p->ai_addrlen, name, NI_MAXHOST,
                            NULL, 0, NI_NUMERICHOST | NI_NUMERICSERV);
                printf("connected to %s:%d\n", name, port);
                break;
            }
        }
    }
    freeaddrinfo(res);

    if (sd == -1) {
        perror("socket");
        exit(1);
    }
}

void client_start() {
    if (!client_enabled) {
        return;
    }
    mtx_init(&mutex, mtx_plain);
    if (thrd_create(&recv_thread, recv_worker, NULL) != thrd_success) {
        perror("thrd_create");
        exit(1);
    }
}

void client_stop() {
    if (!client_enabled) {
        return;
    }
    close(sd);
    if (thrd_join(recv_thread, NULL) != thrd_success) {
        perror("thrd_join");
        exit(1);
    }
    mtx_destroy(&mutex);
}
