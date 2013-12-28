{
  'variables':{
  },
  'targets': [
    
    ########################################
    # squirrel-http static library
    ########################################
    {
      'target_name': 'squirrel',
      'product_name': 'squirrel',
      'type': 'static_library',

      'dependencies': [
        'deps/http-parser/http_parser.gyp:http_parser',
        'deps/uv/uv.gyp:libuv',
        'deps/yajl.gyp:yajl',
        'deps/oniguruma.gyp:oniguruma',
        'deps/oniguruma.gyp:oniguruma_ctest',
        'deps/oniguruma.gyp:oniguruma_utest',
        'deps/yajl.gyp:yajl_copy_headers',
      ],
     'export_dependent_settings': [
       'deps/yajl.gyp:yajl',
       'deps/uv/uv.gyp:libuv',
      ],

      'conditions': [
        ['OS=="linux"', {
          'defines': [
            'PLATFORM_LINUX',
          ],
          'cflags': [
            '-std=gnu99',
          ],
        }],
        ['OS=="win"', {
          'defines': [
            'PLATFORM_WINDOWS',
          ],
          'sources': [
            'include/win32/sys/queue.h',
            'include/win32/sys/tree.h',
          ],
          'include_dirs': [
            'include/win32',
          ],
        }, { # OS != "win",
          'defines': [
            'PLATFORM_POSIX',
          ],
        }],
        ['"<(without_ssl)" == "false"', {
          'dependencies': [
            'deps/openssl/openssl.gyp:openssl'
          ],
          'export_dependent_settings': [
            'deps/openssl/openssl.gyp:openssl'
          ],
          'defines': [ 'USE_OPENSSL' ],
        }],
      ],

      'include_dirs': [
        './include',
        './lib/libuv/include',
      ],

      'sources': [
        'include/squirrel_config.h',
        'include/shttp.h',
        'include/link.h',
        'include/array.h',
        'include/hashtable.h',
        'include/sstring.h',
        'include/test.h',
        'src/ht-internal.h',
        'src/internal.h',
        'src/shttp_connection.c',
        'src/shttp_request.c',
        'src/shttp_response.c',
        'src/shttp_server.c',
        'src/sstring.c',
        'src/shttp_os_win.c',
        'src/shttp_log.c',
        'src/test.c',
      ],

    }, # squirrel static library

    ########################################
    # squirrel shared library
    ########################################
    {
      'target_name': 'squirrel_shared',
      'product_name': 'squirrel',
      'type': 'shared_library',

      'dependencies': [
        'deps/http-parser/http_parser.gyp:http_parser',
        'deps/uv/uv.gyp:libuv',    
        'deps/yajl.gyp:yajl',
        'deps/oniguruma.gyp:oniguruma',
        'deps/oniguruma.gyp:oniguruma_ctest',
        'deps/oniguruma.gyp:oniguruma_utest',
        'deps/yajl.gyp:yajl_copy_headers',
      ],
     'export_dependent_settings': [
       'deps/yajl.gyp:yajl',
       'deps/uv/uv.gyp:libuv',
      ],

      'conditions': [
        ['OS=="linux"', {
          'defines': [
            'PLATFORM_LINUX',
          ],
          'cflags': [
            '-std=gnu99',
          ],
        }],
        ['OS=="win"', {
          'defines': [
            'PLATFORM_WINDOWS',
            'BUILDING_SQUIRREL_SHARED',
          ],
          'sources': [
            'include/win32/sys/queue.h',
            'include/win32/sys/tree.h',
          ],
          'include_dirs': [
            'include/win32',
          ],
        }, { # OS != "win",
          'defines': [
            'PLATFORM_POSIX',
          ],
        }],
        ['"<(without_ssl)" == "false"', {
          'dependencies': [
            'deps/openssl/openssl.gyp:openssl'
          ],
          'export_dependent_settings': [
            'deps/openssl/openssl.gyp:openssl'
          ],
          'defines': [ 'USE_OPENSSL' ],
        }],
      ],

      'include_dirs': [
        './include',
        './lib/libuv/include',
      ],

    'sources': [
        'include/squirrel_config.h',
        'include/shttp.h',
        'include/link.h',
        'include/array.h',
        'include/hashtable.h',
        'include/sstring.h',
        'include/test.h',
        'src/ht-internal.h',
        'src/internal.h',
        'src/shttp_connection.c',
        'src/shttp_request.c',
        'src/shttp_response.c',
        'src/shttp_server.c',
        'src/sstring.c',
        'src/shttp_os_win.c',
        'src/shttp_log.c',
        'src/test.c',
      ],

    }, # squirrel shared library

    ########################################
    # unit test
    ########################################
    {
      'target_name': 'utest',
      'product_name': 'utest',
      'type': 'executable',

      'dependencies': [
        'squirrel',
      ],
      
      'include_dirs': [
        './include',
      ],

      'sources': [
        'test/array_test.cpp',
        'test/hashtable_test.cpp',
        'test/main.c',
      ],
    }, # hello_world sample

    ########################################
    # hello_world sample
    ########################################
    {
      'target_name': 'http_hello_world',
      'product_name': 'http_hello_world',
      'type': 'executable',

      'dependencies': [
        'squirrel',
      ],
      
      'include_dirs': [
        './include',
      ],

      'sources': [
        'samples/hello_world/program.c',
      ],

      'copies': [
      {
        'destination': '<(PRODUCT_DIR)',
        'files': [
        ],
      }],
    }, # hello_world sample
  ],
}