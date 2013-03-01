#ifndef _BUFFERS_H_
#define _BUFFERS_H_

struct buffer {
  char *data;
  int len;
};

struct buffer * new_buffer();
void free_buffer(struct buffer *buffer);
void append_buffer(struct buffer *buffer, char *data);

#endif

