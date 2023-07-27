#pragma once

#include <jsapi.h>

namespace JavaScriptIC {

extern const JSClassOps DefaultGlobalClassOps;

std::error_code throwError( JSContext* _context,
                            JS::HandleObject _global,
                            const char* _message,
                            const char* _fileName,
                            int32_t _lineNumber );
JSObject* createGlobal( JSContext* _context );
void reportAndClearException( JSContext* _context );
bool defineFunctions( JSContext* _context, JS::Handle< JSObject* > _global );

bool callGlobalFunction( JSContext* _context,
                         JS::HandleObject _global,
                         std::string _functionName,
                         JS::HandleValueArray _arguments,
                         JS::RootedValue& _returnValue );
std::error_code executeFileByName( JSContext* _context, std::string _fileName );
std::error_code executeCode( JSContext* _context,
                             std::string _fileName,
                             uint32_t _lineNumber,
                             std::string _code );

} // namespace JavaScriptIC
