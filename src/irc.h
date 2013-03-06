#ifndef _IRC_H_
#define _IRC_H_

#include <ev.h>
#include <glib.h>

// types / structs

typedef int irc_mode;
typedef struct _irc_user irc_user;
typedef struct _irc_user_object irc_user_object;
typedef struct _irc_channel_mode irc_channel_mode;
typedef struct _irc_channel irc_channel;

struct _irc_user {
  GString *nick, *username, *realname;
  irc_mode *modes;
};

struct _irc_user_object {
  irc_user *user_data;
  GString *in, *out;
};

struct _irc_channel_mode {
  irc_mode mode;
  char *target;
};

struct _irc_channel {
  char *name;
  irc_channel_mode *modes;
};

// struct functions

irc_user * new_irc_user(GString *nick, GString *username, GString *realname);
void free_irc_user(irc_user *user);

irc_user_object * new_irc_user_object(GString *nick, GString *username, GString *realname);
void free_irc_user_object(irc_user_object *obj);
void free_irc_user_from_hash(gpointer obj);

void add_irc_user_object(irc_user_object *obj);
void delete_irc_user_object(irc_user_object *obj);

// irc functions

int irc_send(irc_user_object *dest, GString *msg);
int irc_broadcast_msg(irc_user_object *src, GString *msg);

// parsing 

int irc_parse_message(GString *msg, irc_user_object *src);
int irc_parse_user_message(GString *msg, irc_user_object *src);
int irc_parse_nick_message(GString *msg, irc_user_object *src);

#endif

