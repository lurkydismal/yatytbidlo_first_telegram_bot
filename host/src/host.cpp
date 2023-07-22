#include <asio.hpp>
#include <host.hpp>
#include <session.hpp>

using asio::ip::tcp;

host::host( asio::io_context& _ioContext, short _port )
    : acceptor_( _ioContext, tcp::endpoint( tcp::v4(), _port ) ) {
    doAccept();
}

void host::doAccept( void ) {
    acceptor_.async_accept(
        [ this ]( std::error_code _errorCode, tcp::socket _socket ) {
            if ( !_errorCode ) {
                std::make_shared< session >( std::move( _socket ) )->start();
            }

            doAccept();
        } );
}
