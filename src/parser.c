#include <strings.h>
#include <glib.h>

#include "irc.h"
#include "parser.h"

char * parse_const(char *msg, char *str) {
  int str_len = strlen(str);
  if (strlen(msg) < str_len) return NULL;

  char *msg_ptr, *str_ptr;
  for (msg_ptr = msg, str_ptr = str; (str_ptr - str) < str_len; msg_ptr++, str_ptr++) {
    if (*msg_ptr != *str_ptr) return NULL;
  }

  return msg_ptr;
}

int parse_irc_user_msg(char *msg, irc_user *user) {
  msg = parse_const(msg, "USER ");
  if (!msg) return 0;

  int delim_count = 0;
  int msg_len = strlen(msg);
  GString *username = NULL;
  GString *realname = NULL;

  char *ptr, *last_delim;
  for (ptr = msg, last_delim = msg; (ptr - msg) <= msg_len; ptr++) {
    if (IS_NICKCHAR(*ptr) ||
        (delim_count == 1 && *ptr == '*')) {
      continue;
    }

    if (IS_SPACE(*ptr) && delim_count == 0) {
      delim_count++;
      username = g_string_new_len(last_delim, ptr - last_delim);
      last_delim = ptr + 1;
    } else if (IS_SPACE(*ptr) && delim_count == 1) {
      delim_count++;
      last_delim = ptr + 1;
    } else if ((ptr - msg) == msg_len) {
      delim_count++;
      realname = g_string_new_len(last_delim, ptr - last_delim + 1);
    } else {
      return 0;
    }
  }

  if (user->username) g_string_free(user->username, TRUE);
  if (user->realname) g_string_free(user->realname, TRUE);
  user->username = username;
  user->realname = realname;
  
  return 1;
}

int parse_irc_nick_msg(char *msg, irc_user *user) {
  msg = parse_const(msg, "NICK ");
  if (!msg) return 0;

  int msg_len = strlen(msg);

  if (!msg_len) return 0;

  char *ptr;
  for (ptr = msg; (ptr - msg) < msg_len; ptr++) {
    if (!IS_NICKCHAR(*ptr)) {
      return 0;
    }
  }

  if (user->nick) g_string_free(user->nick, TRUE);
  user->nick = g_string_new(msg);
  
  return 1;
}

