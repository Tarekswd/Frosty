/**
 * @file server.h
 * @brief Minimal HTTP/1.1 server for the chatbot REST API.
 *
 * Listens on a configurable TCP port and exposes one endpoint:
 *
 *   POST /chat
 *   Content-Type: application/json
 *   Body: {"message": "user input here"}
 *
 *   Response 200 OK
 *   Body: {"response": "bot reply", "sentiment": "positive", "score": 3}
 *
 * The server runs in a separate thread.  Each connection is handled by
 * a worker from the built-in thread pool.
 *
 * NOTE: Windows builds use Winsock2; POSIX builds use BSD sockets.
 *       No external HTTP library is required.
 */
#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>

/** Opaque HTTP server handle. */
typedef struct ChatServer ChatServer;

/**
 * @brief Create and start the HTTP server.
 *
 * Binds to 0.0.0.0:<port>, starts the thread pool, and begins
 * accepting connections.
 *
 * @param port         TCP port to listen on (e.g. 8080).
 * @param num_threads  Worker thread count (>= 1, typically 4).
 * @return Newly allocated ChatServer, or NULL on failure.
 */
ChatServer *server_start(int port, int num_threads);

/**
 * @brief Signal the server to stop and wait for all workers to exit.
 * @param srv The server to stop.
 */
void server_stop(ChatServer *srv);

/**
 * @brief Free server resources (call after server_stop).
 * @param srv The server to destroy.
 */
void server_destroy(ChatServer *srv);

/**
 * @brief Check whether the server is currently running.
 * @param srv The server.
 * @return true if the server loop is active.
 */
bool server_is_running(const ChatServer *srv);

#endif /* SERVER_H */
