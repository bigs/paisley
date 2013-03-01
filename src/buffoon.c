/*
 * Buffoon: A horn blaring server.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <netinet/in.h>
#include <ev.h>

#define BUFFER_SIZE 1024

void listen_loop(int port);
void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void client_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);

struct client_buffers {
  char in[BUFFER_SIZE];
  char out[BUFFER_SIZE];
};

int main(int argc, char **argv) {
  char *port = NULL;
  int c;
  opterr = 0;

  // Parse args
  while ((c = getopt(argc, argv, "p:")) != -1) {
    switch (c) {
      case 'p':
        port = optarg;
        break;
      case '?':
        if (optopt == 'p') {
          fprintf(stderr, "Option -%c requires an argument.\n", optopt);
        }
        return 1;
      default:
        abort();
    }
  }

  // Default port
  if (port == NULL) {
    port = "3000";
  }

  // Cast port
  int _port;
  if (!(_port = atoi(port))) {
    fprintf(stderr, "Option -p must be an integer.\n");
    return -1;
  }

  // Listen
  listen_loop(_port);

  return 0;
}

void listen_loop(int port) {
  struct ev_loop *loop = EV_DEFAULT;
  int sock;
  struct sockaddr_in addr;
  int addr_len = sizeof(addr);
  struct ev_io w_accept;

  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Failed to open socket.");
  }

  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) != 0) {
    perror("Failed to bind to socket.");
  }

  if (listen(sock, 10) < 0) {
    perror("Failed to listen to socket.");
  }

  ev_io_init(&w_accept, accept_cb, sock, EV_READ);
  ev_io_start(loop, &w_accept);

  while (1) {
    ev_loop(loop, 0);
  }
}

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  int sock;

  struct ev_io *w_client = (struct ev_io *) malloc(sizeof(struct ev_io));
  w_client->data = (void *) malloc(sizeof(struct client_buffers));
  bzero(w_client->data, sizeof(struct client_buffers));

  if (EV_ERROR & revents) {
    perror("Invalid event in accept.");
    return;
  }

  sock = accept(watcher->fd, (struct sockaddr *) &client_addr, &client_len);

  if (sock < 0) {
    perror("Error accepting socket.");
    return;
  }
  
  ev_io_init(w_client, client_cb, sock, EV_READ | EV_WRITE);
  ev_io_start(loop, w_client);
}

void client_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  // Since read/write are sharing the same data segment, I figured
  // I could do my own dispatch here.
  if (revents & EV_WRITE) {
    write_cb(loop, watcher, revents);
  }
  if (revents & EV_READ) {
    read_cb(loop, watcher, revents);
  }
}

void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  struct client_buffers *buffers = (struct client_buffers *) watcher->data;
  bzero(buffers->in, BUFFER_SIZE);

  if (EV_ERROR & revents) {
    perror("Invalid event in read.");
    return;
  }

  int read = recv(watcher->fd, buffers->in, BUFFER_SIZE, 0);

  if (read < 0) {
    perror("Read error in read.");
    return;
  } else if (read == 0) {
    // Read socket is in charge of closing
    printf("Socket %d closed.\n", watcher->fd);
    ev_io_stop(loop, watcher);
    free(watcher->data);
    free(watcher);
    return;
  }

  printf("Message from client %d: %s", watcher->fd, (char *) buffers->in);
  bcopy(buffers->in, buffers->out, BUFFER_SIZE);
}

void write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  struct client_buffers *buffers = (struct client_buffers *) watcher->data;

  if (EV_ERROR & revents) {
    perror("Invalid event in read.");
    return;
  }

  if (buffers->out[0] == 0) {
    return;
  }

  printf("Writing response to client %d.\n", watcher->fd);

  int err = send(watcher->fd, buffers->out, strlen(buffers->out) + 1, 0);

  if (err == -1) {
    bzero(buffers->out, BUFFER_SIZE);
    return;
  }

  bzero(buffers->out, BUFFER_SIZE);
}

