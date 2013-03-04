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
#include <glib.h>

#include "const.h"
#include "paisley.h"
#include "irc.h"

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

GHashTable *global_irc_users = NULL;

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

  global_irc_users = g_hash_table_new_full(NULL, g_str_equal, free, free_irc_user_object);

  // Open socket + set options
  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Failed to open socket.");
  }

  set_nonblocking(sock);

  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  // Bind + listen
  if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) != 0) {
    perror("Failed to bind to socket.");
  }

  if (listen(sock, 10) < 0) {
    perror("Failed to listen to socket.");
  }

  // Register accept handler
  ev_io_init(&w_accept, accept_cb, sock, EV_READ);
  ev_io_start(loop, &w_accept);

  // Begin event loop
  while (1) {
    ev_loop(loop, 0);
  }
}

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  int sock;

  // Allocate a new event watcher
  struct ev_io *w_client = (struct ev_io *) malloc(sizeof(struct ev_io));
  irc_user_object *obj = new_irc_user_object(NULL, NULL, NULL);
  w_client->data = (void *) obj;

  // Short circuit on error
  if (EV_ERROR & revents) {
    perror("Invalid event in accept.");
    return;
  }

  // Accept socket + set options
  sock = accept(watcher->fd, (struct sockaddr *) &client_addr, &client_len);

  set_nonblocking(sock);

  if (sock < 0) {
    perror("Error accepting socket.");
    return;
  }
  
  // Register read + write event handler
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
  irc_user_object *obj = (irc_user_object *) watcher->data;

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

    if (obj->user_data->nick)
      delete_irc_user_object(obj->user_data->nick);
    else
      free_irc_user_object(obj);

    free(watcher);
    return 0;
  }

  g_string_append_len(obj->in, buf, read);

  char *pos;
  GString *msg;
  if ((pos = strchr(obj->in->str, '\n')) != NULL) {
    int len = pos - obj->in->str - 1; // truncate \r
    msg = g_string_new_len(obj->in->str, len);
    g_string_erase(obj->in, 0, len + 2);

    if (!obj->user_data->nick) {
      if (strstr(msg->str, "USER ") == msg->str && msg->len > 5) {
        int nick_len = msg->len - 5 + 1;
        g_string_erase(msg, 0, 5);
        obj->user_data->nick = g_string_new(msg->str);
        printf("User registered: %s\n", obj->user_data->nick->str);
        add_irc_user_object(obj);
      }
    } else {
      // broadcast
      GString *_msg = g_string_new(NULL);
      g_string_append_printf(_msg, "%s: %s", obj->user_data->nick->str, msg->str);
      int count = irc_broadcast_msg(obj, _msg);
      printf("%s broadcast to %d users.\n", obj->user_data->nick->str, count);

      g_string_free(_msg, TRUE);
    }

    g_string_free(msg, TRUE);
  }

  return 1;
}

int write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  irc_user_object *obj = (irc_user_object *) watcher->data;
  GString *buffer = obj->out;

  if (EV_ERROR & revents) {
    perror("Invalid event in read.");
    return 0;
  }

  if (!buffer->len) {
    return 0;
  }

  int n = send(watcher->fd, buffer->str, buffer->len + 1, 0);
  printf("Delivered message.\n");

  g_string_erase(buffer, 0, buffer->len);
  return 1;
}

