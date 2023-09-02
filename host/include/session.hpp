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

#include <map>
#include <thread>
#include <vector>

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
     * @brief Destroy the session object
     *
     */
    ~session();

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

    /**
     * @brief
     *
     * @param _GETQuery GET request to send
     * @param _response String to write response into
     * @return std::error_code Exit code
     */
    std::error_code sendGETRequest( std::string _GETQuery,
                                    std::string& _response );

    /**
     * @brief Parse HTTP request
     *
     * @details Does put request headers into requestHeaders_ and request data
     * into data_
     *
     */
    void parseRequest( void );

    /**
     * @brief Parse HTTP header
     *
     * @return std::pair Header type : header content
     */
    std::pair< std::string, std::string > parseHeader( std::string );

private:
    /**
     * @brief Max read length
     *
     */
    enum { MAX_LENGTH = 8164 };

    /**
     * @brief Array to store read data
     *
     */
    char data_[ MAX_LENGTH ];

    /**
     * @brief String to store read data
     *
     */
    std::string wholeData_;

    /**
     * @brief Session exit code
     *
     */
    std::error_code errorCode_;

    /**
     * @brief JS to execute in answer to read
     *
     */
    const std::string fileName_ = "main.js";

    /**
     * @brief HTTP request headers
     *
     */
    std::map< std::string, std::string > requestHeaders_;

    /**
     * @brief The TCP socket type
     *
     */
    tcp::socket socket_;

    /**
     * @brief Host address for connection
     *
     */
    std::string hostAddress_ = "api.telegram.org";

    /**
     * @brief Threads vector where JS runtime happens
     *
     */
    std::vector< std::thread > JSThreads_;
};
