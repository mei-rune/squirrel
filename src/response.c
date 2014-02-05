#include "squirrel_config.h"
#include "response_helper.h"

#ifdef __cplusplus
extern "C" {
#endif

DLL_VARIABLE shttp_res shttp_response_call_after_writed(shttp_connection_t *external,
    shttp_write_cb cb, void *cb_data) {
  shttp_connection_internal_t          *conn;

  conn = to_internal(external);

  if(SHTTP_RES_OK != conn_outgoing(conn).failed_code) {
    return SHTTP_RES_RESPONSE_ALREADY_FAILED;
  }
  if(1 == conn_outgoing(conn).is_body_end) {
    return SHTTP_RES_RESPONSE_ALREADY_END;
  }
  if(nil == cb) {
    return SHTTP_RES_INVALID_PARAMETERS;
  }
  CALL_AFTER(conn, data_writed, cb, cb_data);
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_call_after_completed(shttp_connection_t *external,
    shttp_write_cb cb, void *cb_data) {
  shttp_connection_internal_t          *conn;
  conn = to_internal(external);

  if(SHTTP_RES_OK != conn_outgoing(conn).failed_code) {
    return SHTTP_RES_RESPONSE_ALREADY_FAILED;
  }
  if(1 == conn_response(conn).head_writed) {
    return SHTTP_RES_HEAD_WRITED;
  }
  if(1 == conn_outgoing(conn).is_body_end) {
    shttp_assert(0 == conn_outgoing(conn).is_body_end);
    return SHTTP_RES_RESPONSE_ALREADY_END;
  }
  if(nil == cb) {
    return SHTTP_RES_INVALID_PARAMETERS;
  }
  CALL_AFTER(conn, completed, cb, cb_data);
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_start(shttp_connection_t *external,
    uint16_t status,
    const char *content_type,
    size_t content_type_len) {
   return _shttp_response_start_helper(to_internal(external),
      status, content_type, content_type_len);
}


DLL_VARIABLE shttp_res shttp_response_async_start(shttp_connection_t *external,
    uint16_t status,
    const char *content_type,
    size_t content_type_len) {
  shttp_connection_internal_t *conn = to_internal(external);
  if(0 == shttp_atomic_read32(&conn_outgoing(conn).is_async)) {
    return SHTTP_RES_NOT_ASYNC;
  }
#ifdef SHTTP_THREAD_SAFE
  if(0 != shttp_atomic_cvs32(&conn_outgoing(conn).thread_id, uv_thread_self(), 0)) {
    return SHTTP_RES_THREAD_SAFE;
  }
#endif

  return _shttp_response_start_helper(conn,
      status, content_type, content_type_len);
}

DLL_VARIABLE shttp_res shttp_response_set_chuncked(shttp_connection_t *external) {
  shttp_connection_internal_t          *conn;
  conn = to_internal(external);
  if(1 == conn_response(conn).head_writed) {
    return SHTTP_RES_HEAD_WRITED;
  }
  if(1 == conn_outgoing(conn).is_body_end) {
    shttp_assert(0 == conn_outgoing(conn).is_body_end);
    return SHTTP_RES_RESPONSE_ALREADY_END;
  }
  conn_response(conn).chunked = 1;
  return SHTTP_RES_OK;
}

DLL_VARIABLE shttp_res shttp_response_set_async(shttp_connection_t *external) {
  shttp_connection_internal_t          *conn;
  conn = to_internal(external);
  
  if(1 == conn_outgoing(conn).is_body_end) {
    shttp_assert(0 == conn_outgoing(conn).is_body_end);
    return SHTTP_RES_RESPONSE_ALREADY_END;
  }
  shttp_atomic_set32(&conn_outgoing(conn).is_async, 1);
  return SHTTP_RES_OK;
}

DLL_VARIABLE void shttp_response_pool_free (shttp_connection_t *external, void *data) {
  spool_free(external->pool, data);
}

DLL_VARIABLE void shttp_response_c_free (shttp_connection_t *conn, void *data) {
  sl_free(data);
}

DLL_VARIABLE shttp_res shttp_response_write_header(shttp_connection_t *external,
    const char *key,
    size_t     key_len,
    const char *value,
    size_t     value_len) {
  return _shttp_response_write_header_helper(to_internal(external),
    key, key_len, value, value_len);
}

DLL_VARIABLE shttp_res shttp_response_async_write_header(shttp_connection_t *external,
    const char *key,
    size_t     key_len,
    const char *value,
    size_t     value_len) {
  shttp_connection_internal_t* conn = to_internal(external);
  IS_THREAD_SAFE(conn);
  return _shttp_response_write_header_helper(conn,
    key, key_len, value, value_len);
}

DLL_VARIABLE shttp_res shttp_response_format_header(shttp_connection_t *external,
    const char *key,
    size_t     key_len,
    const char *fmt,
    ...) {
  int                                  rc;
  va_list                              args;
  va_start(args, fmt);
  rc = _shttp_response_format_header_helper(to_internal(external),
    key, key_len, fmt, args);
  va_end(args);
  return rc;
}

DLL_VARIABLE shttp_res shttp_response_vformat_header(shttp_connection_t *external,
    const char *key,
    size_t      key_len,
    const char *fmt,
    va_list     args) {
  return _shttp_response_format_header_helper(to_internal(external),
    key, key_len, fmt, args);
}

DLL_VARIABLE shttp_res shttp_response_async_format_header(shttp_connection_t *external,
    const char *key,
    size_t     key_len,
    const char *fmt,
    ...) {
  int                                  rc;
  va_list                              args;
  shttp_connection_internal_t*         conn;
  
  va_start(args, fmt);
  conn = to_internal(external);
  IS_THREAD_SAFE(conn);
  rc = _shttp_response_format_header_helper(to_internal(external),
    key, key_len, fmt, args);
  va_end(args);
  return rc;
}

DLL_VARIABLE shttp_res shttp_response_async_vformat_header(shttp_connection_t *external,
    const char *key,
    size_t      key_len,
    const char *fmt,
    va_list     args) {
  shttp_connection_internal_t*         conn;

  conn = to_internal(external);
  IS_THREAD_SAFE(conn);
  return _shttp_response_format_header_helper(conn,
    key, key_len, fmt, args);
}

DLL_VARIABLE shttp_res shttp_response_write(shttp_connection_t *external,
    const char *data,
    size_t length,
    shttp_write_cb cb,
    void *cb_data) {
   return _shttp_response_write_helper(to_internal(external),
     data, length, cb, cb_data);
}

DLL_VARIABLE shttp_res shttp_response_async_write(shttp_connection_t *external,
    const char *data,
    size_t length,
    shttp_write_cb cb,
    void *cb_data) {
  shttp_connection_internal_t*    conn;
  conn = to_internal(external);
  IS_THREAD_SAFE(conn);
  return _shttp_response_write_helper(conn,
     data, length, cb, cb_data);
}

DLL_VARIABLE shttp_res shttp_response_write_copy(shttp_connection_t *external,
    const char *data, size_t length) {
  return _shttp_response_write_copy_helper(to_internal(external), data, length);
}

DLL_VARIABLE shttp_res shttp_response_async_write_copy(shttp_connection_t *external,
    const char *data, size_t length) {
  shttp_connection_internal_t   *conn;

  conn = to_internal(external);
  return _shttp_response_write_copy_helper(conn, data, length);
}

DLL_VARIABLE shttp_res shttp_response_format(shttp_connection_t *external,
    const char *fmt, ...) {
  va_list                              args;
  shttp_res                            rc;
  
  va_start(args, fmt);
  rc = _shttp_response_vformat_helper(to_internal(external), fmt, args);
  va_end(args);

  return rc;
}

DLL_VARIABLE shttp_res shttp_response_async_format(shttp_connection_t *external,
    const char *fmt, ...) {
  shttp_connection_internal_t         *conn;
  va_list                              args;
  shttp_res                            rc;

  conn = to_internal(external);
  IS_THREAD_SAFE(conn);

  va_start(args, fmt);
  rc = _shttp_response_vformat_helper(conn, fmt, args);
  va_end(args);

  return rc;
}

DLL_VARIABLE shttp_res shttp_response_vformat(shttp_connection_t *external,
    const char *fmt, va_list args) {
  return _shttp_response_vformat_helper(to_internal(external), fmt, args);
}

DLL_VARIABLE shttp_res shttp_response_async_vformat(shttp_connection_t *external,
    const char *fmt, va_list args) {
  IS_THREAD_SAFE(to_internal(external));
  return _shttp_response_vformat_helper(to_internal(external), fmt, args);
}

void _shttp_response_async_send(shttp_connection_internal_t *conn) {
  if(0 == shttp_atomic_read32(&conn_outgoing(conn).is_writing)) {
    abort();
  }
  _shttp_response_send_immediate(conn, &_shttp_response_async_send, 1);
}

DLL_VARIABLE shttp_res shttp_response_async_flush(shttp_connection_t *external, shttp_write_cb cb, void* cb_data) {
  shttp_connection_internal_t   *conn;
  conn = to_internal(external);
  IS_THREAD_SAFE(conn);
  if(0 == conn_response(conn).chunked) {
    ERR("callback: chunked must is true while body is not completed.");
    return SHTTP_RES_BODY_NOT_COMPLETE;
  }
  return _shttp_response_async_flush_helper(to_internal(external), cb, cb_data);
}

DLL_VARIABLE shttp_res shttp_response_end(shttp_connection_t *external) {
  return _shttp_response_end_helper(to_internal(external));
}

DLL_VARIABLE shttp_res shttp_response_async_end(shttp_connection_t *external, shttp_write_cb cb, void* cb_data) {
  shttp_res                      rc;
  shttp_connection_internal_t   *conn;

  conn = to_internal(external);
  IS_THREAD_SAFE(conn);
  rc = _shttp_response_end_helper(conn);
  if(SHTTP_RES_OK == rc) {
    return _shttp_response_async_flush_helper(conn, cb, cb_data);
  }
  return rc;
}

void _shttp_response_on_completed(shttp_connection_internal_t *conn, int status) {
  shttp_assert((0 == status)? true : (SHTTP_RES_OK != conn_outgoing(conn).failed_code));
  conn_outgoing(conn).head_write_buffers.length = 0;
  conn_outgoing(conn).body_write_buffers.length = 0;
  _shttp_response_call_hooks_after_writed(conn);
  _shttp_response_call_hooks_after_completed(conn);

  _shttp_response_assert_after_response_end(conn);
  
  if(nil != conn_outgoing(conn).content_type.str) {
    conn_response_free(conn, conn_outgoing(conn).content_type.str);
    conn_outgoing(conn).content_type.len = 0;
    conn_outgoing(conn).content_type.str = nil;
  }
  
  shttp_atomic_set32(&conn_outgoing(conn).is_async, 0);
  shttp_atomic_set32(&conn_outgoing(conn).is_writing, 0);
  shttp_atomic_set32(&conn_outgoing(conn).thread_id, 0);
}

void _shttp_response_sync_send(shttp_connection_internal_t *conn) {
  _shttp_response_send_immediate(conn, &_shttp_response_sync_send, 0);
}

#ifdef __cplusplus
};
#endif