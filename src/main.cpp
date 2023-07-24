#include <asio.hpp>
#include <fmt/core.h>
#include <host.hpp>

#include <cerrno>

int main( int _argumentCount, char* _argumentVector[] ) {
    std::error_code l_exitCode;
    asio::io_context l_ioContext;
    uint32_t l_portNumber;

    try {
        if ( _argumentCount != 2 ) {
            fmt::print( stderr, "Usage: {} <port>\n", _argumentVector[ 0 ] );

            l_exitCode.assign( ENOTSUP, std::generic_category() );

            throw( std::runtime_error( l_exitCode.message() ) );
        }

        l_portNumber = std::atoi( _argumentVector[ 1 ] );

        host l_host( l_ioContext, l_portNumber );

        l_ioContext.run();

    } catch ( std::exception& _exception ) {
        fmt::print( stderr, "Exception: {}\n ", _exception.what() );
    }

    return ( l_exitCode.value() );
}
