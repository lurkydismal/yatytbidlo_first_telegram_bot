/**
 * @file host.hpp
 * @author lurkydismal (lurkydismal@duck.com)
 * @brief Network sockets accepter
 * @version 1.0
 * @date 2023-07-28
 *
 */

#pragma once

#include <asio.hpp>

using asio::ip::tcp;

/**
 * @brief Network sockets accepter
 *
 */
class host {
public:
    /**
     * @brief Construct a new host object
     *
     * @param _ioContext Provides core I/O functionality
     * @param _port Hosting port number
     */
    host( asio::io_context& _ioContext, short _port );

private:
    /**
     * @brief Start an asynchronous accept
     *
     */
    void doAccept( void );

    /**
     * @brief The TCP acceptor type
     *
     */
    tcp::acceptor acceptor_;
};
