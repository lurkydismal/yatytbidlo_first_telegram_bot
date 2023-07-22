#pragma once

#include <asio.hpp>

using asio::ip::tcp;

class host {
public:
    host( asio::io_context& _ioContext, short _port );

private:
    void doAccept( void );

    tcp::acceptor acceptor_;
};
