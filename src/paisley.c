/*
 * paisley
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <ev.h>

#include "buffer.h"
#include "user_list.h"
#include "const.h"

int set_nonblocking(int fd)
{
  int flags;

  if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
    flags = 0;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}  

void listen_loop(int port);
void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void client_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
int read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
int write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);

struct user_node *users_head;

int broadcast_msg(struct user_node *head, struct user_node *src, char *msg) {
  int count = 0;
  struct user_node *ptr;

  for (ptr = head; ptr != NULL; ptr = ptr->next) {
    if (ptr == src) continue;
    append_buffer(ptr->user->out, msg);
    count++;
  }

  return count;
}

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

  users_head = NULL;

  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Failed to open socket.");
  }

  set_nonblocking(sock);

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
  struct user_node *node = insert_user(&users_head, new_user());
  w_client->data = (void *) node;

  if (EV_ERROR & revents) {
    perror("Invalid event in accept.");
    return;
  }

  sock = accept(watcher->fd, (struct sockaddr *) &client_addr, &client_len);

  set_nonblocking(sock);

  if (sock < 0) {
    perror("Error accepting socket.");
    return;
  }
  
  ev_io_init(w_client, client_cb, sock, EV_READ | EV_WRITE);
  ev_io_start(loop, w_client);
}

void client_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  int res = 1;
  if (revents & EV_READ) {
    res = read_cb(loop, watcher, revents);
  }
  if (revents & EV_WRITE && res) {
    write_cb(loop, watcher, revents);
  }
}

int read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  struct user_node *node = (struct user_node *) watcher->data;
  struct user *user = node->user;

  if (EV_ERROR & revents) {
    perror("Invalid event in read.");
    return 0;
  }

  char buf[BUFFER_SIZE];
  bzero(buf, BUFFER_SIZE);
  int read = recv(watcher->fd, buf, BUFFER_SIZE, 0); 

  if (read < 0) {
    perror("Read error in read.");
    return 0;
  } else if (read == 0) {
    // Read socket is in charge of closing
    printf("Socket %d closed.\n", watcher->fd);
    ev_io_stop(loop, watcher);
    delete_node(&users_head, node);
    free(watcher);
    return 0;
  }

  append_buffer(user->in, buf);

  char *pos;
  char *msg;
  if ((pos = strchr(user->in->data, '\n')) != NULL) {
    int len = pos - user->in->data - 1;
    msg = (char *) malloc(sizeof(char) * len + 1);
    msg[len] = 0;
    bcopy(user->in->data, msg, len);
    bzero(user->in->data, user->in->len);

    if (!user->name) {
      if (strstr(msg, "USER ") == msg && strlen(msg) > 5) {
        int name_len = strlen(msg) - 5 + 1;
        user->name = (char *) malloc(name_len);
        user->name[name_len] = 0;
        bcopy(msg + 5, user->name, name_len);
        printf("User registered: %s\n", user->name);
      }
    } else {
      // broadcast
      int _len = strlen(msg) + strlen(user->name) + 2 + 3;
      char *_msg = (char *) malloc(sizeof(char) * _len);
      snprintf(_msg, _len, "%s: %s\r\n", user->name, msg);
      int count = broadcast_msg(users_head, node, _msg);
      printf("%s broadcast to %d users.\n", user->name, count);
    }

    free(msg);
  }

  return 1;
}

int write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  struct user_node *node = (struct user_node *) watcher->data;
  struct user *user = node->user;
  struct buffer *buffer = user->out;

  if (EV_ERROR & revents) {
    perror("Invalid event in read.");
    return 0;
  }

  if (buffer->data[0] == 0) {
    return 0;
  }

  int n = send(watcher->fd, buffer->data, strlen(buffer->data) + 1, 0);
  printf("Delivered message.\n");

  bzero(buffer->data, buffer->len); 
  return 1;
}

