/**
 * @file js_ic.hpp
 * @author lurkydismal (lurkydismal@duck.com)
 * @brief JavaScript Inter Communication
 * @version 1.0
 * @date 2023-07-28
 *
 */

#pragma once

#include <jsapi.h>

#define JS_THROW_ERROR( _context, _global, _message ) \
    JavaScriptIC::throwError( _context, _global, _message, __FILE__, __LINE__ )

/**
 * @brief JavaScript Inter Communication
 *
 */
namespace JavaScriptIC {

/**
 * @brief A global object is the top-level 'this' value in a script and is
 * required in order to compile or execute JavaScript.
 *
 */
extern const JSClassOps DefaultGlobalClassOps;

/**
 * @brief Throws an error in JS context and scope
 *
 * @details When JS_ReportError creates a new Error object, it sets the fileName
 * and lineNumber properties to the line of JavaScript code currently at the top
 * of the stack. This is usually the line of code that called your native
 * function, so it's usually what you want. JSAPI code can override this by
 * creating the Error object directly and passing additional arguments to the
 * constructor:
 *
 * throw new Error( _message, _fileName, _lineNumber );
 *
 * An example use would be to pass the filename and line number in the C++ code
 * instead:
 *
 * return throwError( _context, _global, _message, __FILE__, __LINE__ );
 *
 *
 * @param _context JS context
 * @param _global JS global
 * @param _message Message
 * @param _fileName File name where error happened
 * @param _lineNumber Line number where error happened
 * @return std::error_code Function exit code
 */
std::error_code throwError( JSContext* _context,
                            JS::HandleObject _global,
                            const char* _message,
                            const char* _fileName,
                            int32_t _lineNumber );

/**
 * @brief Create a Global object
 *
 * @param _context JS context
 * @return JSObject* JS global
 */
JSObject* createGlobal( JSContext* _context );

/**
 * @brief Helper to read current exception and dump to stderr.
 *
 * @note This must be called with a JSAutoRealm (or equivalent) on the stack.
 *
 * @param _context JS context
 * @return std::error_code Error code
 *
 */
std::error_code reportAndClearException( JSContext* _context );

/**
 * @brief Define C/ C++ functions to be called from JS
 *
 * @param _context JS context
 * @param _global JS global
 * @return bool Global define success
 */
bool defineFunctions( JSContext* _context, JS::Handle< JSObject* > _global );

/**
 * @brief Call JS function from global scope
 *
 * @param _context JS context
 * @param _global JS global
 * @param _functionName Global function name
 * @param _arguments Arguments to be passed
 * @param _returnValue Rooted return lvalue
 * @return bool Call attempt success
 */
bool callGlobalFunction( JSContext* _context,
                         JS::HandleObject _global,
                         std::string _functionName,
                         JS::HandleValueArray _arguments,
                         JS::RootedValue& _returnValue );

/**
 * @brief Execute JS file by name
 *
 * @param _context JS context
 * @param _fileName File name which has to be executed
 * @return bool Function exit status
 */
bool executeFileByName( JSContext* _context, std::string _fileName );

/**
 * @brief Execute JS code from string
 *
 * @param _context JS context
 * @param _fileName File name from what code was taken
 * @param _lineNumber Line number from what code was taken
 * @param _code Code as string to be executed
 * @return std::error_code Function exit code
 */
std::error_code executeCode( JSContext* _context,
                             std::string _fileName,
                             uint32_t _lineNumber,
                             std::string _code );

} // namespace JavaScriptIC
