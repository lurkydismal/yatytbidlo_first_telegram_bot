#include <asio.hpp>
#include <fmt/core.h>
#include <host.hpp>

#include <cerrno>

int main( int _argumentCount, char* _argumentVector[] ) {
    std::error_code l_exitCode;

    try {
        uint16_t l_portNumber = std::atoi( _argumentVector[ 1 ] );

        if ( ( _argumentCount != 2 ) || ( l_portNumber < 65534 ) ) {
            fmt::print( stderr, "Usage: {} <port>\n", _argumentVector[ 0 ] );

            l_exitCode.assign( ENOTSUP, std::generic_category() );

            throw( std::runtime_error( l_exitCode.message() ) );
        }

        asio::io_context l_ioContext;

        host l_host( l_ioContext, std::atoi( _argumentVector[ 1 ] ) );

        l_ioContext.run();

    } catch ( std::exception& _exception ) {
        fmt::print( stderr, "Exception: {}\n ", _exception.what() );
    }

    return ( l_exitCode.value() );
}
