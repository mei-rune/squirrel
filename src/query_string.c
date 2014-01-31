
#include "squirrel_config.h"
#include <malloc.h>
#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif

enum unscape_state {
  unscape_state_start = 0,
  unscape_state_hex1,
  unscape_state_hex2
};

int _shttp_unescape_string(unsigned char ** out, unsigned char * str, size_t str_len) {
  unsigned char    * optr;
  unsigned char    * sptr;
  unsigned char      d;
  unsigned char      ch;
  unsigned char      c;
  size_t             i;
  enum unscape_state state;

  if (out == NULL || *out == NULL) {
    return -1;
  }

  state = unscape_state_start;
  optr  = *out;
  sptr  = str;
  d     = 0;

  for (i = 0; i < str_len; i++) {
    ch = *sptr++;

    switch (state) {
    case unscape_state_start:
      if (ch == '%') {
        state = unscape_state_hex1;
        break;
      }

      *optr++ = ch;

      break;
    case unscape_state_hex1:
      if (ch >= '0' && ch <= '9') {
        d     = (unsigned char)(ch - '0');
        state = unscape_state_hex2;
        break;
      }

      c = (unsigned char)(ch | 0x20);

      if (c >= 'a' && c <= 'f') {
        d     = (unsigned char)(c - 'a' + 10);
        state = unscape_state_hex2;
        break;
      }

      state   = unscape_state_start;
      *optr++ = ch;
      break;
    case unscape_state_hex2:
      state   = unscape_state_start;

      if (ch >= '0' && ch <= '9') {
        ch      = (unsigned char)((d << 4) + ch - '0');

        *optr++ = ch;
        break;
      }

      c = (unsigned char)(ch | 0x20);

      if (c >= 'a' && c <= 'f') {
        ch      = (unsigned char)((d << 4) + c - 'a' + 10);
        *optr++ = ch;
        break;
      }

      break;
    } /* switch */
  }

  return 0;
}

typedef enum {
  s_query_start = 0,
  s_query_question_mark,
  s_query_separator,
  s_query_key,
  s_query_val,
  s_query_done
} shttp_query_parser_state;

static inline int _shttp_is_hex_query_char(unsigned char ch) {
  switch (ch) {
  case 'a':
  case 'A':
  case 'b':
  case 'B':
  case 'c':
  case 'C':
  case 'd':
  case 'D':
  case 'e':
  case 'E':
  case 'f':
  case 'F':
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    return 1;
  default:
    return 0;
  } /* switch */
}

#define restart_kv(p)                         \
        key_mark = p;                         \
        val_mark = nil;                       \
        key_len  = 0;                         \
        val_len  = 0


int shttp_parse_query_string(const char * query, size_t len, shttp_query_cb cb, void* ctx) {
  shttp_query_parser_state   state   = s_query_start;
  const char               * key_mark = nil;
  const char               * val_mark = nil;
  size_t                     key_len;
  size_t                     val_len;
  unsigned char              ch;
  size_t                     i;
  int                        res;

  key_len = 0;
  val_len = 0;

  for (i = 0; i < len; i++) {
    ch  = query[i];

    switch (state) {
    case s_query_start:


      switch (ch) {
      case '?':
        state = s_query_key;
        restart_kv(&query[i+1]);
        break;
      case '/':
        state = s_query_question_mark;
        break;
      default:
        state = s_query_key;
        restart_kv(&query[i]);

        goto query_key;
      }

      break;
    case s_query_question_mark:
      if( '?' == ch) {
        state = s_query_key;
        restart_kv(&query[i+1]);
      }
      break;
query_key:
    case s_query_key:
      if('=' == ch) {
        state = s_query_val;
        key_len  = (&query[i] - key_mark);
        val_mark = &query[i+1];
        break;
      }

      if( ';' == ch || '&' == ch ) {
        key_len  = (&query[i] - key_mark);
        cb(ctx, key_mark, key_len, nil, 0);

        state = s_query_key;
        restart_kv(&query[i+1]);
      }
      break;
    case s_query_val:
      if( ';' == ch || '&' == ch ) {
        val_len  = (&query[i] - val_mark);
        cb(ctx, key_mark, key_len, val_mark, val_len);

        state = s_query_key;
        restart_kv(&query[i+1]);
      }
      break;
    default:
      /* bad state */
      return -1;
    }       /* switch */
  }
  switch(state) {
  case s_query_key:
    key_len  = (&query[i] - key_mark);
    if (0 != key_len || 0 != val_len) {
      cb(ctx, key_mark, key_len, nil, 0);
    }
    break;
  case s_query_val:
    val_len  = (&query[i] - val_mark);
    cb(ctx, key_mark, key_len, val_mark, val_len);
    break;
  }
  return 0;
}     /* evhtp_parse_query */

#ifdef __cplusplus
};
#endif