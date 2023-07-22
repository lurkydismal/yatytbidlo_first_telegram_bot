#pragma once

#include <jsapi.h>

bool printJS( JSContext* _context,
              unsigned _argumentCount,
              JS::Value* _valuePointer );
