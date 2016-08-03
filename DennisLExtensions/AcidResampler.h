//
//  AcidResampler.h
//  SRXvert
//
//  Created by Dennis Lapchenko on 06/05/2016.
//
//

#ifndef ACIDRESAMPLER_H_
#define ACIDRESAMPLER_H_

#include "AcidHeader.h"

namespace AcidR
{
    class AcidResampler
    {
    public:
        AcidResampler();
        
        ~AcidResampler();
        
        bool resample(AcidAudioBuffer* input, AcidAudioBuffer* output, double newSampleRate, int interpType);
        float interpolate(int interpType, const float* readingHead, int currentIndex, int maxIndex, double mu);
    };
}

#endif
