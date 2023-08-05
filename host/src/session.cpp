#include <asio.hpp>
#include <asio/ssl.hpp>
#include <auto_report_exception.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <js/Conversions.h>
#include <js/Initialization.h>
#include <js_ic.hpp>
#include <session.hpp>

#include <mutex>
#include <thread>
#include <vector>

namespace ssl = asio::ssl;
using asio::ip::tcp;

namespace {

std::mutex g_JSThreadMutex;

};

session::session( tcp::socket _socket ) : socket_( std::move( _socket ) ) {}

session::~session() {
    for ( std::thread& _JSThread : JSThreads_ ) {
        if ( _JSThread.joinable() ) {
            _JSThread.join();
        }
    }
}

void session::start( void ) {
    doRead();
}

void session::doRead( void ) {
    auto self( shared_from_this() );

    socket_.async_read_some(
        asio::buffer( data_, MAX_LENGTH ),
        [ this, self ]( std::error_code _errorCode, std::size_t _length ) {
            auto l_stripJSON = [ this ]( size_t _receivedLength ) {
                fmt::print( "{}={}\n", data_, _receivedLength );
            };

            if ( ( _errorCode ) && ( _errorCode.value() != 2 ) ) {
                errorCode_ = _errorCode;

                fmt::print( fmt::emphasis::bold | fg( fmt::color::red ),
                            "session::doRead has reported {} error code\n",
                            errorCode_.value() );
                fmt::print( fg( fmt::color::white ), "{}\n",
                            errorCode_.message() );

            } else {
                parseRequest();

                l_stripJSON( _length );

                fmt::print( fmt::emphasis::bold | fg( fmt::color::green ),
                            "{}\n", data_ );
                fmt::print( fg( fmt::color::white ), "\n" );

                JSThreads_.emplace_back( std::thread( [ data_ =
                                                            std::move( data_ ),
                                                        fileName_ = std::move(
                                                            fileName_ ),
                                                        this ]( void ) {
                    std::error_code l_exitCode;
                    uint32_t l_errorLineNumber;

                    const std::lock_guard< std::mutex > l_JSThreadMutexLock(
                        g_JSThreadMutex );

                    if ( !JS_Init() ) {
                        l_exitCode.assign( EADDRNOTAVAIL,
                                           std::generic_category() );

                        l_errorLineNumber = __LINE__;

                        goto EXIT;
                    }

                    {
                        JSContext* l_context =
                            JS_NewContext( JS::DefaultHeapMaxBytes );

                        if ( !l_context ) {
                            l_exitCode.assign( EADDRNOTAVAIL,
                                               std::generic_category() );

                            l_errorLineNumber = __LINE__;

                            goto DESTROY_CONTEXT_AND_SHUTDOWN;
                        }

                        if ( !JS::InitSelfHostedCode( l_context ) ) {
                            l_exitCode.assign( EACCES,
                                               std::generic_category() );

                            l_errorLineNumber = __LINE__;

                            goto DESTROY_CONTEXT_AND_SHUTDOWN;
                        }

                        {
                            JS::RootedObject l_global(
                                l_context,
                                JavaScriptIC::createGlobal( l_context ) );

                            if ( !l_global ) {
                                l_exitCode.assign( EADDRNOTAVAIL,
                                                   std::generic_category() );

                                l_errorLineNumber = __LINE__;

                                goto DESTROY_CONTEXT_AND_SHUTDOWN;
                            }

                            {
                                JSAutoRealm l_autoRealm( l_context, l_global );

                                if ( !JavaScriptIC::defineFunctions(
                                         l_context, l_global ) ) {
                                    l_exitCode.assign(
                                        EADDRNOTAVAIL,
                                        std::generic_category() );

                                    l_errorLineNumber = __LINE__;

                                    goto DESTROY_CONTEXT_AND_SHUTDOWN;
                                }

                                AutoReportException l_autoReport( l_context );

                                if ( !JavaScriptIC::executeFileByName(
                                         l_context, fileName_ ) ) {
                                    l_exitCode =
                                        JavaScriptIC::reportAndClearException(
                                            l_context );

                                    l_errorLineNumber = __LINE__;

                                    goto DESTROY_CONTEXT_AND_SHUTDOWN;
                                }

                                {
                                    JS::RootedValue l_returnValue( l_context );
                                    JS::RootedValue l_response( l_context );

                                    JS::RootedString l_responseAsJSString(
                                        l_context,
                                        JS_NewStringCopyZ( l_context, data_ ) );

                                    if ( !l_responseAsJSString ) {
                                        l_exitCode.assign(
                                            EACCES, std::generic_category() );

                                        l_errorLineNumber = __LINE__;

                                        goto DESTROY_CONTEXT_AND_SHUTDOWN;
                                    }

                                    l_response.setString(
                                        l_responseAsJSString );

                                    JavaScriptIC::callGlobalFunction(
                                        l_context, l_global, "onReceiveRoutine",
                                        JS::HandleValueArray( l_response ),
                                        l_returnValue );

                                    if ( l_returnValue.isUndefined() ) {
                                        l_exitCode.assign(
                                            EADDRNOTAVAIL,
                                            std::generic_category() );

                                        l_errorLineNumber = __LINE__;

                                        goto DESTROY_CONTEXT_AND_SHUTDOWN;
                                    }

                                    JS::Rooted< JSString* >
                                        l_returnValueAsJSString(
                                            l_context,
                                            JS::ToString( l_context,
                                                          l_returnValue ) );

                                    if ( !l_returnValueAsJSString ) {
                                        l_exitCode.assign(
                                            EACCES, std::generic_category() );

                                        l_errorLineNumber = __LINE__;

                                        JS_THROW_ERROR( l_context, l_global,
                                                        "Permission denied" );
                                    }

                                    JS::UniqueChars l_returnValueAsString =
                                        JS_EncodeStringToUTF8(
                                            l_context,
                                            l_returnValueAsJSString );

                                    std::string l_onReceiveRoutineReturnValue =
                                        l_returnValueAsString.get();

                                    size_t l_delimeterIndex;

                                    std::string l_GETQuery;
                                    l_delimeterIndex =
                                        l_onReceiveRoutineReturnValue.find(
                                            ',' );

                                    if ( l_delimeterIndex ==
                                         std::string::npos ) {
                                        l_exitCode.assign(
                                            ESPIPE, std::generic_category() );

                                        l_errorLineNumber = __LINE__;

                                        goto DESTROY_CONTEXT_AND_SHUTDOWN;
                                    }

                                    l_GETQuery =
                                        l_onReceiveRoutineReturnValue.substr(
                                            0, l_delimeterIndex );
                                    l_onReceiveRoutineReturnValue.erase(
                                        0, l_delimeterIndex + 1 );

                                    std::string l_callbackFunctionName =
                                        l_onReceiveRoutineReturnValue;

                                    std::string l_GETQueryResponse;

                                    l_exitCode = sendGETRequest(
                                        l_GETQuery, l_GETQueryResponse );

                                    l_errorLineNumber = __LINE__;

                                    if ( ( l_exitCode ) ||
                                         ( l_GETQueryResponse.empty() ) ) {
                                        l_errorLineNumber = __LINE__;

                                        goto DESTROY_CONTEXT_AND_SHUTDOWN;
                                    }

                                    l_responseAsJSString = JS::RootedString(
                                        l_context,
                                        JS_NewStringCopyZ( l_context, data_ ) );

                                    if ( !l_responseAsJSString ) {
                                        l_exitCode.assign(
                                            EACCES, std::generic_category() );

                                        l_errorLineNumber = __LINE__;

                                        goto DESTROY_CONTEXT_AND_SHUTDOWN;
                                    }

                                    l_response.setString(
                                        l_responseAsJSString );

                                    JavaScriptIC::callGlobalFunction(
                                        l_context, l_global,
                                        l_callbackFunctionName.c_str(),
                                        JS::HandleValueArray( l_response ),
                                        l_returnValue );
                                }
                            }
                        }

                    DESTROY_CONTEXT_AND_SHUTDOWN:

                        JS_DestroyContext( l_context );
                        JS_ShutDown();
                    }
                EXIT:

                    if ( l_exitCode ) {
                        errorCode_ = l_exitCode;

                        fmt::print(
                            fmt::emphasis::bold | fg( fmt::color::red ),
                            "thread has reported {} error code on {} line\n",
                            l_exitCode.value(), l_errorLineNumber );
                        fmt::print( fg( fmt::color::white ), "{}\n",
                                    l_exitCode.message() );
                    }
                } ) );

                doWrite( _length );
            }
        } );
}

void session::doWrite( std::size_t _length ) {
    auto self( shared_from_this() );

    asio::async_write(
        socket_, asio::buffer( data_, _length ),
        [ this, self ]( std::error_code _errorCode, std::size_t /*_length*/ ) {
            if ( _errorCode ) {
                errorCode_ = _errorCode;

                fmt::print( fmt::emphasis::bold | fg( fmt::color::red ),
                            "session::doWrite has reported {}\n",
                            errorCode_.value() );
                fmt::print( fg( fmt::color::white ), "{}\n",
                            errorCode_.message() );

            } else {
                doRead();
            }
        } );
}

std::error_code session::sendGETRequest( std::string _GETQuery,
                                         std::string& _response ) {
    std::error_code l_exitCode;

    asio::io_context l_ioContext;
    const SSL_METHOD* l_rawSSLMethod = SSLv23_client_method();
    SSL_CTX* l_rawSSLContext = SSL_CTX_new( l_rawSSLMethod );

    ssl::context l_SSLContext( l_rawSSLContext );

    ssl::stream< tcp::socket > l_TCPSocketStream( l_ioContext, l_SSLContext );

    tcp::resolver l_TCPResolver( l_ioContext );
    tcp::resolver::query l_queryAddress( hostAddress_, "https" );
    tcp::resolver::iterator l_endpointIterator =
        l_TCPResolver.resolve( l_queryAddress );

    SSL_set_tlsext_host_name( l_TCPSocketStream.native_handle(),
                              hostAddress_.c_str() );

    asio::connect( l_TCPSocketStream.lowest_layer(), l_endpointIterator,
                   l_exitCode );

    if ( l_exitCode ) {
        fmt::print( fmt::emphasis::bold | fg( fmt::color::red ),
                    "session::sendGETRequest has reported {} error code\n",
                    l_exitCode.value() );
        fmt::print( fg( fmt::color::white ), "{}\n", l_exitCode.message() );
    }

    l_TCPSocketStream.handshake( ssl::stream_base::handshake_type::client );

    asio::streambuf request;
    std::ostream request_stream( &request );
    request_stream << "GET " << _GETQuery << " HTTP/1.1\r\n";
    request_stream << "Host: " << hostAddress_ << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    asio::write( l_TCPSocketStream, request );

    std::string response;

    std::error_code l_temporaryErrorCode;

    while ( !l_temporaryErrorCode ) {
        char l_buffer[ MAX_LENGTH ];
        size_t bytes_transferred = l_TCPSocketStream.read_some(
            asio::buffer( l_buffer ), l_temporaryErrorCode );

        if ( !l_temporaryErrorCode ) {
            response.append( l_buffer, l_buffer + bytes_transferred );
        }
    };

    _response = response;

    memset( &( data_[ 0 ] ), 0, sizeof( data_ ) );
    strncpy( data_, response.c_str(), sizeof( data_ ) );
    data_[ sizeof( data_ ) - 1 ] = 0;

    parseRequest();

    return ( l_exitCode );
}

void session::parseRequest( void ) {
    /* Removes in place any invalid UTF-8 sequences from at most '_length'
     * characters of the string pointed to by '_cString'. (If a NULL byte is
     * encountered, conversion stops.) If the length of the converted string is
     * less than '_length', a NULL byte is inserted. Returns the length of the
     * possibly modified string (with a maximum of '_length'), not including the
     * NULL terminator (if any). Requires that a UTF-8 locale be active; since
     * there is no way to test for this condition, no attempt is made to do so.
     * If the current locale is not UTF-8, behaviour is undefined.
     */
    static auto l_removeNonUTF8 = []( char* _cString, size_t _length ) {
        int l_sequenceLength;
        char* l_firstPassValue = _cString;

        // Skip over the initial correct sequence. Avoid relying on mbtowc
        // returning zero if n is 0, since Posix is not clear whether mbtowc
        // returns 0 or -1.
        while ( ( _length ) &&
                ( ( l_sequenceLength =
                        mbtowc( NULL, l_firstPassValue, _length ) ) > 0 ) ) {
            _length -= l_sequenceLength;
            l_firstPassValue += l_sequenceLength;
        }

        char* l_secondPassValue = l_firstPassValue;

        if ( _length && l_sequenceLength < 0 ) {
            ++l_firstPassValue;
            --_length;

            // If we find an invalid sequence, we need to start shifting
            // correct sequences.
            for ( ; _length; l_firstPassValue += l_sequenceLength,
                             _length -= l_sequenceLength ) {
                l_sequenceLength = mbtowc( NULL, l_firstPassValue, _length );

                // Shift the valid sequence ( if one was found )
                if ( l_sequenceLength > 0 ) {
                    memmove( l_secondPassValue, l_firstPassValue,
                             l_sequenceLength );
                    l_secondPassValue += l_sequenceLength;

                } else if ( l_sequenceLength < 0 ) {
                    l_sequenceLength = 1;

                } else if ( l_sequenceLength == 0 ) {
                    break;
                }
            }

            *l_secondPassValue++ = 0;
        }

        return ( l_secondPassValue - _cString );
    };

    std::istringstream l_dataStringStream( data_ );
    std::istream l_bufferStream( l_dataStringStream.rdbuf() );
    std::string l_line;

    requestHeaders_.clear();

    while ( getline( l_bufferStream, l_line, '\n' ) ) {
        if ( ( l_line.empty() ) || ( l_line == "\r" ) ) {
            break;
        }

        if ( l_line.back() == '\r' ) {
            l_line.resize( l_line.size() - 1 );
        }

        requestHeaders_.insert( parseHeader( l_line ) );
    }

    std::string const l_body(
        std::istreambuf_iterator< char >{ l_bufferStream }, {} );

    strncpy( data_, l_body.c_str(), sizeof( data_ ) );
    data_[ sizeof( data_ ) - 1 ] = 0;

    l_removeNonUTF8( data_, sizeof( data_ ) );
}

std::pair< std::string, std::string > session::parseHeader(
    std::string _header ) {
    std::string l_headerType;
    std::string l_headerContent;
    size_t l_delimeterIndex = _header.find( ':' );

    if ( l_delimeterIndex == std::string::npos ) {
        l_delimeterIndex = _header.find( " / " );

        l_headerType = _header.substr( 0, l_delimeterIndex );
        _header.erase( 0, l_delimeterIndex + 3 );

    } else if ( l_delimeterIndex != std::string::npos ) {
        l_headerType = _header.substr( 0, l_delimeterIndex );
        _header.erase( 0, l_delimeterIndex + 2 );
    }

    l_headerContent = _header;

    return ( std::make_pair( l_headerType, l_headerContent ) );
}
