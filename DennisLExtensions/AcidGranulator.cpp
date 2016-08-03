//
//  AcidGranulator.cpp
//  SRXvert
//
//  Created by Dennis Lapchenko on 11/05/2016.
//
//

#include "AcidGranulator.h"

using namespace AcidR;

AcidGranulator::AcidGranulator()
{
    //set default values
    _sampleRate = 44100;
    setGrainSize(50);
    setRandomGrainSize(0, 0);
    setStretch(1.0f);
    setRandomStretch(0.0, 0.0);
    setVoices(1);
    setEnvelopeType(1);
}

AcidGranulator::~AcidGranulator()
{
    envelopeBuffer.clear();
}

//argument is grain size in milliseconds, but for calc we convert to size in samples
void AcidGranulator::setGrainSize(int sizeInMilliseconds)
{
    _grainSize = _sampleRate / (1000.0/(float)sizeInMilliseconds);
    calculateEnvelope(_grainSize);
}
void AcidGranulator::setGrainSizeSamples(int sizeInSamples)
{
    _grainSize = sizeInSamples;
    calculateEnvelope(_grainSize);
}
void AcidGranulator::setBiasGrain(int sizeInMillis)
{
    userSetGrain = _sampleRate / (1000.0/(float)sizeInMillis);
}

void AcidGranulator::setRandomGrainSize(int min, int max)
{
    if(min == _grainSize && max == _grainSize)
    {
        _minMaxRandomGrain = Range<int>(0,0);
        return;
    }
    auto minSamples = _sampleRate / (1000.0/(float)min);
    auto maxSamples = _sampleRate / (1000.0/(float)max);
    _minMaxRandomGrain = Range<int>(minSamples,maxSamples);
}

void AcidGranulator::setStretch(float stretch)
{
    _stretch = stretch;
}

void AcidGranulator::setBiasStretch(float stretch)
{
    userSetStretch = stretch;
}

void AcidGranulator::setRandomStretch(float min, float max)
{
    if(min == _stretch && max == _stretch)
    {
        _minMaxRandomStretch = Range<float>(0,0);
        return;
    }
    _minMaxRandomStretch = Range<float>(min,max);
}

void AcidGranulator::setOutputDuration(long milliseconds)
{
    _newDuration = milliseconds;
}

void AcidGranulator::setEnvelopeType(int type)
{
    _envelopeType = type;
}

void AcidGranulator::setVoices(int numVoices)
{
    _voices = numVoices;
}

void AcidGranulator::setSampleRate(int newSampleRate)
{
    _sampleRate = newSampleRate;
}

bool AcidGranulator::process(AcidAudioBuffer *input, AcidAudioBuffer *outputTo, long newDurationInMillis)
{
    _newDuration = newDurationInMillis;
    int newSamplesTotal = (_sampleRate * (_newDuration/(float)1000));

    outputTo->clear();
    jassert(newSamplesTotal > 0);
    outputTo->setSize(input->getNumChannels(), newSamplesTotal);
    
    //granulating loop
    //WRITE TO MULTIPLE CHANNELS (OR 1)
    for(int channel = 0; channel < outputTo->getNumChannels(); channel++)
    {
        //WRITE MULTIPLE VOICES (OR 1)
        for(int voice = 1; voice <= _voices; voice++)
        {
            const float* readingHead = input->getReadPointer(channel);
            float* writingHead = outputTo->getWritePointer(channel);

            int grainWriteIndex = 0,
                envIndex;
            int grainCount = 0;
            
            //WHILE EVERY SAMPLE IS WRITTEN FOR CURRENT VOICE IN CURRENT CHANNEL
            while(grainWriteIndex < newSamplesTotal-1)
            {
                if(_minMaxRandomGrain.getLength() != 0) //if random ranges are selected, generate random grain size
                {
                    int randomGrain = acidUtils.randomBiasedIntRange(_minMaxRandomGrain.getStart(), _minMaxRandomGrain.getEnd()+1, userSetGrain, 1.0); //1.0(max) bias strength
                    setGrainSizeSamples(randomGrain);
                }
                else
                {//if grainSize is static, but stretch is not, grain needs to be reset each iteration in order to avoid accumulation
                    setGrainSizeSamples(userSetGrain);
                }
                
                if(_minMaxRandomStretch.getLength() != 0.0) //if random stretch is selected, generate random coefficients
                {//limiting random float to a range
                    float randomStretch = acidUtils.randomBiasedFloatRange(_minMaxRandomStretch.getStart(), _minMaxRandomStretch.getEnd(), userSetStretch, 1.0);
                    setStretch(randomStretch);
                }
                
                float calcRatio = 0;//in case a stretch needs to be performed
                int readGrainForStretch; //original grain size pre-stretch, that will be stretched into grain * stretch and stored in _grainSize;
                if(_stretch != 1.0)
                {
                    calcRatio = _grainSize / (float)(_grainSize*_stretch);
                    readGrainForStretch = _grainSize; //save original grainsize, before its stretched
                    setGrainSizeSamples(_grainSize*_stretch);
                }
                
                int offset = 0;
                if(voice > 1) offset = (_grainSize/_voices); //when multiple voices - offset the grain position for >1 voices
                if(_grainSize >= input->getNumSamples()) setGrainSizeSamples(input->getNumSamples()-1); //if grain is bigger than whole file, set grain to be file size - 1;
                int randomPos = acidUtils.randomIntRange(0, input->getNumSamples()-_grainSize) + offset;
                int finalGrainPos = randomPos+_grainSize;
                
                if(finalGrainPos >= input->getNumSamples())
                {   //if grain final pos +offset bigger than buffer -> subtract the overflow from grains size to fit it in
                    setGrainSizeSamples(_grainSize - (finalGrainPos - input->getNumSamples()-1));
                    finalGrainPos = input->getNumSamples();
                }
                
                const float* envelope = envelopeBuffer.getReadPointer(0);
                envIndex = 0;

                if((grainWriteIndex + _grainSize) < newSamplesTotal-1) //check that new grain won't go out of new buffer's size
                {
                    ++grainCount;
                    for(float grainReadIndex = randomPos; grainReadIndex < finalGrainPos;) //read grain data and write/interp it
                    {
                        AcidResampler interpolator;
                        //NO STRETCH
                        if(_stretch == 1.0)
                        {
                            float voicedSample = readingHead[(int)grainReadIndex++];// * (1.0f/_voices);
                            writingHead[grainWriteIndex++] += voicedSample * envelope[envIndex++];
                        }
                        //STRETCH THE GRAIN
                        else
                        {
                            grainReadIndex += calcRatio;
                            int flooredIncr = floorf(grainReadIndex);
                            float mu = grainReadIndex - flooredIncr;
                            //DBG(String::formatted("floored incr: %d / %D", flooredIncr, input->getNumSamples()));
                            if(flooredIncr+1 < input->getNumSamples())
                            {
                                if(envIndex > _grainSize) continue;
                                float interpolatedSample = interpolator.interpolate(2, readingHead, flooredIncr, input->getNumSamples()-1, mu);// * (1/_voices);
                                jassert(grainWriteIndex < newSamplesTotal);
                                jassert(envIndex < envelopeBuffer.getNumSamples());
                                if(grainWriteIndex + 1 < newSamplesTotal) writingHead[grainWriteIndex++] += interpolatedSample * envelope[envIndex++];
                            }
                        }
                    }
                    grainWriteIndex = ((grainWriteIndex - _grainSize/2) < 0) ? 0 : grainWriteIndex - _grainSize/2; //go back half a grain to overlap the next one
                    DBG(String::formatted("write index - grain/2 = %d - %d = %d",grainWriteIndex + _grainSize/2, _grainSize/2, grainWriteIndex));
                } //proper writing condition
                else
                {
                    //write last reduced grain if set grain was too big for new buffer
                    int samplesRemaining = (grainWriteIndex + _grainSize) - newSamplesTotal+1;
                    finalGrainPos = randomPos + samplesRemaining;
                    envIndex = envelopeBuffer.getNumSamples() - samplesRemaining; //also adjust the evelope window to properly fade out last grain
                    for(int grainReadIndex = randomPos; grainReadIndex < finalGrainPos;)
                    {
                        if(grainWriteIndex + 1 < outputTo->getNumSamples())
                        {
                            float voicedSample = readingHead[grainReadIndex++] * (1/_voices);
                            writingHead[grainWriteIndex++] += voicedSample * envelope[envIndex++];
                            //std:: cout << "\n\n overflow index: " + String(grainWriteIndex) + "\n\n";
                        }
                        else
                        {
                            break;
                        }
                        //DBG(String::formatted("OFLOW - writingIndex: %d / %d - readingIndex %d / %d (grain: %d)", grainWriteIndex, newSamplesTotal, grainReadIndex, input->getNumSamples(), _grainSize));
                    }
                    //DBG(String::formatted("ZZZwrite index %d + grain %d = %d(+finalpos) / %d", grainWriteIndex, _grainSize, grainWriteIndex+finalGrainPos, newSamplesTotal));
                    grainWriteIndex += 1; //finishing increment
                    //std:: cout << "\n\n saviour shit index: " + String(grainWriteIndex) + "\n\n";
                } //overflow writing condition
            }//whole buffer writing loop
            DBG(String::formatted("total grains on channel %d: %d", channel, grainCount));
            writingHead = nullptr;
            readingHead = nullptr;
        }//voice loop
     }//channel loop
    return true;
}

//int stretchedGrainSize(int grainSize, float stretchRatio)
//{
//    int newGrain = grainSize * stretchRatio;
//    float calcRatio = grainSize / newGrain;
//    float incr = 0;
//    
//    float shit[newGrain];
//    for(int i = 0; i < newGrain; i++)
//    {
//        incr += calcRatio;
//        float bareRatio = incr - floorf(incr);
//        int index = floorf(incr);
//    }
//}



void AcidGranulator::calculateEnvelope(int bufferSize)
{
    envelopeBuffer.clear();
    envelopeBuffer.setSize(1, bufferSize+1);
    float* writingHead = envelopeBuffer.getWritePointer(0);
    for(float i = 0.0; i++ < bufferSize+1;)
    {
        double value = 0;
        switch(_envelopeType)
        {
            case(1): //HANN
                value = 0.5 * (1 - cosf( (2*float_Pi*i) / (bufferSize-1) ));
                break;
            case(2): //HAMMING
                value = 0.53836 - 0.46164 * (cosf((2*float_Pi*i) / (bufferSize - 1)));
                break;
            case(3): //BLACKMAN
                value = 0.42659 - 0.49656 * (cosf((2*float_Pi*i) / (bufferSize - 1))) + 0.076849 * (cosf((4*float_Pi*i) / (bufferSize - 1)));
                break;
            case(4): //BLACKMAN-NUTTALL
                value = 0.355768 - 0.487396 * (cosf((2*float_Pi*i) / (bufferSize - 1))) + 0.144232 * (cosf((4*float_Pi*i) / (bufferSize - 1))) - 0.012604 * (cosf((6*float_Pi*i) / (bufferSize - 1)));
                break;
            case(5): //TRIANGLE
                value = 1.0 - std::fabs(((i - ((bufferSize-1) /2))) / ((bufferSize+1)/2));
                break;
            default:
                _envelopeType = 1; //if no envelope type defined, sets as 0 and resends the method msg
                calculateEnvelope(bufferSize);
                break;
        }
        writingHead[(int)i] = value;
    }
}

