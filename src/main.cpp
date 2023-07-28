/**
 * @file main.cpp
 * @author lurkydismal (lurkydismal@duck.com)
 * @brief Program starting point
 * @version 1.0
 * @date 2023-07-28
 *
 */

#include <asio.hpp>
#include <fmt/core.h>
#include <host.hpp>

#include <cerrno>
#include <random>

/**
 * @brief Program starting point
 *
 * @param _argumentCount Arguments count
 * @param _argumentVector Vector with program launch option arguments. Pass 0 as
 * random port
 * @return int Program exit code
 */
int main( int _argumentCount, char* _argumentVector[] ) {
    std::error_code l_exitCode;

    try {
        uint16_t l_portNumber =
            ( _argumentCount == 2 ) ? std::atoi( _argumentVector[ 1 ] ) : 0;

        if ( ( _argumentCount != 2 ) ||
             ( l_portNumber != std::atoi( _argumentVector[ 1 ] ) ) ) {
            fmt::print( stderr, "Usage: {} <port> or 0\n",
                        _argumentVector[ 0 ] );

            l_exitCode.assign( EINVAL, std::generic_category() );

            throw( std::invalid_argument( l_exitCode.message() ) );
        }

        if ( l_portNumber == 0 ) {
            std::default_random_engine l_generator(
                std::chrono::system_clock::now().time_since_epoch().count() );

            std::uniform_int_distribution< uint16_t > l_distribution( 1000,
                                                                      65535 );

            l_portNumber = l_distribution( l_generator );
        }

        fmt::print( "Launch using port {}\n ", l_portNumber );

        asio::io_context l_ioContext;

        host l_host( l_ioContext, l_portNumber );

        l_ioContext.run();

    } catch ( std::invalid_argument const& _exception ) {
        fmt::print( stderr, "Exception: {}\n ", _exception.what() );
    }

    return ( l_exitCode.value() );
}
