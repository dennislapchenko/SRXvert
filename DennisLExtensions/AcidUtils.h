//
//  AcidUtils.h
//  SRXvert
//
//  Created by Dennis Lapchenko on 15/05/2016.
//
//

#ifndef ACIDUTILS_H
#define ACIDUTILS_H

//#include "AcidHeader.h"

/*
 Had this as a simply static header first, 
 but due to these methods being called for every grain
 it was very inefficient. Therefore moved to an inline class
 */
namespace AcidR
{
    class AcidUtils
    {
    public:
        AcidUtils();
        ~AcidUtils();
        
        int randomIntRange(int min, int max);
        
        float randomFloatRange(float min, float max);
        
        //random int range biased towards argument, with heaviness of bias defined my infl(1.0 - heavyy)
        int randomBiasedIntRange(int min, int max, int bias, float influence);
        
        float randomBiasedFloatRange(float min, float max, float bias, float influence);
    };
}

#endif /* ACIDUTILS_H */
