
#include "squirrel_config.h"

#include <assert.h>

#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif

  /*****************************************************************************
   * HTTP Server Callback Functions
   ****************************************************************************/
  
  static void _shttp_on_null_disconnect(uv_handle_t* server_handle) {
	  sl_free(server_handle);
  }

  static void _shttp_on_null_connect(uv_stream_t* server_handle, int status) {
	  int          rc;
	  uv_handle_t *handle;

      switch (server_handle->type) {
      case UV_TCP:
		  handle = (uv_handle_t *)sl_calloc(1, sizeof(uv_tcp_t));
		  uv_tcp_init(server_handle->loop, (uv_tcp_t *)handle);
		  break;
      case UV_NAMED_PIPE:
		  handle = (uv_handle_t *)sl_calloc(1, sizeof(uv_pipe_t));
		  uv_pipe_init(server_handle->loop, (uv_pipe_t *)handle, 0);
		  break;
	  default:
		  assert(server_handle->type != UV_TCP);
	  }

	rc = uv_accept(server_handle, (uv_stream_t*)handle);
	if(rc) {
        ERR("accept: %s\n", uv_strerror(rc));
		goto end;
	}
	uv_close(handle, &_shttp_on_null_disconnect);
end:
	sl_free(handle);
  }
  
  static void _shttp_on_disconnect(uv_handle_t* handle) {
    shttp_connection_internal_t *conn;
    shttp_t            *http;
	conn = (shttp_connection_internal_t *)handle->data;
	http = conn->inner.http;

	TAILQ_REMOVE(&http->connections, conn, next);
	TAILQ_INSERT_TAIL(&http->free_connections, conn, next);
  }

  static void _shttp_on_connection_alloc(uv_handle_t* req, size_t suggested_size, uv_buf_t* buf) {
    buf->base = (char*)sl_malloc(suggested_size);
    buf->len = suggested_size;
  }

  static void _shttp_on_connection_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf) {
    size_t parsed;
    // Get the client
    shttp_connection_internal_t *client = (shttp_connection_internal_t*)tcp->data;

    if (nread > 0) {
      // Read Data for Client connection
      parsed = http_parser_execute(&client->parser, &client->inner.http->parser_settings, buf->base, nread);
      if (parsed < nread || http_should_keep_alive(&client->parser)) {
		if(HPE_OK != HTTP_PARSER_ERRNO(&client->parser)) {
          ERR("parse error:%s\n", http_errno_name(HTTP_PARSER_ERRNO(&client->parser)));
		}
        uv_close((uv_handle_t*) &client->uv_handle, &_shttp_on_disconnect);
      }
    } else {
      // No Data Read... Handle Error case
      if (0 != nread && UV_EOF != nread) {
        ERR("read: %s\n", uv_strerror(nread));
      }
      uv_close((uv_handle_t*) &client->uv_handle, &_shttp_on_disconnect);
    }

    // Free memory that was allocated by _http_uv__on_alloc__cb
    sl_free(buf->base);
  }

  static void _shttp_on_connect(uv_stream_t* server_handle, int status) {

	int                 rc;
    shttp_listening_t  *listening;
    shttp_connection_internal_t *conn;
    shttp_t            *http;

	if(-1 == status) {
        ERR("accept: %s\n", uv_strerror(status));
		return;
	}

    listening = (shttp_listening_t *)server_handle->data;
	http = listening->http;
	if(TAILQ_EMPTY(&http->free_connections)) {
		goto failed;
	}

    conn = TAILQ_LAST(&http->free_connections, shttp_connections_s);
	
    uv_tcp_init(http->uv_loop, &conn->uv_handle);
	conn->uv_handle.data = conn;

    http_parser_init(&conn->parser, HTTP_REQUEST);
    conn->parser.data = conn;
    conn->inner.http = http;
	conn->inner.internal = conn;

    rc = uv_accept(server_handle, (uv_stream_t*)&conn->uv_handle);
	if(rc) {
        ERR("accept: %s\n", uv_strerror(rc));
		return ;
	}
	TAILQ_REMOVE(&http->free_connections, conn, next);
	TAILQ_INSERT_TAIL(&http->connections, conn, next);

	rc = uv_read_start((uv_stream_t*)&conn->uv_handle,
                             _shttp_on_connection_alloc,
							 _shttp_on_connection_read);

	if(rc) {
        ERR("accept: %s\n", uv_strerror(rc));
		uv_close((uv_handle_t*)&conn->uv_handle, &_shttp_on_disconnect);
		return ;
	}
	
    return ;
failed:
	_shttp_on_null_connect(server_handle, status);
    return ;
  }

#ifdef __cplusplus
};
#endif