/**
 * @file server.c
 * @brief Cross-platform minimal HTTP/1.1 server with a thread pool.
 *
 * Supports Windows (Winsock2) and POSIX (BSD sockets).
 * POST /chat: reads JSON body, extracts "message" field, calls the
 * chatbot pipeline, returns JSON response.
 */
#include "server.h"
#include "utils.h"
#include "model.h"
#include "memory.h"
#include "sentiment.h"
#include "antigravity.h"
#include "plugins.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <windows.h>
   typedef SOCKET sock_t;
#  define SOCK_INVALID INVALID_SOCKET
#  define sock_close(s) closesocket(s)
   typedef HANDLE thread_t;
   /* simple thread creation wrapper */
   typedef DWORD (WINAPI *win_thread_fn)(LPVOID);
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <pthread.h>
#  include <unistd.h>
   typedef int  sock_t;
#  define SOCK_INVALID (-1)
#  define sock_close(s) close(s)
   typedef pthread_t thread_t;
#endif

#define RECV_BUF  8192
#define SEND_BUF  8192
#define BACKLOG   16
#define MAX_WORKERS 16

/* ──────────────────────────────── Thread pool ─── */

typedef struct WorkItem {
    sock_t client;
    struct WorkItem *next;
} WorkItem;

typedef struct {
    WorkItem  *head;
    WorkItem  *tail;
#ifdef _WIN32
    CRITICAL_SECTION  mutex;
    HANDLE            sem;
#else
    pthread_mutex_t   mutex;
    pthread_cond_t    cond;
#endif
    atomic_bool  shutdown;
} WorkQueue;

struct ChatServer {
    sock_t      listen_sock;
    WorkQueue   queue;
    thread_t    workers[MAX_WORKERS];
    int         num_workers;
    atomic_bool running;
    /* shared NLP objects (read-only during serving) */
    NLPModel       *model;
    ConvMemory     *mem;
    PluginRegistry *plugins;
};

/* ──────────────────────────────── JSON helpers ─── */

/** Extract the value of a simple string field from JSON. */
static bool json_get_string(const char *json, const char *key,
                             char *out, size_t out_sz)
{
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *pos = strstr(json, search);
    if (!pos) return false;
    pos += strlen(search);
    while (*pos && (*pos == ' ' || *pos == ':')) ++pos;
    if (*pos != '"') return false;
    ++pos; /* skip opening quote */
    size_t i = 0;
    while (*pos && *pos != '"' && i < out_sz - 1) {
        if (*pos == '\\') { ++pos; } /* skip escape */
        out[i++] = *pos++;
    }
    out[i] = '\0';
    return true;
}

static void build_json_response(const char *response,
                                 const char *sentiment,
                                 int score,
                                 char *out, size_t out_sz)
{
    snprintf(out, out_sz,
        "{\"response\":\"%s\",\"sentiment\":\"%s\",\"score\":%d}",
        response, sentiment, score);
}

/* ──────────────────────────────── HTTP processing ─── */

static void process_client(ChatServer *srv, sock_t client) {
    char recv_buf[RECV_BUF] = {0};
    char send_buf[SEND_BUF] = {0};

    /* Receive HTTP request */
    int bytes = (int)recv(client, recv_buf, sizeof(recv_buf) - 1, 0);
    if (bytes <= 0) { sock_close(client); return; }
    recv_buf[bytes] = '\0';

    /* Only handle POST /chat */
    bool is_post = (strncmp(recv_buf, "POST /chat", 10) == 0);
    bool is_options = (strncmp(recv_buf, "OPTIONS", 7) == 0);

    if (is_options) {
        const char *cors =
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Length: 0\r\n\r\n";
        send(client, cors, (int)strlen(cors), 0);
        sock_close(client);
        return;
    }

    if (!is_post) {
        const char *not_found =
            "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
        send(client, not_found, (int)strlen(not_found), 0);
        sock_close(client);
        return;
    }

    /* Find JSON body (after \r\n\r\n) */
    const char *body = strstr(recv_buf, "\r\n\r\n");
    if (!body) { sock_close(client); return; }
    body += 4;

    char message[1024] = {0};
    if (!json_get_string(body, "message", message, sizeof(message)) ||
        message[0] == '\0') {
        const char *bad =
            "HTTP/1.1 400 Bad Request\r\nContent-Length: 19\r\n\r\n"
            "{\"error\":\"no input\"}";
        send(client, bad, (int)strlen(bad), 0);
        sock_close(client);
        return;
    }

    /* ── Chatbot pipeline ── */
    str_trim(message);
    char msg_lower[1024];
    str_safe_copy(msg_lower, message, sizeof(msg_lower));
    str_lower(msg_lower);

    char response[1024] = {0};
    int  score = sentiment_score(msg_lower);

    bool handled = plugin_dispatch(srv->plugins, msg_lower, response, sizeof(response));
    if (!handled) {
        if (ag_detect(msg_lower)) {
            ag_respond(score, response, sizeof(response));
        } else {
            model_respond(srv->model, msg_lower, score, srv->mem, response, sizeof(response));
        }
    }

    mem_push(srv->mem, "user", message);
    mem_push(srv->mem, "bot",  response);

    /* ── Build HTTP response ── */
    char json_body[1200];
    build_json_response(response, sentiment_label(score), score,
                        json_body, sizeof(json_body));

    snprintf(send_buf, sizeof(send_buf),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        (int)strlen(json_body), json_body);

    send(client, send_buf, (int)strlen(send_buf), 0);
    sock_close(client);
}

/* ──────────────────────────────── Worker threads ─── */

#ifdef _WIN32
static DWORD WINAPI worker_fn(LPVOID arg) {
#else
static void *worker_fn(void *arg) {
#endif
    ChatServer *srv = (ChatServer *)arg;
    while (!atomic_load(&srv->queue.shutdown)) {
#ifdef _WIN32
        WaitForSingleObject(srv->queue.sem, INFINITE);
        if (atomic_load(&srv->queue.shutdown)) break;
        EnterCriticalSection(&srv->queue.mutex);
        WorkItem *item = srv->queue.head;
        if (item) {
            srv->queue.head = item->next;
            if (!srv->queue.head) srv->queue.tail = NULL;
        }
        LeaveCriticalSection(&srv->queue.mutex);
#else
        pthread_mutex_lock(&srv->queue.mutex);
        while (!srv->queue.head && !atomic_load(&srv->queue.shutdown))
            pthread_cond_wait(&srv->queue.cond, &srv->queue.mutex);
        WorkItem *item = srv->queue.head;
        if (item) {
            srv->queue.head = item->next;
            if (!srv->queue.head) srv->queue.tail = NULL;
        }
        pthread_mutex_unlock(&srv->queue.mutex);
#endif
        if (item) {
            process_client(srv, item->client);
            free(item);
        }
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static void enqueue(ChatServer *srv, sock_t client) {
    WorkItem *item = malloc(sizeof(WorkItem));
    if (!item) { sock_close(client); return; }
    item->client = client;
    item->next   = NULL;
#ifdef _WIN32
    EnterCriticalSection(&srv->queue.mutex);
    if (srv->queue.tail) srv->queue.tail->next = item;
    else                 srv->queue.head = item;
    srv->queue.tail = item;
    LeaveCriticalSection(&srv->queue.mutex);
    ReleaseSemaphore(srv->queue.sem, 1, NULL);
#else
    pthread_mutex_lock(&srv->queue.mutex);
    if (srv->queue.tail) srv->queue.tail->next = item;
    else                 srv->queue.head = item;
    srv->queue.tail = item;
    pthread_cond_signal(&srv->queue.cond);
    pthread_mutex_unlock(&srv->queue.mutex);
#endif
}

/* ──────────────────────────────── Accept loop ─── */

#ifdef _WIN32
static DWORD WINAPI accept_loop(LPVOID arg) {
#else
static void *accept_loop(void *arg) {
#endif
    ChatServer *srv = (ChatServer *)arg;
    while (atomic_load(&srv->running)) {
        sock_t client = accept(srv->listen_sock, NULL, NULL);
        if (client == SOCK_INVALID) break;
        enqueue(srv, client);
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

/* ──────────────────────────────── Public API ─── */

ChatServer *server_start(int port, int num_threads) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        log_error("server_start: WSAStartup failed.");
        return NULL;
    }
#endif

    ChatServer *srv = calloc(1, sizeof(ChatServer));
    if (!srv) return NULL;

    atomic_store(&srv->running, true);
    atomic_store(&srv->queue.shutdown, false);

    /* Shared NLP objects */
    srv->model   = model_load("data/responses.txt");
    srv->mem     = mem_create(20, "data/history.bin");
    srv->plugins = plugin_create();

    /* Create listen socket */
    srv->listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (srv->listen_sock == SOCK_INVALID) {
        log_error("server_start: socket() failed.");
        server_destroy(srv);
        return NULL;
    }

    int opt = 1;
    setsockopt(srv->listen_sock, SOL_SOCKET, SO_REUSEADDR,
               (const char *)&opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((unsigned short)port);

    if (bind(srv->listen_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
        listen(srv->listen_sock, BACKLOG) < 0) {
        log_error("server_start: bind/listen failed on port %d.", port);
        server_destroy(srv);
        return NULL;
    }

    /* Init thread-pool queue */
#ifdef _WIN32
    InitializeCriticalSection(&srv->queue.mutex);
    srv->queue.sem = CreateSemaphore(NULL, 0, 1024, NULL);
#else
    pthread_mutex_init(&srv->queue.mutex, NULL);
    pthread_cond_init(&srv->queue.cond, NULL);
#endif

    int nw = (num_threads < 1) ? 1 :
             (num_threads > MAX_WORKERS) ? MAX_WORKERS : num_threads;
    srv->num_workers = nw;
    for (int i = 0; i < nw; ++i) {
#ifdef _WIN32
        srv->workers[i] = CreateThread(NULL, 0, worker_fn, srv, 0, NULL);
#else
        pthread_create(&srv->workers[i], NULL, worker_fn, srv);
#endif
    }

    /* Accept loop in its own thread */
    thread_t accept_thread;
#ifdef _WIN32
    accept_thread = CreateThread(NULL, 0, accept_loop, srv, 0, NULL);
    (void)accept_thread;
#else
    pthread_create(&accept_thread, NULL, accept_loop, srv);
    pthread_detach(accept_thread);
#endif

    log_info("HTTP server started on port %d with %d workers.", port, nw);
    return srv;
}

void server_stop(ChatServer *srv) {
    if (!srv) return;
    atomic_store(&srv->running, false);
    atomic_store(&srv->queue.shutdown, true);
    sock_close(srv->listen_sock);
    srv->listen_sock = SOCK_INVALID;

#ifdef _WIN32
    for (int i = 0; i < srv->num_workers; ++i)
        ReleaseSemaphore(srv->queue.sem, 1, NULL);
    for (int i = 0; i < srv->num_workers; ++i)
        WaitForSingleObject(srv->workers[i], 2000);
#else
    pthread_mutex_lock(&srv->queue.mutex);
    pthread_cond_broadcast(&srv->queue.cond);
    pthread_mutex_unlock(&srv->queue.mutex);
    for (int i = 0; i < srv->num_workers; ++i)
        pthread_join(srv->workers[i], NULL);
#endif
    log_info("HTTP server stopped.");
}

void server_destroy(ChatServer *srv) {
    if (!srv) return;
    if (atomic_load(&srv->running)) server_stop(srv);
    model_destroy(srv->model);
    mem_save(srv->mem);
    mem_destroy(srv->mem);
    plugin_destroy(srv->plugins);
#ifdef _WIN32
    if (srv->listen_sock != SOCK_INVALID) closesocket(srv->listen_sock);
    DeleteCriticalSection(&srv->queue.mutex);
    CloseHandle(srv->queue.sem);
    WSACleanup();
#else
    pthread_mutex_destroy(&srv->queue.mutex);
    pthread_cond_destroy(&srv->queue.cond);
#endif
    /* Drain any leftover queue items */
    WorkItem *it = srv->queue.head;
    while (it) { WorkItem *nx = it->next; free(it); it = nx; }
    free(srv);
}

bool server_is_running(const ChatServer *srv) {
    return srv && atomic_load(&srv->running);
}
