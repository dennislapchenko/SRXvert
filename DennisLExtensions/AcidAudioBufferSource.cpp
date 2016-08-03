//
//  AcidAudioBufferSource.cpp
//  SRXvert
//
//  Created by Dennis Lapchenko on 04/05/2016.
//
//

#include "AcidAudioBufferSource.h"

using namespace AcidR;

/*
 This class allows for AudioBuffer playback/real-time resampling
 using Juce's AudioSource classes.
 Was initially written for this project to use AudioSource's resampling, however later own resampling solution was written
 */

AcidAudioBufferSource::AcidAudioBufferSource(AcidAudioBuffer *buffer)
        : _position(0), _start(0), _repeat(false), _buffer(buffer)
{}

AcidAudioBufferSource::~AcidAudioBufferSource()
{
    _buffer = NULL;
}

void AcidAudioBufferSource::getNextAudioBlock(const AudioSourceChannelInfo &info)
{
    int bufferSamples = _buffer->getNumSamples();
    int bufferChannels = _buffer->getNumChannels();
    
    if(info.numSamples > 0)
    {
        int start = _position;
        int numToCopy = 0;
        
        if(start + info.numSamples <= bufferSamples)
        {
            numToCopy = info.numSamples; //copy all samples
        }
        else if(start > bufferSamples)
        {
            numToCopy = 0; //dont copy
        }
        else if(bufferSamples - start > 0)
        {
            numToCopy = bufferSamples - start; //copy the remainder in the buffer
        }
        else
        {
            numToCopy = 0; //dont copy
        }
        
        if(numToCopy > 0)
        {
            //loop through channels and copy samples
            for(int channel = 0; channel < bufferChannels; channel++)
            {
                info.buffer->copyFrom(channel, info.startSample, *_buffer, channel, start, numToCopy);
            }
            
            _position += numToCopy; //update source position;
        }
    }
}

void AcidAudioBufferSource::prepareToPlay(int expedtedSamplesPerBlock, double sampleRate)
{
    
}

void AcidAudioBufferSource::releaseResources()
{}

void AcidAudioBufferSource::setNextReadPosition(long long newPosition)
{
    if(newPosition >= 0 && newPosition < _buffer->getNumSamples())
    {
        _position = newPosition;
    }
}

long long AcidAudioBufferSource::getNextReadPosition() const
{
    return _position;
}

long long AcidAudioBufferSource::getTotalLength() const
{
    return _buffer->getNumSamples();
}

bool AcidAudioBufferSource::isLooping() const
{
    return _repeat;
}

void AcidAudioBufferSource::setLooping(bool shouldLoop)
{
    _repeat = shouldLoop;
}

void AcidAudioBufferSource::setBuffer(AcidAudioBuffer *buffer)
{
    _buffer = buffer;
    setNextReadPosition(0);
}
