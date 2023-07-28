/**
 * @file auto_report_exception.hpp
 * @author lurkydismal (lurkydismal@duck.com)
 * @brief Auto Report Exception
 * @version 1.0
 * @date 2023-07-28
 *
 */

#pragma once

#include <jsapi.h>

/**
 * @brief Auto Report Exception
 *
 */
class AutoReportException {
public:
    /**
     * @brief Construct a new Auto Report Exception object
     *
     * @param _context JS context
     */
    explicit AutoReportException( JSContext* _context );

    /**
     * @brief Destroy the Auto Report Exception object
     *
     */
    ~AutoReportException();

public:
    /**
     * @brief JS context
     *
     */
    JSContext* context_;
};
