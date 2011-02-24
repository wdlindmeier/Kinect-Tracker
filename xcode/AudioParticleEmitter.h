/*
 *  AudioParticleEmitter.h
 *  kinectBasic
 *
 *  Created by Bill Lindmeier on 2/23/11.
 *  Copyright 2011 R/GA. All rights reserved.
 *
 */

#include "AudioParticle.h"
#include <list>

class AudioParticleEmitter {
    
public:
    
    AudioParticleEmitter();
    void update();
    void addParticle(const ci::Vec2i &loc, float depth);
    void draw();
    
    std::list<AudioParticle>	mParticles;
    int                         mMaxParticles;

};