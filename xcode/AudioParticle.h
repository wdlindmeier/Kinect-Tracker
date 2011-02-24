/*
 *  AudioParticle.h
 *  kinectBasic
 *
 *  Created by Bill Lindmeier on 2/23/11.
 *  Copyright 2011 R/GA. All rights reserved.
 *
 */

#ifndef AUDIO_PARTICLE
#define AUDIO_PARTICLE

#include "Particle.h"

class AudioParticle : public Particle {
    
public:
    
    AudioParticle(const ci::Vec2f &loc, float aDepth);
    void update();
    void draw();
    float   depth;
    float   age;
    
};

#endif