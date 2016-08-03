//
//  AcidAudioBuffer.cpp
//  SRXvert
//
//  Created by Dennis Lapchenko on 04/05/2016.
//
//

#include "AcidAudioBuffer.h"

using namespace AcidR;

AcidAudioBuffer::AcidAudioBuffer()
{
}

AcidAudioBuffer::~AcidAudioBuffer()
{
}

void AcidAudioBuffer::PeakNormalise(float maxPeak, float threshold)
{

    float peakGainMult = maxPeak / getMagnitude(0, getNumSamples());
    
    for(int index = 0; index < getNumSamples(); index++)
    {
        float sampleGain = getSample(0, index);
        if(std::fabs(sampleGain) > threshold)
        {
            sampleGain *= peakGainMult; //get new value, which is current amplitude * peak gain
        }
        applyGain(0, index, 0, sampleGain); //0 channel, index sample, apply gain to 1 sample, apply new value to that sample
    }
}

void AcidAudioBuffer::WriteFromReader(AudioFormatReader *reader)
{
    if(getNumSamples() >= 0) clear(); //clears the buffer if something was already written
    
    AudioSampleBuffer leftChannel, rightChannel; //seperate stereo channel buffers, which will be summed up into mono channel
    
    leftChannel.setSize(1, reader->lengthInSamples); //left channel buffer set to 1 channel and length of converted files samples
    reader->read(&leftChannel, 0, reader->lengthInSamples, 0, true, false); //left channel filled with leftchannel data from file reader
    rightChannel.setSize(1, reader->lengthInSamples); //right channel buffer set to 1 channel and length of converted files samples
    reader->read(&rightChannel, 0, reader->lengthInSamples, 0, false, true); //right channel filled with rightchannel data from file reader
    
    const float* leftSamples = leftChannel.getReadPointer(0); //get the pointer to the start of left channel samples array
    const float* rightSamples = rightChannel.getReadPointer(0); //get the pointer to the start of right channel samples array
    
    if(getNumChannels() < 2) //if Stereo is unticked, file will be converted to mono, else - stereo
    {
        //MONO: summing R+L channels
        for(int index = 0; index < reader->lengthInSamples; index++)
        {
            float averageSample = (leftSamples[index] + rightSamples[index]) / 2.0f;
            addSample(0, index, averageSample);
        }
    }
    else
    {
        //STEREO: writing left and right channels to buffer
        for(int index = 0; index < reader->lengthInSamples; index++)
        {
            addSample(0, index, leftSamples[index]);
            addSample(1, index, rightSamples[index]);
        }
    }
    leftChannel.clear(); //clear temporary read channels
    rightChannel.clear();
}

void AcidAudioBuffer::setSampleRate(double newSampleRate)
{
    sampleRate = newSampleRate;
}

double AcidAudioBuffer::getSampleRate()
{
    return sampleRate;
}