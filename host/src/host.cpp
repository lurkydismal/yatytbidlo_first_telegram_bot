#include <asio.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <host.hpp>
#include <session.hpp>

using asio::ip::tcp;

host::host( asio::io_context& _ioContext, short _port )
    : acceptor_( _ioContext,
                 tcp::endpoint( asio::ip::address_v6::any(), _port ) ) {
    doAccept();
}

void host::doAccept( void ) {
    acceptor_.async_accept( [ this ]( std::error_code _errorCode,
                                      tcp::socket _socket ) {
        if ( ( _errorCode.value() == ENETDOWN ) ||
             ( _errorCode.value() == EPROTO ) ||
             ( _errorCode.value() == ENOPROTOOPT ) ||
             ( _errorCode.value() == EHOSTDOWN ) ||
             ( _errorCode.value() == ENONET ) ||
             ( _errorCode.value() == EHOSTUNREACH ) ||
             ( _errorCode.value() == EOPNOTSUPP ) ||
             ( _errorCode.value() == ENETUNREACH ) ) {
            fmt::print( fmt::emphasis::bold | fg( fmt::color::red ),
                        "host::doAccept has reported {} error code\n",
                        _errorCode.value() );
            fmt::print( fg( fmt::color::white ), "{}\n", _errorCode.message() );

        } else if ( !_errorCode ) {
            std::make_shared< session >( std::move( _socket ) )->start();

        } else {
            throw( std::system_error( _errorCode ) );
        }

        doAccept();
    } );
}
