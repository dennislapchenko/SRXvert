//
//  AcidAudioBuffer.hpp
//  SRXvert
//
//  Created by Dennis Lapchenko on 04/05/2016.
//
//

#ifndef ACIDAUDIOBUFFER_H_
#define ACIDAUDIOBUFFER_H_

#include "../JuceLibraryCode/JuceHeader.h"

namespace AcidR
{
    class AcidAudioBuffer : public AudioSampleBuffer
    {
    public:
        AcidAudioBuffer();
        
        ~AcidAudioBuffer();
        
        
        void setSampleRate(double newSampleRate);
        double getSampleRate();
        void PeakNormalise(float maxPeak, float threshold);
        void WriteFromReader(AudioFormatReader *reader);
        
    private:
        double sampleRate;
    };
}

#endif /* AcidAudioBuffer_h */