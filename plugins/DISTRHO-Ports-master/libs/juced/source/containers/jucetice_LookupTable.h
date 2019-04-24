/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2009 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU General Public License, as published by the Free Software Foundation;
 either version 2 of the License, or (at your option) any later version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================
*/

#ifndef __JUCETICE_LOOKUPTABLE_HEADER__
#define __JUCETICE_LOOKUPTABLE_HEADER__

// ==============================================================================
/**
    This is a simple generic lookup table(lut) class for functions that
    return a single float and take a single float parameter, for example
    a specific function in the form:

    @code
        float somefunction (float x);
    @endcode

    becomes

    @code
        float somelut.lookup (float x);
    @endcode

*/
class LookupTable
{
public:

    typedef float (*LookupTableFunction)(float);

    //==============================================================================
    /** Constructor provides everything */
    LookupTable (int samples, float instart, float inend, LookupTableFunction func )
    {
        //store the pointer to the function for testing later
        function = func;
        size = samples;
        start = instart;
        end = inend;

        data = new float [size + 1];

        // may as well fabs() to be sure we get a positive delta
        float diff = fabs (end - start);
        float angleDelta = diff / (float)(size);

        for (int i = 0; i < size; i++)
        {
            const float x = start + (float)i * angleDelta;
            data[i] = (*function)(x);
        }
        data [size] = data [0];

        delta = 1.0 / angleDelta;
    }

    /** Destructor */
    ~LookupTable()
    {
        if(data)
            delete[] data;
    }

    //==============================================================================
    /** Looks up like the function */
    forcedinline float lookup (float lookup_what) const
    {
        lookup_what *= delta;
        int il = (int) lookup_what ;

        il = il % size;
//        while (il >= size) il -= size;
//        while (il < 0) il += size;

        jassert (il >= 0);
        jassert (il < size);

        return data[il];
    }

    /** Looks up like the function with linear interpolation */
    forcedinline float lookupLinear (float lookup_what) const
    {
        lookup_what *= delta;
        int il = (int) lookup_what ;
        float fr = lookup_what - il;

        il = il % size;
//        while (il >= size - 1) il -= size - 1;
//        while (il < 0) il += size - 1;

        jassert (il >= 0);
        jassert (il < size - 1);

        return data[il] + fr * data[il + 1];
    }

    //==============================================================================
    /** This is useful for testing lookup table speed */
/*
    void testSpeed(int test_samples) const
    {
        // printf("\n\ntesting speed\n\n");

        long before,after,deltatime;

        float some_number = 2.0f * float_Pi / 1.23456789f;
        float out;

        before = Time::getMillisecondCounterHiRes();
        for (int x=0;x<test_samples;x++)
            out = lookup(some_number);
        after = Time::getMillisecondCounterHiRes();
        deltatime = after - before;
        // printf("lookup(%f) x %i = (%f) in %i msec\n",some_number,samples,out,deltatime);

        before = Time::getMillisecondCounterHiRes();
        for (int x=0;x<test_samples;x++)
            out = function(some_number);
        after = Time::getMillisecondCounterHiRes();
        deltatime = after - before;

        // printf("function(%f) x %i = (%f) in %i msec\n",some_number,samples,out,deltatime);
    }
*/

private:

    float *data;
    int size;
    float start,end;
    float delta;
    LookupTableFunction function;
};


// static const LookupTable sintable (1024, 0.0f, 2.0f*float_Pi, sinf);
// static const LookupTable costable (1024, 0.0f, 2.0f*float_Pi, cosf);

#endif // __JUCETICE_LOOKUPTABLE_HEADER__