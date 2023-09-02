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
            if ( ( _errorCode ) && ( _errorCode.value() != 2 ) ) {
                errorCode_ = _errorCode;

                fmt::print( fmt::emphasis::bold | fg( fmt::color::red ),
                            "session::doRead has reported {} error code\n",
                            errorCode_.value() );
                fmt::print( fg( fmt::color::white ), "{}\n",
                            errorCode_.message() );

            } else {
                parseRequest();

                if ( requestHeaders_.count( "Content-Length" ) ) {
                    if ( std::stoul( requestHeaders_.at( "Content-Length" ) ) >
                         _length ) {
                        wholeData_ += data_;

                        return;
                    }

                } else {
                    fmt::print( fmt::emphasis::bold | fg( fmt::color::red ),
                                "Request header has no \"Content-Length\" "
                                "attribute\n" );
                    fmt::print( fg( fmt::color::white ), "\n" );

                    return;
                }

                if ( !wholeData_.empty() ) {
                    memset( &( data_[ 0 ] ), 0, sizeof( data_ ) );
                    strncpy( data_, wholeData_.c_str(), sizeof( data_ ) );
                    data_[ sizeof( data_ ) - 1 ] = 0;

                    wholeData_.clear();
                }

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
    auto openProcessToString = []( char** _responsePointer, char* _bashCommand,
                                   ... ) {
        int l_exitCode;
        char l_buffer[ MAX_LENGTH ] = "unknown";
        char* l_bashCommand = NULL;
        FILE* l_pipeDescriptor = NULL;
        va_list l_variableArgumentListIterator;

        va_start( l_variableArgumentListIterator, _bashCommand );
        l_exitCode = vasprintf( &l_bashCommand, _bashCommand,
                                l_variableArgumentListIterator );
        va_end( l_variableArgumentListIterator );

        if ( !l_exitCode ) {
            goto ERROR;
        }

        if ( ( l_pipeDescriptor = popen( l_bashCommand, "r" ) ) == NULL ) {
            goto ERROR;
        }

        if ( fgets( l_buffer, MAX_LENGTH, l_pipeDescriptor ) == NULL ) {
            goto ERROR;
        }

        l_buffer[ strlen( l_buffer ) - 1 ] = '\0';

        l_exitCode = asprintf( _responsePointer, "%s", l_buffer );

        if ( !l_exitCode ) {
            goto ERROR;
        }

        free( l_bashCommand );

        return ( pclose( l_pipeDescriptor ) );

    ERROR:
        free( l_bashCommand );

        return ( ( l_pipeDescriptor == NULL )
                     ? ( 1 )
                     : ( 2 + pclose( l_pipeDescriptor ) ) );
    };

    std::error_code l_exitCode;

    char* l_response;

    if ( openProcessToString(
             &l_response, const_cast< char* >(
                              ( "curl -s https://" + hostAddress_ + _GETQuery )
                                  .c_str() ) ) ) {
        l_exitCode.assign( EADDRNOTAVAIL, std::generic_category() );
    }

    if ( l_exitCode ) {
        fmt::print( fmt::emphasis::bold | fg( fmt::color::red ),
                    "session::sendGETRequest has reported {} error code\n",
                    l_exitCode.value() );
        fmt::print( fg( fmt::color::white ), "{}\n", l_exitCode.message() );
    }

    _response = l_response;

    memset( &( data_[ 0 ] ), 0, sizeof( data_ ) );
    strncpy( data_, l_response, sizeof( data_ ) );
    data_[ sizeof( data_ ) - 1 ] = 0;

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

    memset( &( data_[ 0 ] ), 0, sizeof( data_ ) );
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
