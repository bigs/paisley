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
void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
void write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);

struct buffer {
  char *data;
  int len;
};

struct buffer * new_buffer() {
  struct buffer *buffer = (struct buffer *) malloc(sizeof(struct buffer));
  buffer->data = (char *) malloc(sizeof(char) * BUFFER_SIZE);
  buffer->len = BUFFER_SIZE;
  bzero(buffer->data, buffer->len);

  return buffer;
}

void free_buffer(struct buffer *buffer) {
  free(buffer->data);
  free(buffer);
  return;
}

void buffer_append(struct buffer *buffer, char *data) {
  int buf_len = strlen(buffer->data);
  if (strlen(data) > (buffer->len - buf_len - 1)) {
    int new_len = buffer->len * 2;
    while (strlen(data) > (new_len - buf_len - 1)) {
      new_len *= 2;
    }
    char *new_buff = (char *) malloc(sizeof(char) * new_len);
    bcopy(buffer->data, new_buff, buffer->len);
    bzero(buffer->data + buffer->len, new_len - buffer->len);
    free(buffer->data);
    buffer->data = new_buff;
    buffer->len = new_len;
  } else if ((strlen(data) + buf_len) < (buffer->len / 4)) {
    int new_len = buffer->len / 2;
    char *new_buff = (char *) malloc(sizeof(char) * new_len);
    bzero(new_buff, new_len);
    bcopy(buffer->data, new_buff, buf_len);
    free(buffer->data);
    buffer->data = new_buff;
    buffer->len = new_len;
  }

  bcopy(data, buffer->data + buf_len, strlen(data));
}

struct user {
  // user info
  char *name;

  // buffers
  struct buffer *in;
  struct buffer *out;
};

struct user_node {
  struct user *user;
  struct user_node *prev;
  struct user_node *next;
};

struct node_pair {
  struct user_node *node;
  struct ev_io *watcher;
};

struct user * new_user() {
  struct user *user = (struct user *) malloc(sizeof (struct user));
  bzero(user, sizeof(user));

  user->out = new_buffer();
  user->in = new_buffer();

  return user;
}

void free_user(struct user *user) {
  free_buffer(user->out);
  free_buffer(user->in);
  if (user->name) free(user->name);
  free(user);
}

struct user_node * insert_user(struct user_node **head, struct user *user) {
  struct user_node *node = (struct user_node *) malloc(sizeof(struct user_node));
  node->user = user;
  node->next = NULL;

  int count = 1;
  if (!*head) {
    node->prev = NULL;
    *head = node;
  } else {
    struct user_node *ptr = *head;
    count++;
    while (ptr->next != NULL) {
      ptr = ptr->next;
      count++;
    }
    ptr->next = node;
    node->prev = ptr;
  }

  return node;
}

struct user * get_user(struct user_node *head, char *name) {
  struct user_node *ptr;
  struct user *user = NULL;

  for (ptr = head; ptr != NULL; ptr = ptr->next) {
    if (strcmp(ptr->user->name, name) == 0) {
      user = ptr->user;
      break;
    }
  }

  return user;
}

void delete_node(struct user_node *node) {
  if (node->prev) {
    node->prev->next = node->next;
  }
  if (node->next) {
    node->next->prev = node->prev;
  }

  free_user(node->user);
  free(node);
}

int broadcast_msg(struct user_node *head, struct user_node *src, char *msg) {
  int count = 0;
  struct user_node *ptr;

  for (ptr = head; ptr != NULL; ptr = ptr->next) {
    if (ptr == src) continue;
    buffer_append(ptr->user->out, msg);
    printf("new buffer: %s\n", ptr->user->out->data);
    count++;
  }

  return count;
}

struct user_node *users_head;

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

  struct ev_io *w_client_r = (struct ev_io *) malloc(sizeof(struct ev_io));
  struct ev_io *w_client_w = (struct ev_io *) malloc(sizeof(struct ev_io));
  struct user_node *node = insert_user(&users_head, new_user());
  w_client_r->data = (void *) node;
  w_client_w->data = (void *) node;

  if (EV_ERROR & revents) {
    perror("Invalid event in accept.");
    return;
  }

  sock = accept(watcher->fd, (struct sockaddr *) &client_addr, &client_len);

  if (sock < 0) {
    perror("Error accepting socket.");
    return;
  }
  
  ev_io_init(w_client_r, read_cb, sock, EV_READ);
  ev_io_init(w_client_w, write_cb, sock, EV_WRITE);
  ev_io_start(loop, w_client_r);
  ev_io_start(loop, w_client_w);
}

void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  struct user_node *node = (struct user_node *) watcher->data;
  struct user *user = node->user;

  if (EV_ERROR & revents) {
    perror("Invalid event in read.");
    return;
  }

  char buf[BUFFER_SIZE];
  bzero(buf, BUFFER_SIZE);
  int read = recv(watcher->fd, buf, BUFFER_SIZE, 0); 

  if (read < 0) {
    perror("Read error in read.");
    return;
  } else if (read == 0) {
    // Read socket is in charge of closing
    printf("Socket %d closed.\n", watcher->fd);
    ev_io_stop(loop, watcher);
    delete_node(node);
    free(watcher);
    return;
  }

  buffer_append(user->in, buf);

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
      int _len = strlen(msg) + 3;
      char *_msg = (char *) malloc(sizeof(char) * _len);
      snprintf(_msg, _len, "%s\r\n\0", msg);
      printf("msg: %s", _msg);
      int count = broadcast_msg(users_head, node, _msg);
      printf("%s broadcast to %d users.\n", user->name, count);
    }

    free(msg);
  }
}

void write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  struct user_node *node = (struct user_node *) watcher->data;
  struct user *user = node->user;
  struct buffer *buffer = user->out;

  if (EV_ERROR & revents) {
    perror("Invalid event in read.");
    return;
  }

  if (buffer->data[0] == 0) {
    return;
  }

  int n = send(watcher->fd, buffer->data, strlen(buffer->data) + 1, 0);

  bzero(buffer->data, buffer->len); 
}

