/**
 * @file std.hpp
 * @author lurkydismal (lurkydismal@duck.com)
 * @brief C/ C++ functions to be called from JS
 * @version 1.0
 * @date 2023-07-28
 *
 */

#pragma once

#include <jsapi.h>

/**
 * @brief Console print
 *
 * @details print( _object )
 * @return bool Success status
 */
bool printJS( JSContext* _context,
              uint32_t _argumentCount,
              JS::Value* _valuePointer );

/**
 * @brief Functions C/ C++ functions to be defined in JS
 *
 */
const JSFunctionSpec GlobalFunctions[] = { JS_FN( "print", &printJS, 1, 0 ),
                                           JS_FS_END };
