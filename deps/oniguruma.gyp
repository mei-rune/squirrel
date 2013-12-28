{
  'variables': {
  },
  'targets': [
    {
      'target_name': 'oniguruma',
      'type': 'static_library',
      'sources': [
          'oniguruma/oniguruma.h',
          'oniguruma/regcomp.c',
          'oniguruma/regenc.c',
          'oniguruma/regenc.h',
          'oniguruma/regerror.c',
          'oniguruma/regexec.c',
          'oniguruma/regext.c',
          'oniguruma/oniggnu.h',
          'oniguruma/reggnu.c',
          'oniguruma/regint.h',
          'oniguruma/regparse.c',
          'oniguruma/regparse.h',
          'oniguruma/regposerr.c',
          'oniguruma/onigposix.h',
          'oniguruma/regposix.c',
          'oniguruma/regsyntax.c',
          'oniguruma/regtrav.c',
          'oniguruma/regversion.c',
          'oniguruma/st.c',
          'oniguruma/st.h',
          'oniguruma/enc/ascii.c',
          'oniguruma/enc/big5.c',
          'oniguruma/enc/cp1251.c',
          'oniguruma/enc/euc_jp.c',
          'oniguruma/enc/euc_kr.c',
          'oniguruma/enc/euc_tw.c',
          'oniguruma/enc/gb18030.c',
          'oniguruma/enc/iso8859_1.c',
          'oniguruma/enc/iso8859_10.c',
          'oniguruma/enc/iso8859_11.c',
          'oniguruma/enc/iso8859_13.c',
          'oniguruma/enc/iso8859_14.c',
          'oniguruma/enc/iso8859_15.c',
          'oniguruma/enc/iso8859_16.c',
          'oniguruma/enc/iso8859_11.c',
          'oniguruma/enc/iso8859_13.c',
          'oniguruma/enc/iso8859_14.c',
          'oniguruma/enc/iso8859_15.c',
          'oniguruma/enc/iso8859_16.c',
          'oniguruma/enc/iso8859_2.c',
          'oniguruma/enc/iso8859_3.c',
          'oniguruma/enc/iso8859_4.c',
          'oniguruma/enc/iso8859_5.c',
          'oniguruma/enc/iso8859_6.c',
          'oniguruma/enc/iso8859_7.c',
          'oniguruma/enc/iso8859_8.c',
          'oniguruma/enc/iso8859_9.c',
          'oniguruma/enc/koi8.c',
          'oniguruma/enc/koi8_r.c',
          'oniguruma/enc/mktable.c',
          'oniguruma/enc/sjis.c',
          'oniguruma/enc/unicode.c',
          'oniguruma/enc/utf16_be.c',
          'oniguruma/enc/utf16_le.c',
          'oniguruma/enc/utf32_be.c',
          'oniguruma/enc/utf32_le.c',
          'oniguruma/enc/utf8.c',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'oniguruma',
          '<(SHARED_INTERMEDIATE_DIR)',
        ]
      },
      'include_dirs': [
          'oniguruma',
        '<(SHARED_INTERMEDIATE_DIR)',
      ],
      'defines': [ 'RUBY_EXPORT', ],
      'conditions': [
  			['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
  			  'cflags': [ '--std=c89' ],
  			  'defines': [ '_GNU_SOURCE' ]
  			}],
  			['OS=="win"', {
  			  'sources': [
  				 'oniguruma/win32/config.h',
  			  ],      
  			  'include_dirs': [
            'oniguruma/win32/',
  			  ],
  			  'defines': [ 'ONIG_EXTERN=extern' ],
          'direct_dependent_settings': {
            'include_dirs': [ 'oniguruma/win32/' ]
          },
  			}],
      ],
    }, # end oniguruma
    {
      'target_name': 'oniguruma_ctest',
      'type': 'executable',
      'dependencies': [
        'oniguruma'
      ],
      'conditions': [
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
          'cflags': [ '--std=c89' ],
          'defines': [ '_GNU_SOURCE' ]
        }],
        ['OS=="win"', {
          'sources': [
           'oniguruma/win32/config.h',
          ],      
          'include_dirs': [
                  'oniguruma/win32/',
          ],
  	      'defines': [ 'ONIG_EXTERN=extern' ],
          'sources': [ 'oniguruma/win32/testc.c' ],
        }],
      ],
    }, # end oniguruma_ctest
    {
      'target_name': 'oniguruma_utest',
      'type': 'executable',
      'dependencies': [
        'oniguruma'
      ],
      'sources': [
          'oniguruma/testu.c',
      ],
      'conditions': [
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
          'cflags': [ '--std=c89' ],
          'defines': [ '_GNU_SOURCE' ]
        }],
        ['OS=="win"', {
          'sources': [
           'oniguruma/win32/config.h',
          ],      
          'include_dirs': [
            'oniguruma/win32/',
          ],
          'defines': [ 'ONIG_EXTERN=extern' ]
        }],
      ],
    }, # end oniguruma_utest
  ] # end targets
}
