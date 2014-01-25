{
  'variables': {
  },
  'targets': [
    {
      'target_name': 'libssh2',
      'type': 'static_library',
      'dependencies': [
        'openssl/openssl.gyp:openssl',
      ],
      'sources': [
          'libssh2/include/libssh2.h',
          'libssh2/include/libssh2_publickey.h',
          'libssh2/include/libssh2_sftp.h',
          'libssh2/src/agent.c',
          'libssh2/src/channel.c',
          'libssh2/src/channel.h',
          'libssh2/src/comp.c',
          'libssh2/src/comp.h',
          'libssh2/src/crypt.c',
          'libssh2/src/crypto.h',
          'libssh2/src/global.c',
          'libssh2/src/hostkey.c',
          'libssh2/src/keepalive.c',
          'libssh2/src/kex.c',
          'libssh2/src/knownhost.c',
          'libssh2/src/libssh2_priv.h',
          'libssh2/src/mac.c',
          'libssh2/src/mac.h',
          'libssh2/src/misc.c',
          'libssh2/src/misc.h',
          'libssh2/src/openssl.c',
          'libssh2/src/openssl.h',
          'libssh2/src/packet.c',
          'libssh2/src/packet.h',
          'libssh2/src/pem.c',
          'libssh2/src/publickey.c',
          'libssh2/src/scp.c',
          'libssh2/src/session.c',
          'libssh2/src/session.h',
          'libssh2/src/sftp.c',
          'libssh2/src/sftp.h',
          'libssh2/src/transport.c',
          'libssh2/src/transport.h',
          'libssh2/src/userauth.c',
          'libssh2/src/userauth.h',
          'libssh2/src/version.c',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'libssh2/include',
          '<(SHARED_INTERMEDIATE_DIR)',
        ]
      },
      'include_dirs': [
          'libssh2/src',
          'libssh2/include',
        '<(SHARED_INTERMEDIATE_DIR)',
      ],
	  'defines': [ 'LIBSSH2_OPENSSL' ],
      'conditions': [
  			['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
  			  'cflags': [ '--std=c89' ],
  			  'defines': [ '_GNU_SOURCE' ]
  			}],
  			['OS=="win"', {
  			  'sources': [
  				 'libssh2/win32/libssh2_config.h',
  			  ],      
  			  'include_dirs': [
                 'libssh2/win32/',
  			  ],
  			  'defines': [ 'ONIG_EXTERN=extern' ],
              'direct_dependent_settings': {
                'include_dirs': [ 'libssh2/win32/' ]
              },
  			}],
      ],
    }, # end libssh2
    {
      'target_name': 'libssh2_test',
      'type': 'executable',
      'sources': [ 'libssh2/test/simple.c' ],
      'dependencies': [
        'libssh2'
      ],
      'conditions': [
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
          'cflags': [ '--std=c89' ],
          'defines': [ '_GNU_SOURCE' ]
        }],
      ],
    }, # end libssh2_test		
  ] # end targets
}
