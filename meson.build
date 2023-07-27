project(
    'first_telegram_bot',
    'cpp',
    version: 'v1.0.0',
    default_options: [ 'cpp_std=c++17', 'warning_level=3' ]
)

cxx = meson.get_compiler( 'cpp' )

argument_list = []

asio = dependency( 'asio' )
fmt = dependency( 'fmt' )
readline = cxx.find_library( 'readline' )
spidermonkey = dependency( 'mozjs-102' )
zlib = dependency( 'zlib' )  # (is already a SpiderMonkey dependency)

javascript_ic_include = include_directories( 'javascript_ic/include' )
host_include = include_directories( 'host/include' )

# Check if SpiderMonkey was compiled with --enable-debug. If this is the case,
# you must compile all your sources with -DDEBUG=1.
# See https://bugzilla.mozilla.org/show_bug.cgi?id=1261161
nondebug_spidermonkey = cxx.compiles(
'''
#include <js-config.h>
#ifdef JS_DEBUG
#error debug yes, if we did not already error out due to DEBUG not being defined
#endif
''',
    args: [ argument_list ], dependencies: [ spidermonkey ],
    name: 'SpiderMonkey is a non-debug build'
)

if not nondebug_spidermonkey
    argument_list += '-DDEBUG=1'
endif

# Check if a minimal SpiderMonkey program compiles, links, and runs. If not,
# it's most likely the case that SpiderMonkey was configured incorrectly, for
# example by building mozglue as a shared library.
minimal_program = cxx.run(
    '''
#include <js/Initialization.h>
int main( void ) {
    uint8_t l_exitCode = 0;

    if ( !JS_Init() ) {
        l_exitCode = 1;
    }

    if ( l_exitCode == 0 ) {
        JS_ShutDown();
    }

    return ( l_exitCode );
}
''',
    args: [ argument_list ], dependencies: [ spidermonkey ],
    name: 'SpiderMonkey sanity check'
)

if not minimal_program.compiled() or minimal_program.returncode() != 0
    error( '''
A minimal SpiderMonkey program could not be compiled, linked, or run. Most
likely you should build it with a different configuration. Check the recommended
configuration in this repository.''' )
endif

add_project_arguments( argument_list, language: 'cpp' )

if cxx.get_id() == 'gcc' or cxx.get_id() == 'clang'
    test_warning_argument_list = [
        # It's useful not to have to initialize all struct fields when defining a
        # JSClass, since some of them are padding.
        '-Wno-missing-field-initializers',

        # We use argument names in some cases for explanatory purposes
        '-Wno-unused-parameter',
    ]
else
    test_warning_argument_list = []
endif

add_project_arguments(
    cxx.get_supported_arguments( test_warning_argument_list ),
    language: 'cpp'
)

javascript_ic = declare_dependency(
    compile_args : argument_list + test_warning_argument_list,
    dependencies : [ fmt, spidermonkey, zlib ],
    include_directories : [ javascript_ic_include ],
    sources : [ 'javascript_ic/src/js_ic.cpp', 'javascript_ic/src/auto_report_exception.cpp', 'javascript_ic/src/std.cpp' ],
    version : '1.0.0'
)
host = declare_dependency(
    compile_args : argument_list + test_warning_argument_list,
    dependencies : [ asio, fmt, spidermonkey, readline ],
    include_directories : [ javascript_ic_include, host_include ],
    sources : [ 'host/src/host.cpp', 'host/src/session.cpp' ],
    version : '1.0.0'
)

executable(
    'main',
    'src/main.cpp',
    dependencies : [ asio, fmt, spidermonkey, zlib, readline, javascript_ic, host ],
    include_directories : [ javascript_ic_include, host_include ]
)

configure_file(
    input : 'src/main.js',
    output : 'main.js',
    configuration : configuration_data()
)