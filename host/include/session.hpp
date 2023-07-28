/**
 * @file session.hpp
 * @author lurkydismal (lurkydismal@duck.com)
 * @brief Asynchronous accept executor
 * @version 1.0
 * @date 2023-07-28
 *
 */

#pragma once

#include <asio.hpp>
#include <js_ic.hpp>

using asio::ip::tcp;

/**
 * @brief Asynchronous accept executor
 *
 */
class session : public std::enable_shared_from_this< session > {
public:
    /**
     * @brief Construct a new session object
     *
     * @param _socket The TCP socket type
     */
    explicit session( tcp::socket _socket );

    /**
     * @brief Begin execute sockets
     *
     */
    void start( void );

private:
    /**
     * @brief Start an asynchronous read
     *
     */
    void doRead( void );

    /**
     * @brief Start an asynchronous operation to write all of the supplied data
     * to a stream
     *
     * @param _length Amount of data
     */
    void doWrite( std::size_t _length );

private:
    /**
     * @brief Session exit code
     *
     */
    std::error_code errorCode_;

    /**
     * @brief The TCP socket type
     *
     */
    tcp::socket socket_;

    /**
     * @brief JS to execute in answer to read
     *
     */
    const std::string fileName_ = "main.js";

    /**
     * @brief Max read lenght
     *
     */
    enum { MAX_LENGTH = 1024 };

    /**
     * @brief Array to store read data
     *
     */
    char data_[ MAX_LENGTH ];
};
