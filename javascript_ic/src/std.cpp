#include <fmt/core.h>
#include <js/Conversions.h>
#include <jsapi.h>
#include <std.hpp>

bool printJS( JSContext* _context,
              unsigned _argumentCount,
              JS::Value* _valuePointer ) {
    JS::CallArgs l_arguments =
        JS::CallArgsFromVp( _argumentCount, _valuePointer );

    JS::Rooted< JS::Value > l_argument( _context, l_arguments.get( 0 ) );
    JS::Rooted< JSString* > l_argumentAsJSString(
        _context, JS::ToString( _context, l_argument ) );

    if ( !l_argumentAsJSString ) {
        return ( false );
    }

    JS::UniqueChars l_argumentAsString =
        JS_EncodeStringToUTF8( _context, l_argumentAsJSString );

    fmt::print( stderr, "{}\n", l_argumentAsString.get() );

    l_arguments.rval().setUndefined();

    return ( true );
}
