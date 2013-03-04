#include <glib.h>

#include "irc.h"
#include "paisley.h"
#include "user_list.h"
#include "ev.h"
#include <netinet/in.h>

int irc_broadcast_msg(struct user_node *src, GString *msg) {
  int count = 0;
  struct user_node *ptr;

  for (ptr = users_head; ptr != NULL; ptr = ptr->next) {
    if (ptr == src) continue;
    irc_send(ptr->user, msg);
    count++;
  }

  return count;
}

int irc_send(struct user *dest, GString *msg) {
  if (!dest) {
    return -1;
  }

  g_string_append_printf(dest->out, "%s\r\n", msg->str);
  return 0;
}

int irc_parse_message(GString *msg, struct user_node *src) {
  return 0;
}

