#include <glib.h>
#include <ev.h>
#include <netinet/in.h>
#include <strings.h>
#include <stdlib.h>

#include "irc.h"
#include "paisley.h"

irc_user * new_irc_user(GString *nick, GString *username, GString *realname) {
  irc_user *user = (irc_user *) malloc(sizeof(irc_user));
  bzero(user, sizeof(irc_user));

  if (nick) user->nick = g_string_new(nick->str);
  if (username) user->username = g_string_new(username->str);
  if (realname) user->realname = g_string_new(realname->str);

  return user;
}

void free_irc_user(irc_user *user) {
  g_string_free(user->nick, TRUE);
  g_string_free(user->username, TRUE);
  g_string_free(user->realname, TRUE);

  free(user);
}

irc_user_object * new_irc_user_object(GString *nick, GString *username, GString *realname) {
  irc_user_object *obj = (irc_user_object *) malloc(sizeof(irc_user_object));
  bzero(obj, sizeof(irc_user_object));

  obj->user_data = new_irc_user(nick, username, realname);
  obj->in = g_string_new(NULL);
  obj->out = g_string_new(NULL);

  return obj;
}

void free_irc_user_object(irc_user_object *obj) {
  free_irc_user(obj->user_data);
  g_string_free(obj->in, TRUE);
  g_string_free(obj->out, TRUE);

  free(obj);
}

void add_irc_user_object(irc_user_object *obj) {
  g_hash_table_insert(global_irc_users, (gpointer) obj->user_data->nick, (gpointer) obj);
}

void delete_irc_user_object(irc_user_object *obj) {
  g_hash_table_remove(global_irc_users, (gpointer) obj->user_data->nick);
}

int irc_broadcast_msg(irc_user_object *src, GString *msg) {
  int count = 0;
  struct user_node *ptr;

  GHashTableIter iter;
  gpointer key, value;
  GString *_key;
  irc_user_object *_value;

  g_hash_table_iter_init(&iter, global_irc_users);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    _value = (irc_user_object *) value;
    if (_value == src) continue;

    irc_send(_value, msg);
    count++;
  }

  return count;
}

int irc_send(irc_user_object *dest, GString *msg) {
  if (!dest) {
    return -1;
  }

  g_string_append_printf(dest->out, "%s\r\n", msg->str);
  return 0;
}

int irc_parse_message(GString *msg, irc_user_object *src) {


  return 0;
}

