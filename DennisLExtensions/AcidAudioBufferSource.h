//
//  AcidAudioBufferSource.hpp
//  SRXvert
//
//  Created by Dennis Lapchenko on 04/05/2016.
//
//

#ifndef ACIDAUDIOBUFFERSOURCE_H_
#define ACIDAUDIOBUFFERSOURCE_H_

#include "AcidHeader.h"


/*
 This class allows for AudioBuffer playback/real-time resampling 
 using Juce's AudioSource classes. 
 Was initially written for this project to use AudioSource's resampling, however later own resampling solution was written
 */

namespace AcidR
{
    class AcidAudioBufferSource : public PositionableAudioSource
    {

        
        
    public:
        AcidAudioBufferSource(AcidAudioBuffer *buffer);
        
        ~AcidAudioBufferSource();
        
        void getNextAudioBlock(const AudioSourceChannelInfo &info);
        
        void prepareToPlay(int, double);
        
        void releaseResources();
        
        void setNextReadPosition(long long newPosition);
        
        long long getNextReadPosition() const;
        
        long long getTotalLength() const;
        
        bool isLooping() const;
        
        void setLooping(bool shouldLoop);
        
        void setBuffer(AcidAudioBuffer *buffer);
        
        AcidAudioBuffer *_buffer;
        
    private:
        int _position;
        int _start;
        bool _repeat;
    };
}

#endif /* AcidAudioBufferSource_h */