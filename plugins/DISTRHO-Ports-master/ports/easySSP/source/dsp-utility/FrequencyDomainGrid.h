#ifndef TOMATL_FREQUENCY_DOMAIN_GRID
#define TOMATL_FREQUENCY_DOMAIN_GRID

#include <string>

namespace tomatl{ namespace dsp{

	// TODO: left and top offsets for coordinates.
	class FrequencyDomainGrid
	{
	public:
		enum NoteName
		{
			noteA = 0,
			noteASharp,
			noteB,
			noteC,
			noteCSharp,
			noteD,
			noteDSharp,
			noteE,
			noteF,
			noteFSharp,
			noteG,
			noteGSharp
		};

		struct NoteNotation
		{
			NoteNotation() : 
				mOctaveNumber(4), mNoteName(noteA), mCentsCount(0)
			{
			}

			NoteNotation(short octaveNumber, NoteName noteName, unsigned short centsCount) :
				mOctaveNumber(octaveNumber), mNoteName(noteName), mCentsCount(centsCount)
			{
			}

			// TODO: this and next() methods are rather hacky. Maybe make NoteName start from C
			// and rewrite
			static NoteNotation fromFrequency(double freq)
			{
				NoteNotation result;

				// 440.0 - base tuning frequency. Which is A4.
				double linearFreq = std::log2(freq / 440.0) + 5;

				// Octave number is equal to integer part of linear frequency obviously
				short octave = floor(linearFreq);

				// Cents from the beginning of octave. Each octave has 1200 cents, because 'cent' is one percent between two adjacent notes (centinote?)
				double cents = 1200 * (linearFreq - octave);

				int temp = floor(cents / 100.);

				unsigned short noteNumber = temp % 12;

				// Octave is starting from C, but our measurements were using A as octave start, so adjustment is needed
				if (noteNumber < noteC)
				{
					--octave;
				}

				result.mOctaveNumber = octave;
				result.mNoteName = (NoteName)noteNumber;
				result.mCentsCount = cents - noteNumber * 100;

				// We're trying to 'snap' note to closest one using negative cents.
				if (result.mCentsCount > 50)
				{
					result = result.next();
					result.mCentsCount = -(100 - result.mCentsCount);
				}

				return result;
			}

			NoteNotation next()
			{
				NoteNotation result;

				if (mNoteName == noteB)
				{
					result = NoteNotation(mOctaveNumber + 1, noteC, mCentsCount);
				}
				else if (mNoteName == noteGSharp)
				{
					result = NoteNotation(mOctaveNumber, noteA, mCentsCount);
				}
				else
				{
					result = NoteNotation(mOctaveNumber, (NoteName)(mNoteName + 1), mCentsCount);
				}

				return result;
			}

			std::wstring toWstring()
			{
				std::wstring result;

				const wchar_t* noteNames[12] = { L"A ", L"A#", L"B ", L"C ", L"C#", L"D ", L"D#", L"E ", L"F ", L"F#", L"G ", L"G#" };

				result += noteNames[mNoteName];

				result += std::to_wstring(mOctaveNumber);

				result += L" ";

				result += std::to_wstring(mCentsCount);

				result += L" cents";

				return result;
			}

			short mOctaveNumber;
			NoteName mNoteName;
			short mCentsCount;
		};

	private:
		tomatl::dsp::OctaveScale mFreqScale;
		tomatl::dsp::LinearScale mMagnitudeScale; // As we use dB values the scale is linear
		tomatl::dsp::Bound2D<double> mBounds;
		tomatl::dsp::Bound2D<double> mFullBounds;
		double* mFreqCache;
		size_t mSampleRate;
		size_t mBinCount;
		size_t mWidth;
		size_t mHeight;
		
		FrequencyDomainGrid(){}
		FrequencyDomainGrid(const FrequencyDomainGrid&){}

		void prepareFreqCache()
		{
			TOMATL_BRACE_DELETE(mFreqCache);

			if (mSampleRate > 0)
			{
				mFreqCache = new double[mSampleRate];
				memset(mFreqCache, 0x0, sizeof(double)* mSampleRate);
			}
		}

		forcedinline int calculateBinNumberToX(double value)
		{
			return mFreqScale.scale(mWidth, mBounds.X, binNumberToFrequency(value), true);
		}

		void recalcGrid()
		{
			mAmplGrid.clear();
			mFreqGrid.clear();

			const int size = 10;
			double freqs[size] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };

			for (int i = 0; i < size; ++i)
			{
				if (TOMATL_IS_IN_BOUNDS_INCLUSIVE(freqs[i], mBounds.X.mLow, mBounds.X.mHigh))
				{
					mFreqGrid.push_back(GridLine(freqToX(freqs[i]), freqs[i], freqToString(freqs[i])));
				}
			}

			int dbStep = std::abs(std::abs(mBounds.Y.mHigh) - std::abs(mBounds.Y.mLow)) * 3 < 72 ? 2 : 6;

			for (int i = mBounds.Y.mLow; i <= mBounds.Y.mHigh; i += dbStep)
			{
				mAmplGrid.push_back(GridLine(dbToY(i), i, dbToString(i)));
			}
		}
	public:
		struct GridLine
		{
			GridLine(int location, double value, std::wstring caption)
			{
				mLocation = location;
				mValue = value;
				mCaption = caption;
			}

			GridLine() : mLocation(0), mValue(0.), mCaption(L"") {}
		

			int mLocation;
			double mValue;
			std::wstring mCaption;
		};		

		FrequencyDomainGrid(Bound2D<double> fullBounds, size_t sampleRate = 0, size_t binCount = 0, size_t width = 0, size_t height = 0)
		{
			mFullBounds = fullBounds;
			mBounds = fullBounds;
			mSampleRate = sampleRate;
			mBinCount = binCount;
			mWidth = width;
			mHeight = height;
			mFreqCache = NULL;

			prepareFreqCache();
		}

		const tomatl::dsp::Bound2D<double>& getCurrentBounds()
		{
			return mBounds;
		}

		bool updateSize(size_t w, size_t h)
		{
			if (w != mWidth || h != mHeight)
			{
				mHeight = h;
				mWidth = w;
				prepareFreqCache();
				recalcGrid();

				return true;
			}

			return false;
		}

		bool updateSampleRate(size_t sampleRate)
		{
			if (sampleRate != mSampleRate)
			{
				mSampleRate = sampleRate;
				prepareFreqCache();

				return true;
			}

			return false;
		}

		bool updateBounds(Bound2D<double> bounds)
		{
			if (!mBounds.areEqual(bounds))
			{
				mBounds = bounds;
				recalcGrid();
				prepareFreqCache();

				return true;
			}

			return false;
		}

		void updateBinCount(size_t binCount)
		{
			if (binCount != mBinCount)
			{
				mBinCount = binCount;
			}
		}

		bool containsPoint(size_t x, size_t y) { return x <= getWidth() && y <= getHeight(); }

		bool isFrequencyVisible(const double& freq) { return TOMATL_IS_IN_BOUNDS_INCLUSIVE(freq, mBounds.X.mLow, mBounds.X.mHigh); }

		size_t getWidth() { return mWidth; }
		size_t getHeight() { return mHeight; }

		size_t getFreqLineCount() { return mFreqGrid.size(); }
		size_t getAmplLineCount() { return mAmplGrid.size(); }

		const GridLine& getFreqLine(int i) { return mFreqGrid[i]; }
		const GridLine& getAmplLine(int i) { return mAmplGrid[i]; }

		forcedinline double binNumberToFrequency(const double& value)
		{
			// Bin count * 2 is just a weird way of getting FFT size
			return value * mSampleRate / (mBinCount * 2);
		}

		forcedinline int freqToX(const double& value)
		{
			if (mFreqCache == NULL)
			{
				return mFreqScale.scale(mWidth, mBounds.X, value, true);
			}

			if (mFreqCache[(int)value] == 0)
			{
				mFreqCache[(int)value] = mFreqScale.scale(mWidth, mBounds.X, value, true);
			}

			return mFreqCache[(int)value];
		}

		forcedinline double xToFreq(const int& x)
		{
			return mFreqScale.unscale(mWidth, mBounds.X, x, true);
		}

		
		forcedinline int dbToY(const double& value)
		{
			return mHeight - mMagnitudeScale.scale(mHeight, mBounds.Y, value, true) - 1;
		}

		forcedinline int minusInfToY()
		{
			return dbToY(-1000.);
		}

		forcedinline int lowestVisibleFreqToX()
		{
			return freqToX(mBounds.X.mLow);
		}

		forcedinline double yToDb(const int& y)
		{
			return mMagnitudeScale.unscale(mHeight, mBounds.Y, mHeight - y - 1, true);
		}

		forcedinline double fullScaleYToDb(const int& y)
		{
			return mMagnitudeScale.unscale(mHeight, mFullBounds.Y, mHeight - y - 1, true);
		}

		forcedinline double fullScaleXToFreq(const double& value)
		{
			return mFreqScale.unscale(mWidth, mFullBounds.X, value, true);
		}

		~FrequencyDomainGrid()
		{
			TOMATL_BRACE_DELETE(mFreqCache);
		}
		
		std::wstring getPointNotation(int x, int y)
		{
			if (containsPoint(x, y))
			{
				auto ampl = dbToExtendedString(yToDb(y));
				auto freq = freqToExtendedString(xToFreq(x));

				auto note = NoteNotation::fromFrequency(xToFreq(x));

				return L"" + freq + L", " + ampl + L" | " + note.toWstring() + L"";
			}
			else
			{
				return L"";
			}
		}

		static std::wstring freqToExtendedString(const double& freq)
		{
			wchar_t buffer[50];
			memset(&buffer, 0x0, 50);

			swprintf(buffer, 50, L"%.0fHz", freq);

			return std::wstring(buffer);
		}

		static std::wstring dbToExtendedString(const double & ampl)
		{
			wchar_t buffer[50];
			memset(&buffer, 0x0, 50);

			swprintf(buffer, 50, L"%.2fdB", ampl);

			return std::wstring(buffer);
		}

		static std::wstring freqToString(const double& freq)
		{
			wchar_t buffer[50];
			memset(&buffer, 0x0, 50);

			if (freq >= 1000)
			{
				swprintf(buffer, 50, L"%dk", (int)freq / 1000);
			}
			else
			{
				swprintf(buffer, 50, L"%d", (int)freq);
			}

			return std::wstring(buffer);
		}

		static std::wstring dbToString(const double& ampl)
		{
			wchar_t buffer[50];
			memset(&buffer, 0x0, 50);

			swprintf(buffer, 50, L"%+d", (int)ampl);

			return std::wstring(buffer);
		}
	private:
		std::vector<GridLine> mFreqGrid;
		std::vector<GridLine> mAmplGrid;
	};

	/*class EqualizerGrid : public FrequencyDomainGrid
	{
	public:

		EqualizerGrid(Bound2D<double> fullBounds, size_t sampleRate = 0, size_t binCount = 0, size_t width = 0, size_t height = 0)
			: FrequencyDomainGrid(fullBounds, sampleRate, binCount, width, height)
		{
		}
		
		void addPoint(FrequencyDomainFilter* filter) { mPoints.push_back(filter); }

		size_t getPointCount() { return mPoints.size(); }

		FrequencyDomainFilter* getPoint(int index) { return mPoints[index]; }
	private:
		std::vector<tomatl::dsp::FrequencyDomainFilter*> mPoints;
	};*/
}}

#endif