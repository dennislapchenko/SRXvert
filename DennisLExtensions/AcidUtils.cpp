//
//  AcidUtils.cpp
//  SRXvert
//
//  Created by Dennis Lapchenko on 15/05/2016.
//
//

#include "AcidUtils.h"
#include "AcidHeader.h"

using namespace AcidR;

AcidUtils::AcidUtils()
{}

AcidUtils::~AcidUtils()
{}

int AcidUtils::randomIntRange(int min, int max)
{
    if(max < min) juce::swapVariables(min, max);
    int result = Random().nextFloat() * (max - min) + min;
    //DBG(String::formatted("min: %d max: %d result: %d", min, max, result));
    return result;
}
        
float AcidUtils::randomFloatRange(float min, float max)
{
    if(max < min) juce::swapVariables(min, max);
    
    return Random().nextFloat() * (max - min) + min;
}
        
//random int range biased towards argument, with heaviness of bias defined my infl(1.0 - heavyy)
int AcidUtils::randomBiasedIntRange(int min, int max, int bias, float influence)
{
    //fail safes
    if(influence > 1.0) influence = 1.0;
    if(influence < 0.0) influence = 0.0;
    if(bias > max) bias = max;
    if(bias < min) bias = min;
    if(max < min) juce::swapVariables(min, max);
            
    float rand = Random().nextFloat() * (max - min) + min;
    float mix = Random().nextFloat() * influence;
    return int(rand * (1 - mix) + bias * mix);
}
        
float AcidUtils::randomBiasedFloatRange(float min, float max, float bias, float influence)
{
    //fail safes
    if(influence > 1.0) influence = 1;
    if(influence < 0.0) influence = 0;
    if(bias > max) bias = max;
    if(bias < min) bias = min;
    if(max < min) juce::swapVariables(min, max);
    
    float rand = Random().nextFloat() * (max - min) + min;
    float mix = Random().nextFloat() * influence;
    return rand * (1 - mix) + bias * mix;
}
