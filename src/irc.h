#ifndef _IRC_H_
#define _IRC_H_

#include "user_list.h"
#include "ev.h"

typedef int irc_mode;

struct irc_user_data {
  char *username, *realname;
  irc_mode *modes;
};

struct irc_user {
  char *nick;
  struct irc_user_data user_data;

};

struct irc_channel_mode {
  irc_mode mode;
  char *target;
};

struct irc_channel {
  char *name;
  struct irc_channel_mode *modes;
};

int irc_send(struct user *dest, GString *msg);
int irc_parse_message(GString *msg, struct user_node *src);
int irc_broadcast_msg(struct user_node *src, GString *msg);

#endif

