
#include "squirrel_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif

  static int  _http_request_init(http_parser *parser);
  static void _shttp_on_connect(uv_stream_t* server_handle, int status);

  /*****************************************************************************
   * HTTP Server API
   ****************************************************************************/
  
	  
  DLL_VARIABLE shttp_t *shttp_create(shttp_settings_t *settings) {
	shttp_t *http = NULL;
	shttp_connection_internal_t *conn = NULL;
	size_t i = 0;

	if (0 == shttp_pagesize){
		os_init();
		
		if (0 == shttp_pagesize){
			ERR("Please first call os_init() or os_init() failed.");
			return NULL;
		}
	}
	

	http = (shttp_t*)sl_calloc(1, sizeof(shttp_t));

	if (NULL == http) {
		ERR("calloc failed.");
		return (NULL);
	}

	if (NULL == settings) {
		http->settings.timeout.tv_sec = 0;
		http->settings.timeout.tv_usec = 0;
	} else {
		memcpy(&http->settings.timeout, &settings->timeout, sizeof(struct timeval));
	}

	if (NULL == settings || settings->max_headers_size <= 0) {
		http->settings.max_headers_size = SIZE_MAX;
	} else {
		http->settings.max_headers_size = settings->max_headers_size;
	}
	
	if (NULL == settings || settings->max_body_size <= 0) {
		http->settings.max_body_size = UINT64_MAX;
	} else {
		http->settings.max_body_size = settings->max_body_size;
	}

	if (NULL == settings || settings->allowed_methods <= 0) {
		http->settings.allowed_methods = SHTTP_REQ_GET | SHTTP_REQ_POST |
	                            SHTTP_REQ_HEAD | SHTTP_REQ_PUT | SHTTP_REQ_DELETE;
	} else {
		http->settings.allowed_methods = settings->allowed_methods;
	}
	
	if (NULL == settings || settings->max_connections_size <= 0) {
		http->settings.max_connections_size = SHTTP_MAX_CONNECTIONS;
    } else {
		http->settings.max_connections_size = settings->max_connections_size;
    }

	if (NULL == settings || settings->user_ctx_size < 0) {
		http->settings.user_ctx_size = SHTTP_CTX_SIZE;
	} else {
		http->settings.user_ctx_size = shttp_mem_align(settings->user_ctx_size);
	}
	
	TAILQ_INIT(&http->connections);
	TAILQ_INIT(&http->free_connections);
	TAILQ_INIT(&http->listenings);

    http->uv_loop = uv_default_loop();

	_http_request_init(&http->parser_settings);

	for(i = 0; i < http->settings.max_connections_size; ++i) {
		conn = (shttp_connection_internal_t*)sl_calloc(1, shttp_mem_align(sizeof(shttp_connection_internal_t)) +
			http->settings.user_ctx_size);
		
		conn->inner.http = http;
		conn->inner.ctx = ((char*)conn) + shttp_mem_align(sizeof(shttp_connection_internal_t));
	
		TAILQ_INSERT_TAIL(&http->free_connections, conn, next);
	}

	return (http);
  }

  DLL_VARIABLE shttp_res shttp_listen_at(shttp_t* http, char *listen_addr, short port) {
    int rc;
    shttp_listening_t *listening;
    struct sockaddr_in address;
    struct sockaddr_in6 address6;

    listening = (shttp_listening_t *)sl_calloc(1, sizeof(shttp_listening_t));

    rc = uv_tcp_init(http->uv_loop, &listening->uv_handle);
    UV_CHECK(rc, http->uv_loop, "init");
    listening->uv_handle.data = listening;

	if(nil == listen_addr || '[' != listen_addr[0]) {
		rc = uv_ip4_addr(listen_addr, port, &address);
		UV_CHECK(rc, http->uv_loop, "inet_pton");
		rc = uv_tcp_bind(&listening->uv_handle, (struct sockaddr*)&address);
		UV_CHECK(rc, http->uv_loop, "bind");
	} else {
		rc = uv_ip6_addr(listen_addr, port, &address6);
		UV_CHECK(rc, http->uv_loop, "inet_pton");
		rc = uv_tcp_bind(&listening->uv_handle, (struct sockaddr*)&address6);
		UV_CHECK(rc, http->uv_loop, "bind");
	}

    rc = uv_listen((uv_stream_t*)&listening->uv_handle, 128, &_shttp_on_connect);
    UV_CHECK(rc, http->uv_loop, "listening");

	TAILQ_INSERT_TAIL(&http->listenings, listening, next);
    return SHTTP_RES_OK;
  }
  
  static void _shttp_free(shttp_t *http) {
	shttp_connection_internal_t *conn;
	shttp_listening_t  *listening;
	
	assert(TAILQ_EMPTY(&http->listenings));
	assert(TAILQ_EMPTY(&http->connections));

	TAILQ_FOREACH(conn, &http->free_connections, next) {
		sl_free(conn);
	}
    sl_free(http);
  }

  DLL_VARIABLE shttp_res shttp_run(shttp_t *http) {
    int rc;
    rc = uv_run(http->uv_loop, UV_RUN_DEFAULT);
    UV_CHECK(rc, http->uv_loop, "listening");
    return SHTTP_RES_OK;
  }

  static void _shttp_on_listening_close(uv_handle_t* handle) {
	  shttp_listening_t *listening;
	  shttp_t           *http;
	  
	  listening = (shttp_listening_t *)handle->data;
	  http = listening->http;

	  TAILQ_REMOVE(&http->listenings, listening, next);
	  sl_free(listening);
  }

  DLL_VARIABLE shttp_res shttp_shutdown(shttp_t *http) {
	shttp_listening_t* listening;

	TAILQ_FOREACH(listening, &http->listenings, next) {
		uv_close((uv_handle_t *)&listening->uv_handle, &_shttp_on_listening_close);
	}
    return SHTTP_RES_OK;
  }
  
#ifdef __cplusplus
};
#endif

//
//#define UVERR(err, msg) fprintf(stderr, "%s: %s\n", msg, uv_strerror(err))
//#define CHECK(r, msg)                         \
//if (r) {                                          \
//  uv_err_t err = uv_last_error(uv_loop);  \
//  UVERR(err, msg);                              \
//  exit(1);                                        \
//}
//
//KHASH_MAP_INIT_STR(string_hashmap, hw_route_entry*)
//
//
//http_connection* create_http_connection(http_server_t* srv)
//{
//    http_connection* connection = (http_connection*)malloc(sizeof(http_connection));
//    connection->request = NULL;
//    INCREMENT_STAT(stat_connections_created_total);
//    return connection;
//}
//
//void free_http_connection(http_connection* connection)
//{
//    if (connection->request != NULL)
//    {
//        free_http_request(connection->request);
//    }
//    
//    free(connection);
//    INCREMENT_STAT(stat_connections_destroyed_total);
//}
//
//void set_route(void* hashmap, char* name, hw_route_entry* route_entry)
//{
//    int ret;
//    khiter_t k;
//    khash_t(string_hashmap) *h = (khash_t(string_hashmap)*)hashmap;
//    k = kh_put(string_hashmap, h, dupstr(name), &ret);
//    kh_value(h, k) = route_entry;
//}
//
//void http_add_route(http_server_t* srv, char *route, http_request_callback callback, void* user_data)
//{
//    hw_route_entry* route_entry = (hw_route_entry*)malloc(sizeof(hw_route_entry));
//    route_entry->callback = callback;
//    route_entry->user_data = user_data;
//    
//    if (NULL == srv->routes)
//    {
//        srv->routes= kh_init(string_hashmap);
//    }
//    set_route(srv->routes, route, route_entry);
//    printf("Added route %s\n", route); // TODO: Replace with logging instead.
//}
//
//http_server_t* http_init_from_config(char* configuration_filename) {
//    configuration* config = load_configuration(configuration_filename);
//    if (NULL == config)
//    {
//        return NULL;
//    }
//    return http_init_with_config(config);
//}
//
//http_server_t* http_init_with_config(configuration* c) {
//    http_server_t* srv;
//    int http_listen_address_length;
//
//    srv = (http_server_t*)malloc(sizeof(http_server_t));
//    srv->config.http_listen_address = dupstr(c->http_listen_address);
//    srv->config.http_listen_port = c->http_listen_port;
//    
//    http_add_route(srv, "/stats", get_server_stats, NULL);
//
//    //http_v1_0 = create_string("HTTP/1.0 ");
//    //http_v1_1 = create_string("HTTP/1.1 ");
//    string_init(&srv->name, "Server: squirrel");
//    return 0;
//}
//
//void http_free_server(http_server_t* srv) {
//    /* TODO: Shut down accepting incoming requests */
//    khash_t(string_hashmap) *h = (khash_t(string_hashmap)*)srv->routes;
//    const char* k;
//    const char* v;
//
//    kh_foreach(h, k, v, { free((char*)k); free((char*)v); });
//    kh_destroy(string_hashmap, srv->routes);
//}
//
//int hw_http_open(http_server_t* srv) {
//    srv->parser_settings.on_header_field = http_request_on_header_field;
//    srv->parser_settings.on_header_value = http_request_on_header_value;
//    srv->parser_settings.on_headers_complete = http_request_on_headers_complete;
//    srv->parser_settings.on_body = http_request_on_body;
//    srv->parser_settings.on_message_begin = http_request_on_message_begin;
//    srv->parser_settings.on_message_complete = http_request_on_message_complete;
//    srv->parser_settings.on_url = http_request_on_url;
//    
//#ifdef PLATFORM_POSIX
//    signal(SIGPIPE, SIG_IGN);
//#endif // PLATFORM_POSIX
//    
//    
//    /* TODO: Use the return values from uv_tcp_init() and uv_tcp_bind() */
//    srv->uv_loop = uv_default_loop();
//    uv_tcp_init(srv->uv_loop, &srv->server);
//    srv->server.data = srv;
//    
//    /* If running single threaded there is no need to use the IPC pipe
//     to distribute requests between threads so lets avoid the IPC overhead */
//    initialize_http_request_cache();
//    if ('[' == srv->config.http_listen_address[0]) {
//        uv_ip4_addr(srv->config.http_listen_address, srv->config.http_listen_port, &srv->listen_address);
//    } else {
//        uv_ip6_addr(srv->config.http_listen_address, srv->config.http_listen_port, &srv->listen_address);
//    }
//    uv_tcp_bind(&srv->server, (const struct sockaddr*)&srv->listen_address);
//    uv_listen((uv_stream_t*)&srv->server, 128, http_stream_on_connect);
//    printf("Listening on %s:%d\n", srv->config.http_listen_address, srv->config.http_listen_port);
//    uv_run(srv->uv_loop, UV_RUN_DEFAULT);
//    return 0;
//}
//
//void http_stream_on_connect(uv_stream_t* stream, int status) {
//    http_server_t* srv = (http_server_t*)stream->data;
//
//    http_connection* connection = create_http_connection(srv);
//    uv_tcp_init(srv->uv_loop, &connection->stream);
//    http_parser_init(&connection->parser, HTTP_REQUEST);
//    connection->parser.data = connection;
//    connection->stream.data = connection;
//    
//    /* TODO: Use the return values from uv_accept() and uv_read_start() */
//    uv_accept(stream, (uv_stream_t*)&connection->stream);
//    uv_read_start((uv_stream_t*)&connection->stream, http_stream_on_alloc, http_stream_on_read);
//}
//
//void http_stream_on_alloc(uv_handle_t* client, size_t suggested_size, uv_buf_t* buf)
//{
//    buf->base = malloc(suggested_size);
//    buf->len = suggested_size;
//}
//
//void http_stream_on_close(uv_handle_t* handle)
//{
//    http_connection* connection = (http_connection*)handle->data;
//    free_http_connection(connection);
//}
//
//void http_stream_on_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf)
//{
//    ssize_t parsed;
//    http_connection* connection = (http_connection*)tcp->data;
//
//    uv_shutdown_t* req;
//    if (nread < 0)
//    {
//        // Error or EOF
//        if (buf->base)
//        {
//            free(buf->base);
//        }
//        
//        req = (uv_shutdown_t*) malloc(sizeof *req);
//        uv_close((uv_handle_t*) &connection->stream, http_stream_on_close);
//        return;
//    }
//    
//    if (nread == 0)
//    {
//        // Everything OK, but nothing read.
//        free(buf->base);
//        return;
//    }
//    
//    parsed = http_parser_execute(&connection->parser, &connection->srv->parser_settings, buf->base, nread);
//    
//}
//
//int http_server_write_response(hw_write_context* write_context, string_t* response)
//{
//    uv_write_t* write_req = (uv_write_t *)malloc(sizeof(*write_req) + sizeof(uv_buf_t));
//    uv_buf_t* resbuf = (uv_buf_t *)(write_req+1);
//    
//    resbuf->base = response->value;
//    resbuf->len = response->length + 1;
//    
//    write_req->data = write_context;
//    
//    /* TODO: Use the return values from uv_write() */
//    uv_write(write_req, (uv_stream_t*)&write_context->connection->stream, resbuf, 1, http_server_after_write);
//    return 0;
//}
//
//void http_server_after_write(uv_write_t* req, int status)
//{
//    hw_write_context* write_context = (hw_write_context*)req->data;
//    uv_buf_t *resbuf = (uv_buf_t *)(req+1);
//    
//    if (!write_context->connection->keep_alive)
//    {
//        uv_close((uv_handle_t*)req->handle, http_stream_on_close);
//    }
//    
//    if (write_context->callback != 0)
//    {
//        write_context->callback(write_context->user_data);
//    }
//    
//    free(write_context);
//    free(resbuf->base);
//    free(req);
//}
