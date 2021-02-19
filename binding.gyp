{
    'targets': [
        {
            'target_name': 'addon',
            'sources': ['addon.cc'],
            'include_dirs': [
                '<!@(node -p "require(\'node-addon-api\').include")',
                '<!@(node -e "require(\'nan\')")'
            ],
            'dependencies': ['<!(node -p "require(\'node-addon-api\').gyp")'],
            'cflags!': ['-fno-exceptions'],
            'cflags_cc!': ['-fno-exceptions'],
            'xcode_settings': {
                'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
                'CLANG_CXX_LIBRARY': 'libc++'
            },
            'msvs_settings': {
                'VCCLCompilerTool': {'ExceptionHandling': 1},
            }
        },
    ],
}