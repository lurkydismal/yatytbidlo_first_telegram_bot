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
 * @param _context JS context
 * @param _argumentCount Argument count
 * @param _valuePointer Arguments pointer
 * @return true On success
 * @return false If convertation to JSString has failed
 */
bool printJS( JSContext* _context,
              unsigned _argumentCount,
              JS::Value* _valuePointer );
