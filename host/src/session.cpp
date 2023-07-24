#include <asio.hpp>
#include <auto_report_exception.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <js/Initialization.h>
#include <js_ic.hpp>
#include <session.hpp>

#include <thread>

using asio::ip::tcp;

static bool onReceiveRoutine( JSContext* _context,
                              std::string _fileName,
                              const std::string& _response ) {
    JS::RootedObject l_global( _context,
                               JavaScriptIC::createGlobal( _context ) );

    if ( !l_global ) {
        return ( false );
    }

    JSAutoRealm ar( _context, l_global );

    JavaScriptIC::defineFunctions( _context, l_global );

    if ( !JavaScriptIC::executeFileByName( _context, _fileName ) ) {
        JavaScriptIC::reportAndClearException( _context );

        return ( false );
    }

    JS::RootedValue l_returnValue( _context );
    JS::RootedValue l_response( _context );
    JS::RootedString l_responseAsJSString(
        _context, JS_NewStringCopyZ( _context, _response.c_str() ) );

    if ( !l_responseAsJSString ) {
        return ( false );
    }

    l_response.setString( l_responseAsJSString );

    JavaScriptIC::callGlobalFunction( _context, l_global, "receiveRoutine",
                                      JS::HandleValueArray( l_response ),
                                      l_returnValue );

    // JavaScriptIC::executeCode( _context, "noname", 1, R"js(
    //     print( `hello world, it is ${new Date()}` );
    // )js" );

    return ( true );
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
            if ( !_errorCode ) {
                fmt::print( fmt::emphasis::bold | fg( fmt::color::green ),
                            "{}\n", data_ );
                fmt::print( fg( fmt::color::white ), "\n" );

                if ( !JS_Init() ) {
                    return;
                }

                JSContext* l_context = JS_NewContext( JS::DefaultHeapMaxBytes );

                if ( !l_context ) {
                    return;
                }

                if ( !JS::InitSelfHostedCode( l_context ) ) {
                    return;
                }

                if ( !onReceiveRoutine( l_context, fileName_, data_ ) ) {
                    return;
                }

                JS_DestroyContext( l_context );
                JS_ShutDown();

                doWrite( _length );
            }
        } );
}

void session::doWrite( std::size_t _length ) {
    auto self( shared_from_this() );

    asio::async_write(
        socket_, asio::buffer( data_, _length ),
        [ this, self ]( std::error_code _errorCode, std::size_t /*_length*/ ) {
            if ( !_errorCode ) {
                // fmt::print( fmt::emphasis::bold | fg( fmt::color::blue ),
                //             "{}\n", data_ );
                // fmt::print( fg( fmt::color::white ), "\n" );

                doRead();
            }
        } );
}
