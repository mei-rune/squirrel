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
        'deps/libssh2.gyp:libssh2',
        'deps/libssh2.gyp:libssh2_test',
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
		  'direct_dependent_settings': {
			'include_dirs': [
              'include/win32',
			]
		  },
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
        'include/squirrel_http.h',
        'include/squirrel_link.h',
        'include/squirrel_array.h',
        'include/squirrel_hashtable.h',
        'include/squirrel_string.h',
        'include/squirrel_test.h',
        'include/squirrel_pool.h',
        'include/squirrel_util.h',
        'src/internal.h',
        'src/connection.c',
        'src/request.c',
        'src/response.c',
        'src/error_response.c',
        'src/server.c',
        'src/sstring.c',
        'src/pool.c',
        'src/os.c',
        'src/log.c',
        'src/cacheline.c',
        'src/utils.c',
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
		  'direct_dependent_settings': {
			'include_dirs': [
              'include/win32',
			]
		  },
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
        'include/squirrel_http.h',
        'include/squirrel_link.h',
        'include/squirrel_array.h',
        'include/squirrel_hashtable.h',
        'include/squirrel_string.h',
        'include/squirrel_test.h',
        'include/squirrel_pool.h',
        'include/squirrel_util.h',
        'src/internal.h',
        'src/connection.c',
        'src/request.c',
        'src/response.c',
        'src/error_response.c',
        'src/server.c',
        'src/sstring.c',
        'src/pool.c',
        'src/os.c',
        'src/log.c',
        'src/cacheline.c',
        'src/utils.c',
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
        'deps/http-parser/http_parser.gyp:http_parser',
        'deps/uv/uv.gyp:libuv',
        'squirrel',
      ],
      
      'include_dirs': [
        './include',
        './src',
      ],

      'sources': [
        'test/array_test.cpp',
        'test/hashtable_test.cpp',
        'test/pool_test.cpp',
        'test/http_test.cpp',
        'test/main.c',
      ],
    }, # hello_world sample

    ########################################
    # parser test
    ########################################
    {
      'target_name': 'parser_test',
      'product_name': 'parser_test',
      'type': 'executable',

      'dependencies': [
        'deps/http-parser/http_parser.gyp:http_parser',
      ],
      
      'include_dirs': [
        './include',
      ],

      'sources': [
        'test/parser.c',
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
        'samples/hello_world.c',
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