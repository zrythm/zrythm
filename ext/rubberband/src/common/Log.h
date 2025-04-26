/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band Library
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2024 Particular Programs Ltd.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.

    Alternatively, if you have a valid commercial licence for the
    Rubber Band Library obtained by agreement with the copyright
    holders, you may redistribute and/or modify it under the terms
    described in that licence.

    If you wish to distribute code using the Rubber Band Library
    under terms other than those of the GNU General Public License,
    you must obtain a valid commercial licence before doing so.
*/

#ifndef RUBBERBAND_LOG_H
#define RUBBERBAND_LOG_H

#include <functional>
#include <iostream>

namespace RubberBand {

class Log {
public:
    Log(std::function<void(const char *)> _log0,
        std::function<void(const char *, double)> _log1,
        std::function<void(const char *, double, double)> _log2) :
        m_log0(_log0),
        m_log1(_log1),
        m_log2(_log2),
        m_debugLevel(m_defaultDebugLevel) { }

    Log(const Log &other) =default;
    Log(Log &&other) =default;
    Log &operator=(const Log &other) =default;
    Log &operator=(Log &&other) =default;

    void setDebugLevel(int level) { m_debugLevel = level; }
    int getDebugLevel() const { return m_debugLevel; }

    static void setDefaultDebugLevel(int level) { m_defaultDebugLevel = level; }
    
    void log(int level, const char *message) const {
        if (level <= m_debugLevel) m_log0(message);
    }
    void log(int level, const char *message, double arg0) const {
        if (level <= m_debugLevel) m_log1(message, arg0);
    }
    void log(int level, const char *message, double arg0, double arg1) const {
        if (level <= m_debugLevel) m_log2(message, arg0, arg1);
    }

private:
    std::function<void(const char *)> m_log0;
    std::function<void(const char *, double)> m_log1;
    std::function<void(const char *, double, double)> m_log2;
    int m_debugLevel;
    static int m_defaultDebugLevel;
};

}

#endif
