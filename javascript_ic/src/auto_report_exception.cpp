#include <auto_report_exception.hpp>
#include <fmt/core.h>
#include <js/Conversions.h>
#include <jsapi.h>
#include <mozilla/Unused.h>

AutoReportException::AutoReportException( JSContext* _context )
    : context_( _context ) {}

AutoReportException::~AutoReportException() {
    if ( !JS_IsExceptionPending( context_ ) )
        return;

    JS::RootedValue v_exn( context_ );
    mozilla::Unused << JS_GetPendingException( context_, &v_exn );
    JS_ClearPendingException( context_ );

    JS::RootedString message( context_, JS::ToString( context_, v_exn ) );

    if ( !message ) {
        fmt::print( stderr,
                    "(could not convert thrown exception to string)\n" );

    } else {
        JS::UniqueChars message_utf8(
            JS_EncodeStringToUTF8( context_, message ) );
        fmt::print( stderr, "{}\n", message_utf8.get() );
    }

    JS_ClearPendingException( context_ );
}