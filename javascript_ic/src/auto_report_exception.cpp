#include <auto_report_exception.hpp>
#include <fmt/core.h>
#include <js/Conversions.h>
#include <jsapi.h>
#include <mozilla/Unused.h>

AutoReportException::AutoReportException( JSContext* _context )
    : context_( _context ) {}

AutoReportException::~AutoReportException() {
    if ( !JS_IsExceptionPending( context_ ) ) {
        return;
    }

    JS::RootedValue l_exception( context_ );
    mozilla::Unused << JS_GetPendingException( context_, &l_exception );
    JS_ClearPendingException( context_ );

    JS::RootedString l_message( context_,
                                JS::ToString( context_, l_exception ) );

    if ( !l_message ) {
        fmt::print( stderr,
                    "(could not convert thrown exception to string)\n" );

    } else {
        JS::UniqueChars l_messageAsUTF8(
            JS_EncodeStringToUTF8( context_, l_message ) );

        fmt::print( stderr, "{}\n", l_messageAsUTF8.get() );
    }

    JS_ClearPendingException( context_ );
}