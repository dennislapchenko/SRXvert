//
//  AcidResampler.cpp
//  SRXvert
//
//  Created by Dennis Lapchenko on 06/05/2016.
//
//

#include "AcidResampler.h"

using namespace AcidR;

AcidResampler::AcidResampler()
{
}

AcidResampler::~AcidResampler()
{
}

bool AcidResampler::resample(AcidAudioBuffer *inputBuffer, AcidAudioBuffer *outputBuffer, double newSampleRate, int interpType)
{
    double  srRatio;
    if (inputBuffer->getSampleRate() > newSampleRate) srRatio = inputBuffer->getSampleRate() / newSampleRate; //decimation ratio
    else srRatio = newSampleRate / inputBuffer->getSampleRate(); //upsampling ratio
    
    double  srIncrement = srRatio - 1, //the amount to add to the Counter to know when to add/decimate extra samples
            srAddCount = 0.0f;
    
    int     inputSamples = inputBuffer->getNumSamples(),
            outputSamples = inputSamples * (newSampleRate / inputBuffer->getSampleRate());
    
    outputBuffer->setSize(inputBuffer->getNumChannels(), outputSamples);
    
    if (inputBuffer->getSampleRate() == newSampleRate) //if file's samplerate is same as chosen samplerate, the buffer is a straight copy
    {
        outputBuffer->makeCopyOf(*inputBuffer);
        return true;
    }

    for(int channel = 0; channel < outputBuffer->getNumChannels(); channel++) //iterating through 1 or 2 channels
    {
        const   float* readingHead = inputBuffer->  getReadPointer(channel);
                float* writingHead = outputBuffer-> getWritePointer(channel);
        
        //Juce built-in sample converter, using 4point lagrange interpolation
        if(interpType == 4)
        {
            LagrangeInterpolator lagrange;
            lagrange.process(inputBuffer->getSampleRate()/newSampleRate, readingHead, writingHead, outputSamples);
            lagrange.reset();
            continue; //<- skips all code below and continues to next channel loop iteration
        }
        
        int readingIndex = 0,
            writingIndex = 0;
        
        if (inputBuffer->getSampleRate() < newSampleRate) //UPSAMPLING INTERPOLATION
        {
            while (readingIndex < inputSamples)
            {
                writingHead[writingIndex++] = readingHead[readingIndex];
                srAddCount += srIncrement;
                if(std::abs(srAddCount) >= 1 && readingIndex+1 <= inputSamples) //if increment counter has reached above 1 = its time to add samples!
                {                                                               //+ checks to not go outside of buffer bounds
                    int samplesToAdd = std::abs(srAddCount);
                    for(int i = 0; i < samplesToAdd; ++i)
                    {
                        double mu = (1/(samplesToAdd+1) * i);
                        writingHead[writingIndex++] = interpolate(interpType, readingHead, readingIndex, inputSamples, mu);
                    }
                    srAddCount -= samplesToAdd;
                }
                readingIndex++;
                //DBG("INTERP: "+String(readingIndex)+"/"+String(inputSamples)+" : "+ String(writingIndex)+"/"+String(outputSamples));
            };
        }
        else if (inputBuffer->getSampleRate() > newSampleRate) //DOWNSAMPLING DECIMATION
        {
            while (readingIndex < inputSamples)
            {
                writingHead[writingIndex++] = readingHead[readingIndex];
                srAddCount += srIncrement;
                if(std::abs(srAddCount) >= 1 && readingIndex+1 <= inputSamples) //if increment counter has reached above 1 = its time to decimate samples!
                {                                                               //+ checks to not go outside of buffer bounds
                    int samplesToDecimate = std::abs(srAddCount);
                    for(int i = 0; i < samplesToDecimate; ++i)
                    {
                        readingIndex++; //skip readers samples
                    }
                    srAddCount -= samplesToDecimate;
                }
                readingIndex++;
                //DBG("DECIM: "+String(readingIndex)+"/"+String(inputSamples)+" : "+ String(writingIndex)+"/"+String(outputSamples));
            };
        }
        
        //IIR ANTI-ALIASING Filter
        IIRFilter IIRfilter;
        IIRfilter.setCoefficients(IIRCoefficients::makeLowPass(newSampleRate, newSampleRate*0.5f));
        for(int i = 0; i < 2; i++) //applies the filter 2 times to each channel for better anti-aliasing effect
        {
            IIRfilter.processSamples(writingHead, outputSamples);
            IIRfilter.reset();
        }
        
        readingHead = nullptr;
        writingHead = nullptr;
    }
    return true;
}


float AcidResampler::interpolate(int interpType, const float *readingHead, int currentIndex, int maxIndex, double mu)
{
    float   result;
    float   s0,
            s1 = (currentIndex >= maxIndex) ? 0 : readingHead[currentIndex],
            s2 = (currentIndex+1 >= maxIndex) ? 0 :readingHead[currentIndex+1],
            s3; //s0 & s3 are the additional points for cubic interpolation
    float   mu2; //needed for cosine and cubic interpolation
    
    switch(interpType)
    {
        case(1): //LINEAR
            result = (s1*(1-mu) + s2*mu);
            break;
        case(2): //COSINE
            mu2 = (1-cos(mu*double_Pi))/2;
            result = (s1*(1-mu2) + s2*mu2);
            break;
        case(3): //CUBIC
            s0 = (currentIndex < 1) ? 0.0 : readingHead[currentIndex-1]; //if current sample is first one, add a 0 as first cubic interp sample
            s3 = (currentIndex+2 > maxIndex) ? 0.0 : readingHead[currentIndex+2]; //if taking sample 2 positions ahead would go outside buffer - add a 0
            
            mu2 = mu*mu;
            float a0, a1, a2, a3;
            a0 = s3 - s2 - s0 + s1;
            a1 = s0 - s1 - a0;
            a2 = s2 - s0;
            a3 = s1;
            
            result = (a0*mu*mu2 + a1*mu2 + a2*mu + a3);
            break;
        case(4):
            //Lagrange conversion & filtering is used, with almost whole resample() bodu being skipped
            break;
        default:
            //jassert(interpType < 3); //interpolation enum not created
            break;
    }
    return result;
};