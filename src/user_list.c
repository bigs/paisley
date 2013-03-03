#include <stdlib.h>
#include <strings.h>
#include <glib.h>

#include "user_list.h"

struct user * new_user() {
  struct user *user = (struct user *) malloc(sizeof (struct user));
  bzero(user, sizeof(user));

  user->out = g_string_new(NULL);
  user->in = g_string_new(NULL);

  return user;
}

void free_user(struct user *user) {
  g_string_free(user->out, TRUE);
  g_string_free(user->in, TRUE);
  if (user->name) g_string_free(user->name, TRUE);
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
    if (strcmp(ptr->user->name->str, name) == 0) {
      user = ptr->user;
      break;
    }
  }

  return user;
}

void delete_node(struct user_node **users_head, struct user_node *node) {
  if (node->prev) {
    node->prev->next = node->next;
  } else {
    if (node->next)
      *users_head = node->next;
    else
      *users_head = NULL;
  }
  if (node->next) {
    node->next->prev = node->prev;
  }

  free_user(node->user);
  free(node);
}

