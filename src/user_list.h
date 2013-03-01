#ifndef _USER_LIST_H_
#define _USER_LIST_H_

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

struct user * new_user();
void free_user(struct user *user);
struct user_node * insert_user(struct user_node **head, struct user *user);
struct user * get_user(struct user_node *head, char *name);
void delete_node(struct user_node **users_head, struct user_node *node);

#endif

