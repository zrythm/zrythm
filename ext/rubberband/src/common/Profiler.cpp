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

#include "Profiler.h"

#include "Thread.h"

#include <algorithm>
#include <set>
#include <string>
#include <map>

#include <stdio.h>

#ifdef _MSC_VER
// Ugh --cc
#define snprintf sprintf_s
#endif

namespace RubberBand {

#ifndef NO_TIMING

Profiler::ProfileMap
Profiler::m_profiles;

Profiler::WorstCallMap
Profiler::m_worstCalls;

static Mutex profileMutex;

void
Profiler::add(const char *id, double us)
{
    profileMutex.lock();
    
    ProfileMap::iterator pmi = m_profiles.find(id);
    if (pmi != m_profiles.end()) {
        ++pmi->second.first;
        pmi->second.second += us;
    } else {
        m_profiles[id] = TimePair(1, us);
    }

    WorstCallMap::iterator wci = m_worstCalls.find(id);
    if (wci != m_worstCalls.end()) {
        if (us > wci->second) wci->second = us;
    } else {
        m_worstCalls[id] = us;
    }

    profileMutex.unlock();
}

void
Profiler::dump()
{
    std::string report = getReport();
    fprintf(stderr, "%s", report.c_str());
}

std::string
Profiler::getReport()
{
    profileMutex.lock();
    
    static const int buflen = 256;
    char buffer[buflen];
    std::string report;

#ifdef PROFILE_CLOCKS
    snprintf(buffer, buflen, "Profiling points [CPU time]:\n");
#else
    snprintf(buffer, buflen, "Profiling points [Wall time]:\n");
#endif
    report += buffer;

    typedef std::multimap<double, const char *> TimeRMap;
    typedef std::multimap<int, const char *> IntRMap;
    TimeRMap totmap, avgmap, worstmap;
    IntRMap ncallmap;

    const unsigned char mu_s[] = { 0xce, 0xbc, 's', 0x0 };
    
    for (ProfileMap::const_iterator i = m_profiles.begin();
         i != m_profiles.end(); ++i) {
        totmap.insert(TimeRMap::value_type(i->second.second, i->first));
        avgmap.insert(TimeRMap::value_type(i->second.second /
                                           i->second.first, i->first));
        ncallmap.insert(IntRMap::value_type(i->second.first, i->first));
    }

    for (WorstCallMap::const_iterator i = m_worstCalls.begin();
         i != m_worstCalls.end(); ++i) {
        worstmap.insert(TimeRMap::value_type(i->second, i->first));
    }

    snprintf(buffer, buflen, "\nBy name:\n");
    report += buffer;

    typedef std::set<const char *, std::less<std::string> > StringSet;

    StringSet profileNames;
    for (ProfileMap::const_iterator i = m_profiles.begin();
         i != m_profiles.end(); ++i) {
        profileNames.insert(i->first);
    }

    for (StringSet::const_iterator i = profileNames.begin();
         i != profileNames.end(); ++i) {

        ProfileMap::const_iterator j = m_profiles.find(*i);
        if (j == m_profiles.end()) continue;

        const TimePair &pp(j->second);
        snprintf(buffer, buflen, "%s(%d):\n", *i, pp.first);
        report += buffer;
        snprintf(buffer, buflen, "\tReal: \t%12f %s      \t[%f %s total]\n",
                 (pp.second / pp.first), mu_s,
                 (pp.second), mu_s);
        report += buffer;

        WorstCallMap::const_iterator k = m_worstCalls.find(*i);
        if (k == m_worstCalls.end()) continue;
        
        snprintf(buffer, buflen, "\tWorst:\t%14f %s/call\n", k->second, mu_s);
        report += buffer;
    }

    snprintf(buffer, buflen, "\nBy total:\n");
    report += buffer;
    for (TimeRMap::const_iterator i = totmap.end(); i != totmap.begin(); ) {
        --i;
        snprintf(buffer, buflen, "%-40s  %14f %s\n", i->second, i->first, mu_s);
        report += buffer;
    }

    snprintf(buffer, buflen, "\nBy average:\n");
    report += buffer;
    for (TimeRMap::const_iterator i = avgmap.end(); i != avgmap.begin(); ) {
        --i;
        snprintf(buffer, buflen, "%-40s  %14f %s\n", i->second, i->first, mu_s);
        report += buffer;
    }

    snprintf(buffer, buflen, "\nBy worst case:\n");
    report += buffer;
    for (TimeRMap::const_iterator i = worstmap.end(); i != worstmap.begin(); ) {
        --i;
        snprintf(buffer, buflen, "%-40s  %14f %s\n", i->second, i->first, mu_s);
        report += buffer;
    }

    snprintf(buffer, buflen, "\nBy number of calls:\n");
    report += buffer;
    for (IntRMap::const_iterator i = ncallmap.end(); i != ncallmap.begin(); ) {
        --i;
        snprintf(buffer, buflen, "%-40s  %14d\n", i->second, i->first);
        report += buffer;
    }

    profileMutex.unlock();
    
    return report;
}

Profiler::Profiler(const char* c) :
    m_c(c),
    m_ended(false)
{
    m_start = std::chrono::steady_clock::now();
}

Profiler::~Profiler()
{
    if (!m_ended) end();
}

void
Profiler::end()
{
    auto finish = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::micro> us = finish - m_start;
    add(m_c, us.count());
    m_ended = true;
}
 
#else /* NO_TIMING */

#ifndef NO_TIMING_COMPLETE_NOOP

Profiler::Profiler(const char *) { }
Profiler::~Profiler() { }
void Profiler::end() { }
void Profiler::dump() { }

#endif

#endif

}
