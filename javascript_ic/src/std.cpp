#include <asio.hpp>
#include <fmt/core.h>
#include <js/Conversions.h>
#include <js_ic.hpp>
#include <jsapi.h>
#include <std.hpp>

bool printJS( JSContext* _context,
              uint32_t _argumentCount,
              JS::Value* _valuePointer ) {
    std::error_code l_exitCode;

    JS::CallArgs l_arguments =
        JS::CallArgsFromVp( _argumentCount, _valuePointer );
    JS::RootedObject l_global(
        _context, JS::GetNonCCWObjectGlobal( &l_arguments.callee() ) );

    JS::Rooted< JS::Value > l_argument( _context, l_arguments.get( 0 ) );
    JS::Rooted< JSString* > l_argumentAsJSString(
        _context, JS::ToString( _context, l_argument ) );

    if ( !l_argumentAsJSString ) {
        l_exitCode.assign( EACCES, std::generic_category() );

        JS_THROW_ERROR( _context, l_global, "Permission denied" );
    }

    JS::UniqueChars l_argumentAsString =
        JS_EncodeStringToUTF8( _context, l_argumentAsJSString );

    fmt::print( stderr, "{}\n", l_argumentAsString.get() );

    l_arguments.rval().setUndefined();

    errno = l_exitCode.value();

    return ( !static_cast< bool >( l_exitCode ) );
}
