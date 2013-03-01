#include <stdlib.h>
#include <strings.h>
#include "const.h"
#include "buffer.h"

struct buffer * new_buffer() {
  struct buffer *buffer = (struct buffer *) malloc(sizeof(struct buffer));
  buffer->data = (char *) malloc(sizeof(char) * BUFFER_SIZE);
  buffer->len = BUFFER_SIZE;
  bzero(buffer->data, buffer->len);

  return buffer;
}

void free_buffer(struct buffer *buffer) {
  free(buffer->data);
  free(buffer);
  return;
}

void append_buffer(struct buffer *buffer, char *data) {
  int buf_len = strlen(buffer->data);
  if (strlen(data) > (buffer->len - buf_len - 1)) {
    int new_len = buffer->len * 2;
    while (strlen(data) > (new_len - buf_len - 1)) {
      new_len *= 2;
    }
    char *new_buff = (char *) malloc(sizeof(char) * new_len);
    bcopy(buffer->data, new_buff, buffer->len);
    bzero(buffer->data + buffer->len, new_len - buffer->len);
    free(buffer->data);
    buffer->data = new_buff;
    buffer->len = new_len;
  } else if ((strlen(data) + buf_len) < (buffer->len / 4)) {
    int new_len = buffer->len / 2;
    char *new_buff = (char *) malloc(sizeof(char) * new_len);
    bzero(new_buff, new_len);
    bcopy(buffer->data, new_buff, buf_len);
    free(buffer->data);
    buffer->data = new_buff;
    buffer->len = new_len;
  }

  bcopy(data, buffer->data + buf_len, strlen(data));
}

