#pragma once

#include <jsapi.h>

class AutoReportException {
public:
    JSContext* context_;

public:
    explicit AutoReportException( JSContext* _context );

    ~AutoReportException();
};
