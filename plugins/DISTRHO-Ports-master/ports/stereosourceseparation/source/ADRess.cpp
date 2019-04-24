//
//  ADRess.cpp
//  StereoSourceSeparation
//
//  Created by Xinyuan Lai on 4/8/14.
//
//

#include "ADRess.h"

#define SCALE_DOWN_FACTOR 2

ADRess::ADRess(double sampleRate, int blockSize, int beta):sampleRate_(sampleRate),BLOCK_SIZE(blockSize),BETA(beta)
{
    currStatus_ = kBypass;
    d_ = BETA/2;
    H_ = BETA/4;
    LR_ = 2;
    currFilter_ = kAllPass;
    cutOffFrequency_ = 0.0;
    cutOffBinIndex_ = 0;
    
    // Hann window
    windowBuffer_ = new float[BLOCK_SIZE];
    for (int i = 0; i < BLOCK_SIZE; i++)
        windowBuffer_[i] = 0.5*(1.0 - cos(2.0*M_PI*(float)i/(BLOCK_SIZE-1)) );
    
    // all pass frequency mask
    frequencyMask_ = new float[BLOCK_SIZE/2+1];
    for (int i = 0; i < BLOCK_SIZE/2+1; i++)
        frequencyMask_[i] = 1.0;
    
    // initialise FFt
    fwd_= kiss_fftr_alloc(BLOCK_SIZE,0,NULL,NULL);
    inv_= kiss_fftr_alloc(BLOCK_SIZE,1,NULL,NULL);
    
    leftSpectrum_ = new complex<float>[BLOCK_SIZE];
    rightSpectrum_ = new complex<float>[BLOCK_SIZE];
    
    leftMag_ = new float[BLOCK_SIZE/2+1];
    rightMag_ = new float[BLOCK_SIZE/2+1];
    leftPhase_ = new float[BLOCK_SIZE/2+1];
    rightPhase_ = new float[BLOCK_SIZE/2+1];
    
    resynMagL_ = new float[BLOCK_SIZE/2+1];
    for (int i = 0; i<=BLOCK_SIZE/2; i++)
        resynMagL_[i] = 0;
    
    resynMagR_ = new float[BLOCK_SIZE/2+1];
    for (int i = 0; i<=BLOCK_SIZE/2; i++)
        resynMagR_[i] = 0;
    
    minIndicesL_ = new int[BLOCK_SIZE/2+1];
    minValuesL_ = new float[BLOCK_SIZE/2+1];
    maxValuesL_ = new float[BLOCK_SIZE/2+1];
    
    minIndicesR_ = new int[BLOCK_SIZE/2+1];
    minValuesR_ = new float[BLOCK_SIZE/2+1];
    maxValuesR_ = new float[BLOCK_SIZE/2+1];
    
    azimuthL_ = new float*[BLOCK_SIZE/2+1];
    for (int n = 0; n<BLOCK_SIZE/2+1; n++)
        azimuthL_[n] = new float[BETA+1];
    
    azimuthR_ = new float*[BLOCK_SIZE/2+1];
    for (int n = 0; n<BLOCK_SIZE/2+1; n++)
        azimuthR_[n] = new float[BETA+1];
    
}



ADRess::~ADRess()
{
    if (resynMagL_) {
        delete [] resynMagL_;
        resynMagL_ = 0;
    }
    
    if (resynMagR_) {
        delete [] resynMagR_;
        resynMagR_ = 0;
    }
    
    if (windowBuffer_) {
        delete [] windowBuffer_;
        windowBuffer_ = 0;
    }
    
    if (frequencyMask_) {
        delete [] frequencyMask_;
        frequencyMask_ = 0;
    }
    
    if (leftSpectrum_) {
        delete [] leftSpectrum_;
        leftSpectrum_ = 0;
    }
    
    if (rightSpectrum_) {
        delete [] rightSpectrum_;
        rightSpectrum_ = 0;
    }
    
    if (leftMag_) {
        delete [] leftMag_;
        leftMag_ = 0;
    }
    
    if (rightMag_) {
        delete [] rightMag_;
        rightMag_ = 0;
    }
    
    if (leftPhase_) {
        delete [] leftPhase_;
        leftPhase_ = 0;
    }
    
    if (rightPhase_) {
        delete [] rightPhase_;
        rightPhase_ = 0;
    }
    
    if (minIndicesL_) {
        delete [] minIndicesL_;
        minIndicesL_ = 0;
    }
    
    if (minValuesL_) {
        delete [] minValuesL_;
        minValuesL_ = 0;
    }
    
    if (maxValuesL_) {
        delete [] maxValuesL_;
        maxValuesL_ = 0;
    }
    
    if (minIndicesR_) {
        delete [] minIndicesR_;
        minIndicesR_ = 0;
    }
    
    if (minValuesR_) {
        delete [] minValuesR_;
        minValuesR_ = 0;
    }
    
    if (maxValuesR_) {
        delete [] maxValuesR_;
        maxValuesR_ = 0;
    }
    
    if (azimuthL_) {
        for (int n = 0; n<BLOCK_SIZE/2+1; n++)
            delete [] azimuthL_[n];
        
        delete [] azimuthL_;
        azimuthL_ = 0;
    }
    
    if (azimuthR_) {
        for (int n = 0; n<BLOCK_SIZE/2+1; n++)
            delete [] azimuthR_[n];
        
        delete [] azimuthR_;
        azimuthR_ = 0;
    }
    
}



void ADRess::setStatus(Status_t newStatus)
{
    currStatus_ = newStatus;
}


// left most to right most from 0 to BETA
void ADRess::setDirection(int newDirection)
{
    if (newDirection == BETA/2) {
        d_ = newDirection;
        LR_ = 2;
    }
    else if (newDirection < BETA/2) {
        d_  = newDirection;
        LR_ = 0;
    }
    else {
        d_  = BETA - newDirection;
        LR_ = 1;
    }

}



void ADRess::setWidth(int newWidth)
{
    H_ = newWidth;
}



void ADRess::setFilterType(FilterType_t newFilterType)
{
    currFilter_ = newFilterType;
    updateFrequencyMask();
}



void ADRess::setCutOffFrequency(float newCutOffFrequency)
{
    cutOffFrequency_ = newCutOffFrequency;
    cutOffBinIndex_ = static_cast<int>(cutOffFrequency_/sampleRate_*BLOCK_SIZE);
    updateFrequencyMask();
}



const ADRess::Status_t ADRess::getStatus()
{
    return currStatus_;
}



const int ADRess::getDirection()
{
    if (LR_) {
        return BETA - d_;
    } else {
        return d_;
    }
}



const int ADRess::getWidth()
{
    return H_;
}



const ADRess::FilterType_t ADRess::getFilterType()
{
    return currFilter_;
}



const int ADRess::getCutOffFrequency()
{
    return cutOffFrequency_;
}



void ADRess::process(float *leftData, float *rightData)
{
    // add window
    for (int i = 0; i<BLOCK_SIZE; i++) {
        leftData[i]  *= windowBuffer_[i];
        rightData[i] *= windowBuffer_[i];
    }
    
    if (currStatus_ != kBypass) {
        
        // do fft
        kiss_fftr(fwd_, (kiss_fft_scalar*)leftData,  (kiss_fft_cpx*)leftSpectrum_);
        kiss_fftr(fwd_, (kiss_fft_scalar*)rightData, (kiss_fft_cpx*)rightSpectrum_);
        
        // convert complex to magnitude-phase representation
        for (int i = 0; i<BLOCK_SIZE/2+1; i++) {
            leftMag_[i] = std::abs(leftSpectrum_[i]);
            rightMag_[i] = std::abs(rightSpectrum_[i]);
            leftPhase_[i] = std::arg(leftSpectrum_[i]);
            rightPhase_[i] = std::arg(rightSpectrum_[i]);
        }
        
        
        // generate right or left azimuth
        if (LR_ == 1) { // when right channel dominates
            for (int n = 0; n<BLOCK_SIZE/2+1; n++)
                for (int g = 0; g<=BETA; g++)
                    azimuthR_[n][g] = std::abs(leftSpectrum_[n] - rightSpectrum_[n]*(float)2.0*(float)g/(float)BETA);
            
            for (int n = 0; n<BLOCK_SIZE/2+1; n++) {
                getMinimumMaximum(n, azimuthR_[n], minValuesR_, minIndicesR_, maxValuesR_);
                
                for (int g = 0; g<=BETA; g++)
                    azimuthR_[n][g] = 0;
                
                if  (currStatus_ == kSolo)
                    // for better rejection of signal from other channel
                    azimuthR_[n][minIndicesR_[n]] = maxValuesR_[n] - minValuesR_[n];
                else
                    azimuthR_[n][minIndicesR_[n]] = maxValuesR_[n];
                
                resynMagR_[n] = sumUpPeaks(n, azimuthR_[n]);
            }
            
            // resynth and output
            for (int i = 0; i<BLOCK_SIZE/2+1; i++)
                rightSpectrum_[i] = std::polar(resynMagR_[i], rightPhase_[i]);
            
            kiss_fftri(inv_, (kiss_fft_cpx*)rightSpectrum_, (kiss_fft_scalar*)rightData);
            memcpy(leftData, rightData, BLOCK_SIZE*sizeof(float));
            
            if (currStatus_ == kSolo)
                for (int i = 0; i <BLOCK_SIZE; i++)
                    leftData[i] *= 2.0*d_/BETA;
            
        } else if (LR_ == 0) {   // when left channel dominates
            for (int n = 0; n<BLOCK_SIZE/2+1; n++)
                for (int g = 0; g<=BETA; g++)
                    azimuthL_[n][g] = std::abs(rightSpectrum_[n] - leftSpectrum_[n]*(float)2.0*(float)g/(float)BETA);
            
            for (int n = 0; n<BLOCK_SIZE/2+1; n++) {
                getMinimumMaximum(n, azimuthL_[n], minValuesL_, minIndicesL_, maxValuesL_);
                
                for (int g = 0; g<=BETA; g++)
                    azimuthL_[n][g] = 0;
                
                if  (currStatus_ == kSolo)
                    // for better rejection of signal from other channel
                    azimuthL_[n][minIndicesL_[n]] = maxValuesL_[n] - minValuesL_[n];
                else
                    azimuthL_[n][minIndicesL_[n]] = maxValuesL_[n];
                
                resynMagL_[n] = sumUpPeaks(n, azimuthL_[n]);
            }
            
            // resynth and output
            for (int i = 0; i<BLOCK_SIZE/2+1; i++)
                leftSpectrum_[i] = std::polar(resynMagL_[i], leftPhase_[i]);
            
            kiss_fftri(inv_, (kiss_fft_cpx*)leftSpectrum_, (kiss_fft_scalar*)leftData);
            memcpy(rightData, leftData, BLOCK_SIZE*sizeof(float));
            
            if (currStatus_ == kSolo)
                for (int i = 0; i <BLOCK_SIZE; i++)
                    rightData[i] *= 2.0*d_/BETA;
            
        } else {
            // azimuthR_ for right channel
            for (int n = 0; n<BLOCK_SIZE/2+1; n++) {
                for (int g = 0; g<=BETA; g++) {
                    azimuthR_[n][g] = std::abs(leftSpectrum_[n] - rightSpectrum_[n]*(float)2.0*(float)g/(float)BETA);
                    azimuthL_[n][g] = std::abs(rightSpectrum_[n] - leftSpectrum_[n]*(float)2.0*(float)g/(float)BETA);
                }
                
            }
            
            for (int n = 0; n<BLOCK_SIZE/2+1; n++) {
                getMinimumMaximum(n, azimuthR_[n], minValuesR_, minIndicesR_, maxValuesR_);
                getMinimumMaximum(n, azimuthL_[n], minValuesL_, minIndicesL_, maxValuesL_);
                
                for (int g = 0; g<=BETA; g++) {
                    azimuthR_[n][g] = 0;
                    azimuthL_[n][g] = 0;
                }
                
                if  (currStatus_ == kSolo) {
                    // for better rejection of signal from other channel
                    azimuthR_[n][minIndicesR_[n]] = maxValuesR_[n] - minValuesR_[n];
                    azimuthL_[n][minIndicesL_[n]] = maxValuesL_[n] - minValuesL_[n];
                    
                } else {
                    azimuthR_[n][minIndicesR_[n]] = maxValuesR_[n];
                    azimuthL_[n][minIndicesL_[n]] = maxValuesL_[n];
                    
                }
                
                resynMagR_[n] = sumUpPeaks(n, azimuthR_[n]);
                resynMagL_[n] = sumUpPeaks(n, azimuthL_[n]);
            }
            
            
            // resynth and output
            for (int i = 0; i<BLOCK_SIZE/2+1; i++)
                rightSpectrum_[i] = std::polar(resynMagR_[i], rightPhase_[i]);
            
            kiss_fftri(inv_, (kiss_fft_cpx*)rightSpectrum_, (kiss_fft_scalar*)rightData);
            
            for (int i = 0; i<BLOCK_SIZE/2+1; i++)
                leftSpectrum_[i] = std::polar(resynMagL_[i], leftPhase_[i]);
            
            kiss_fftri(inv_, (kiss_fft_cpx*)leftSpectrum_, (kiss_fft_scalar*)leftData);
            
        }
        
        // scale down ifft results and windowing
        for (int i = 0; i<BLOCK_SIZE; i++) {
            leftData[i] = leftData[i]*windowBuffer_[i]/BLOCK_SIZE/SCALE_DOWN_FACTOR;
            rightData[i] = rightData[i]*windowBuffer_[i]/BLOCK_SIZE/SCALE_DOWN_FACTOR;
        }
        
    }
    
    // when by-pass, compensate for the gain coming from 1/4 hopsize
    else {
        for (int i = 0; i<BLOCK_SIZE; i++) {
            leftData[i] /= SCALE_DOWN_FACTOR;
            rightData[i] /= SCALE_DOWN_FACTOR;
        }
    }
}



void ADRess::getMinimumMaximum(int nthBin, float* nthBinAzm, float* minValues, int* minIndices, float* maxValues)
{
    int minIndex = 0;
    float minValue = nthBinAzm[0];
    float maxValue = nthBinAzm[0];
    
    for (int i = 1; i<=BETA; i++) {
        if (nthBinAzm[i] < minValue) {
            minIndex = i;
            minValue = nthBinAzm[i];
        }
        if (nthBinAzm[i]>maxValue) {
            maxValue = nthBinAzm[i];
        }
    }
    
    minIndices[nthBin] = minIndex;
    minValues[nthBin] = minValue;
    maxValues[nthBin] = maxValue;
}



float ADRess::sumUpPeaks(int nthBIn, float *nthBinAzm)
{
    float sum = 0.0;
    
    int startInd = std::max(0, d_-H_/2);
    int endInd = std::min(BETA, d_+H_/2);
    
    switch (currStatus_) {
        case kSolo:
            
            if (frequencyMask_[nthBIn] == 0.0)
                return 0.0;
            
            for (int i = startInd; i<=endInd; i++)
                sum += nthBinAzm[i];
            
            // add smoothing along azimuth
            for (int i = 1; i<4 && startInd-i>=0; i++)
                sum += nthBinAzm[startInd-i]*(4-i)/4;
            for (int i = 1; i<4 && endInd+i<=BETA;i++)
                sum += nthBinAzm[endInd+i]*(4-i)/4;
            
            sum *= frequencyMask_[nthBIn];
            break;
            
        case kMute:
            if (currFilter_) {
                for (int i = startInd; i<=endInd; i++)
                    sum += nthBinAzm[i];
                sum *= frequencyMask_[nthBIn];
            }
            
            for (int i = 0; i<=BETA; i++)
                if (i<startInd || i>endInd)
                    sum += nthBinAzm[i];
            
        case kBypass:
        default:
            break;
            
    }
    return sum;
    
}


void ADRess::updateFrequencyMask()
{
    switch (currFilter_) {
        case kAllPass:
            for (int i = 0; i<BLOCK_SIZE/2+1; i++ )
                frequencyMask_[i] = 1.0;
            break;
            
        case kLowPass:
            for (int i = 0; i<cutOffBinIndex_; i++)
                frequencyMask_[i] = 1.0;
            for (int i = cutOffBinIndex_; i<BLOCK_SIZE/2+1; i++)
                frequencyMask_[i] = 0.0;
            
            frequencyMask_[cutOffBinIndex_] = 0.5;
            if (cutOffBinIndex_-1 >= 0)
                frequencyMask_[cutOffBinIndex_-1] = 0.75;
            if (cutOffBinIndex_+1 <= BLOCK_SIZE/2)
                frequencyMask_[cutOffBinIndex_+1] = 0.25;
            
            break;
            
        case kHighPass:
            for (int i = 0; i<cutOffBinIndex_; i++)
                frequencyMask_[i] = 0.0;
            for (int i = cutOffBinIndex_; i<BLOCK_SIZE/2+1; i++)
                frequencyMask_[i] = 1.0;
            break;
            
            frequencyMask_[cutOffBinIndex_] = 0.5;
            if (cutOffBinIndex_-1 >= 0)
                frequencyMask_[cutOffBinIndex_-1] = 0.25;
            if (cutOffBinIndex_+1 <= BLOCK_SIZE/2)
                frequencyMask_[cutOffBinIndex_+1] = 0.75;
            
        default:
            break;
    }
}