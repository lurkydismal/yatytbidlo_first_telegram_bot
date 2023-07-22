#include <asio.hpp>
#include <fmt/core.h>
#include <host.hpp>

#include <cerrno>

int main( int _argumentCount, char* _argumentVector[] ) {
    try {
        if ( _argumentCount != 2 ) {
            fmt::print( stderr, "Usage: {} <port>\n", _argumentVector[ 0 ] );

            return ( EOPNOTSUPP );
        }

        asio::io_context l_ioContext;

        host l_host( l_ioContext, std::atoi( _argumentVector[ 1 ] ) );

        l_ioContext.run();

    } catch ( std::exception& _exception ) {
        fmt::print( stderr, "Exception: {}\n ", _exception.what() );
    }

    return ( 0 );
}
