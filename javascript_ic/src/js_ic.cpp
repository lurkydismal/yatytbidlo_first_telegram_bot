#include <auto_report_exception.hpp>
#include <fmt/core.h>
#include <js/CompilationAndEvaluation.h>
#include <js/Conversions.h>
#include <js/Exception.h>
#include <js/Initialization.h>
#include <js/SourceText.h>
#include <js_ic.hpp>
#include <jsapi.h>
#include <mozilla/Unused.h>
#include <std.hpp>

#include <fstream>

std::error_code JavaScriptIC::throwError( JSContext* _context,
                                          JS::HandleObject _global,
                                          const char* _message,
                                          const char* _fileName,
                                          int32_t _lineNumber ) {
    std::error_code l_exitCode;

    JS::RootedString l_messageJSString(
        _context, JS_NewStringCopyZ( _context, _message ) );

    if ( !l_messageJSString ) {
        l_exitCode.assign( EACCES, std::generic_category() );

        goto EXIT;
    }

    {
        JS::RootedString l_fileNameJSString(
            _context, JS_NewStringCopyZ( _context, _fileName ) );

        if ( !l_fileNameJSString ) {
            l_exitCode.assign( EACCES, std::generic_category() );

            goto EXIT;
        }

        JS::RootedValueArray< 3 > l_jsThrowArguments( _context );
        l_jsThrowArguments[ 0 ].setString( l_messageJSString );
        l_jsThrowArguments[ 1 ].setString( l_fileNameJSString );
        l_jsThrowArguments[ 2 ].setInt32( _lineNumber );

        // The JSAPI code here is actually simulating `throw Error(_message)`
        // without the new, as new is a bit harder to simulate using the JSAPI.
        // In this case,
        // unless the script has redefined Error, it amounts to the same thing.
        JS::RootedValue l_exception( _context );

        if ( !JS_CallFunctionName( _context, _global, "Error",
                                   l_jsThrowArguments, &l_exception ) ) {
            l_exitCode.assign( EADDRNOTAVAIL, std::generic_category() );

            goto EXIT;
        }

        JS_SetPendingException( _context, l_exception );
    }

EXIT:
    return ( l_exitCode );
}

JSObject* JavaScriptIC::createGlobal( JSContext* _context ) {
    JS::RealmOptions options;

    static JSClass BoilerplateGlobalClass = {
        "BoilerplateGlobal", JSCLASS_GLOBAL_FLAGS, &JS::DefaultGlobalClassOps };

    return ( JS_NewGlobalObject( _context, &BoilerplateGlobalClass, nullptr,
                                 JS::FireOnNewGlobalHook, options ) );
}

std::error_code JavaScriptIC::reportAndClearException( JSContext* _context ) {
    std::error_code l_exitCode;

    JS::ExceptionStack l_exceptionStack( _context );

    if ( !JS::StealPendingExceptionStack( _context, &l_exceptionStack ) ) {
        fmt::print(
            stderr,
            "Uncatchable exception thrown, out of memory or something" );

        l_exitCode.assign( EADDRNOTAVAIL, std::generic_category() );
    }

    JS::ErrorReportBuilder l_reportBuilder( _context );

    if ( !l_reportBuilder.init( _context, l_exceptionStack,
                                JS::ErrorReportBuilder::WithSideEffects ) ) {
        fmt::print( stderr, "Couldn't build error report" );

        l_exitCode.assign( EACCES, std::generic_category() );
    }

    if ( !l_exitCode ) {
        JS::PrintError( stderr, l_reportBuilder, false );
    }

    return ( l_exitCode );
}

bool JavaScriptIC::defineFunctions( JSContext* _context,
                                    JS::Handle< JSObject* > _global ) {
    return ( JS_DefineFunctions( _context, _global, GlobalFunctions ) );
}

bool JavaScriptIC::callGlobalFunction( JSContext* _context,
                                       JS::HandleObject _global,
                                       std::string _functionName,
                                       JS::HandleValueArray _arguments,
                                       JS::RootedValue& _returnValue ) {
    return ( JS_CallFunctionName( _context, _global, _functionName.c_str(),
                                  _arguments, &_returnValue ) );
}

bool JavaScriptIC::executeFileByName( JSContext* _context,
                                      std::string _fileName ) {
    JS::RootedValue l_returnValue( _context );
    JS::CompileOptions l_options( _context );

    l_options.setFile( _fileName.c_str() );

    return ( EvaluateUtf8Path( _context, l_options, _fileName.c_str(),
                               &l_returnValue ) );
}

std::error_code JavaScriptIC::executeCode( JSContext* _context,
                                           std::string _fileName,
                                           uint32_t _lineNumber,
                                           std::string _code ) {
    std::error_code l_exitCode;

    JS::RootedValue l_returnValue( _context );

    JS::CompileOptions l_options( _context );
    l_options.setFileAndLine( _fileName.c_str(), _lineNumber );

    JS::SourceText< mozilla::Utf8Unit > l_source;

    if ( !l_source.init( _context, _code.c_str(), _code.length(),
                         JS::SourceOwnership::Borrowed ) ) {
        l_exitCode.assign( EBADMSG, std::generic_category() );

    } else if ( !JS::Evaluate( _context, l_options, l_source,
                               &l_returnValue ) ) {
        l_exitCode.assign( EACCES, std::generic_category() );
    }

    return ( l_exitCode );
}
