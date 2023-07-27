#include <asio.hpp>
#include <auto_report_exception.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <js/Initialization.h>
#include <js_ic.hpp>
#include <session.hpp>

#include <thread>

using asio::ip::tcp;

static std::error_code onReceiveRoutine( JSContext* _context,
                                         std::string _fileName,
                                         const std::string& _response ) {
    std::error_code l_exitCode;

    JS::RootedObject l_global( _context,
                               JavaScriptIC::createGlobal( _context ) );

    if ( !l_global ) {
        l_exitCode.assign( EADDRNOTAVAIL, std::generic_category() );

        goto EXIT;
    }

    {
        JSAutoRealm l_autoRealm( _context, l_global );

        if ( !JavaScriptIC::defineFunctions( _context, l_global ) ) {
            goto EXIT;
        }

        AutoReportException l_autoReport( _context );

        l_exitCode = JavaScriptIC::executeFileByName( _context, _fileName );

        if ( static_cast< bool >( l_exitCode ) ) {
            JavaScriptIC::reportAndClearException( _context );

            goto EXIT;
        }

        {
            JS::RootedValue l_returnValue( _context );
            JS::RootedValue l_response( _context );
            JS::RootedString l_responseAsJSString(
                _context, JS_NewStringCopyZ( _context, _response.c_str() ) );

            if ( !l_responseAsJSString ) {
                l_exitCode.assign( EACCES, std::generic_category() );

                goto EXIT;
            }

            l_response.setString( l_responseAsJSString );

            JavaScriptIC::callGlobalFunction(
                _context, l_global, "receiveRoutine",
                JS::HandleValueArray( l_response ), l_returnValue );
        }
    }

EXIT:

    return ( l_exitCode );
}

session::session( tcp::socket _socket ) : socket_( std::move( _socket ) ) {}

void session::start( void ) {
    doRead();
}

void session::doRead( void ) {
    auto self( shared_from_this() );

    socket_.async_read_some(
        asio::buffer( data_, MAX_LENGTH ),
        [ this, self ]( std::error_code _errorCode, std::size_t _length ) {
            if ( _errorCode ) {
                errorCode_ = _errorCode;
            } else {
                fmt::print( fmt::emphasis::bold | fg( fmt::color::green ),
                            "{}\n", data_ );
                fmt::print( fg( fmt::color::white ), "\n" );

                if ( !JS_Init() ) {
                    errorCode_.assign( EADDRNOTAVAIL, std::generic_category() );

                    goto EXIT;
                }

                {
                    JSContext* l_context =
                        JS_NewContext( JS::DefaultHeapMaxBytes );

                    if ( !l_context ) {
                        errorCode_.assign( EADDRNOTAVAIL,
                                           std::generic_category() );

                        goto DESTROY_CONTEXT_AND_SHUTDOWN;
                    }

                    if ( !JS::InitSelfHostedCode( l_context ) ) {
                        errorCode_.assign( EACCES, std::generic_category() );
                    }

                    errorCode_ =
                        onReceiveRoutine( l_context, fileName_, data_ );

                DESTROY_CONTEXT_AND_SHUTDOWN:

                    JS_DestroyContext( l_context );
                    JS_ShutDown();
                }

            EXIT:

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
            } else {
                // fmt::print( fmt::emphasis::bold | fg( fmt::color::blue ),
                //             "{}\n", data_ );
                // fmt::print( fg( fmt::color::white ), "\n" );

                doRead();
            }
        } );
}
