//
//  AcidGranulator.hpp
//  SRXvert
//
//  Created by Dennis Lapchenko on 11/05/2016.
//
//

#ifndef ACIDGRANULATOR_H_
#define ACIDGRANULATOR_H_

#include "AcidUtils.h"
#include "AcidHeader.h"


namespace AcidR
{
    class AcidGranulator
    {
    public:
        AcidGranulator();
        ~AcidGranulator();
        
        void setGrainSize(int sizeInMilliseconds);
        void setGrainSizeSamples(int sizeInSamples);
        void setRandomGrainSize(int min, int max);
        void setBiasGrain(int sizeInMillis);
        void setStretch(float stretch);
        void setRandomStretch(float min, float max);
        void setBiasStretch(float stretch);
        void setOutputDuration(long milliseconds);
        void setEnvelopeType(int type);
        void setVoices(int numVoices);
        void setSampleRate(int newSampleRate);
        void setRandomEveryGrain(bool trueIfGrainFalseIfVoice);
        
        bool process(AcidAudioBuffer *input, AcidAudioBuffer *outputTo, long newDurationInMilliseconds);
        
        //_stretch/_grainS is being changed by code while processing, userSetStretch/Grain remains the same and used for random bias calc
        int userSetGrain;
        float userSetStretch;
        
    private:
        int _sampleRate;
        int _grainSize, _userSetGrain; //_grainsize is being changed by code while processing, _userSetGrain remains the same and used for random bias calc
        Range<int> _minMaxRandomGrain; //min and max random grain deviation from main grain size
        float _stretch, _userSetStretch; //_stretch is being changed by code while processing, _userSetStretch remains the same and used for random bias calc
        Range<float> _minMaxRandomStretch; //min and max random stretch amount deviation from main ratio
        long _newDuration;
        int _voices;
        bool randomEveryGrain;
        
        AcidUtils acidUtils;
        AcidAudioBuffer envelopeBuffer;
        void calculateEnvelope(int bufferSize);
        int _envelopeType;
    };
}



#endif /* AcidGranulator_h */
