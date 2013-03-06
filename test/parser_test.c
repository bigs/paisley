#include <assert.h>
#include <strings.h>
#include <glib.h>
#include "../src/parser.h"
#include "../src/irc.h"

void parse_const_test() {
  char *msg = "FOO bar";
  char *res = parse_const(msg, "FOO");

  assert(res != NULL);
  assert(res == msg + 3);
  assert(strcmp(res, " bar") == 0);
}

void is_inrange_test() {
  assert(IS_INRANGE('a', 'a', 'z'));
  assert(!IS_INRANGE('{', 'a', 'z'));
  assert(IS_INRANGE('f', 'a', 'z'));
  assert(!IS_INRANGE('f', 'A', 'Z'));
}

void is_alphanumeric_test() {
  assert(IS_ALPHANUMERIC('f'));
  assert(IS_ALPHANUMERIC('F'));
  assert(IS_ALPHANUMERIC('3'));
  assert(!IS_ALPHANUMERIC('_'));
}

void parse_irc_user_msg_test() {
  irc_user foo;
  foo.username = NULL;
  foo.realname = NULL;

  int res = parse_irc_user_msg("USER", &foo);
  assert(!res);
  res = parse_irc_user_msg("user foo * bar", &foo);
  assert(!res);
  res = parse_irc_user_msg("USER foo * bar", &foo);
  assert(res);
  assert(strcmp(foo.username->str, "foo") == 0);
  assert(strcmp(foo.realname->str, "bar") == 0);
  res = parse_irc_user_msg("USER fuzz things buzz", &foo);
  assert(res);
  assert(strcmp(foo.username->str, "fuzz") == 0);
  assert(strcmp(foo.realname->str, "buzz") == 0);
  res = parse_irc_user_msg("USER foo ! bar", &foo);
  assert(!res);
}

void parse_irc_nick_msg_test() {
  irc_user foo;
  foo.nick = NULL;

  int res;

  res = parse_irc_nick_msg("NICK", &foo);
  assert(!res);
  res = parse_irc_nick_msg("NICK ", &foo);
  assert(!res);
  res = parse_irc_nick_msg("NICK foo", &foo);
  assert(res);
  assert(strcmp(foo.nick->str, "foo") == 0);
  res = parse_irc_nick_msg("NICK bar", &foo);
  assert(res);
  assert(strcmp(foo.nick->str, "bar") == 0);
  res = parse_irc_nick_msg("NICK b!ar", &foo);
  assert(!res);
}

void parser_tests() {
  parse_const_test();
  is_inrange_test();
  is_alphanumeric_test();
  parse_irc_user_msg_test();
  parse_irc_nick_msg_test();
}

