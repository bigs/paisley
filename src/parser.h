#ifndef _PARSER_H_
#define _PARSER_H_

#include <glib.h>
#include "irc.h"

// macros

#define IS_INRANGE(c, l, u) \
  (c >= l && c <= u)

#define IS_SPACE(c) \
  (c == ' ')

#define IS_ALPHANUMERIC(c) \
  (IS_INRANGE(c, '0', '9') || \
   IS_INRANGE(c, 'A', 'Z') || \
   IS_INRANGE(c, 'a', 'z'))

#define IS_NICKCHAR(c) \
  (IS_ALPHANUMERIC(c) || \
   c == '_' || \
   c == '-' || \
   c == '^')

#define IS_HOSTCHAR(c) \
  (IS_ALPHANUMERIC(c) || \
   c == '_' \
   c == '-' \
   c == '.')

// parse functions

char * parse_const(char *msg, char *str);
int parse_irc_user_msg(char *msg, irc_user *user);
int parse_irc_nick_msg(char *msg, irc_user *user);

#endif

