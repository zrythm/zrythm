#ifndef TOMATL_BUFFER
#define TOMATL_BUFFER

#include <vector>

namespace tomatl { namespace dsp {

template <typename T> class SimpleBuffer
{
private:
	T* mData = NULL;
	size_t mLength = 0;
	size_t mPointer = 0;

	TOMATL_DECLARE_NON_MOVABLE_COPYABLE(SimpleBuffer)
public:
	SimpleBuffer(size_t length, bool clear = true) : mLength(length)
	{
		mData = new T[mLength];

		if (clear) memset(mData, 0x0, sizeof(T) * mLength);
	}

	forcedinline bool isFull()
	{
		return mPointer == mLength;
	}

	forcedinline bool putOne(const T& subj)
	{
		if (!isFull())
		{
			mData[mPointer] = subj;
			++mPointer;

			return true;
		}
		
		return false;
	}

	T* getContents() { return mData; }

	bool read(T& writeTo, const size_t& pos)
	{
		if (pos >= 0 && pos < mPointer)
		{
			writeTo = mData[pos];

			return true;
		}

		return false;
	}

	const size_t& getLength() { return mLength; }
	const size_t& getPointer() { return mPointer; }

	void clear(size_t position = 0, bool eraseData = false)
	{
		mPointer = position;

		if (eraseData) memset(mData, 0x0, sizeof(T)* mLength);
	}

	virtual ~SimpleBuffer()
	{
		TOMATL_BRACE_DELETE(mData);
	}
};

template <typename T> class DelayBuffer
{
private:
	TOMATL_DECLARE_NON_MOVABLE_COPYABLE(DelayBuffer);
	SimpleBuffer<T>* mReadBuffer = NULL;
	SimpleBuffer<T>* mWriteBuffer = NULL;

	T mTemp;
public:
	DelayBuffer(size_t length) : mReadBuffer(new SimpleBuffer<T>(length)), mWriteBuffer(new SimpleBuffer<T>(length))
	{
		for (int i = 0; i < mReadBuffer->getLength(); ++i)
		{
			mReadBuffer->putOne(0x0);
		}
	}

	const T& put(const T& subj)
	{
		if (!mWriteBuffer->putOne(subj))
		{
			std::swap(mWriteBuffer, mReadBuffer);

			mWriteBuffer->clear();

			mWriteBuffer->putOne(subj);
		}
		
		mReadBuffer->read(mTemp, mWriteBuffer->getPointer() - 1);

		return mTemp;
	}

	virtual ~DelayBuffer()
	{
		TOMATL_DELETE(mReadBuffer);
		TOMATL_DELETE(mWriteBuffer);
	}
};

template <typename T> class OverlappingBufferSequence
{
private:
	TOMATL_DECLARE_NON_MOVABLE_COPYABLE(OverlappingBufferSequence);
	std::vector<SimpleBuffer<T>*> mBuffers;
	size_t mHopSize;
	double mOverlappingFactor;
public:
	OverlappingBufferSequence(size_t segmentLength, size_t hopSize) : mHopSize(hopSize), mOverlappingFactor((double)mHopSize / (double)segmentLength)
	{
		size_t hopCount = segmentLength / hopSize;

		// TODO: sanity checks? hopSize < segmentLength and maybe segmentLength % hopSize == 0
		for (int i = 0; i < hopCount; ++i)
		{
			mBuffers.push_back(new SimpleBuffer<T>(segmentLength));
			mBuffers[i]->clear(hopSize * i);
		}
	}

	virtual ~OverlappingBufferSequence()
	{
		for (int i = 0; i < mBuffers.size(); ++i)
		{
			TOMATL_DELETE(mBuffers[i]);
		}

		mBuffers.clear();
	}

	std::tuple<T*, size_t> putOne(const T& subject)
	{
		SimpleBuffer<T>* result = NULL;
		size_t index = 0;
		
		for (int i = 0; i < mBuffers.size(); ++i)
		{
			mBuffers[i]->putOne(subject);

			if (mBuffers[i]->isFull())
			{
				result = mBuffers[i];
				index = i;
				mBuffers[i]->clear();
			}
		}

		return std::tuple<T*, size_t>(result == NULL ? NULL : result->getContents(), index);
	}

	const size_t& getSegmentLength() { return mBuffers[0]->getLength(); }
	const size_t& getHopSize() { return mHopSize; }
	const double& getOverlappingFactor() { return mOverlappingFactor; }
	const size_t& getSegmentCount() { return mBuffers.size(); }
	size_t getSegmentOffset(size_t i) { return i * mHopSize; }
};


}}
#endif