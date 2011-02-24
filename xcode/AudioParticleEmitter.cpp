/*
 *  AudioParticleEmitter.cpp
 *  kinectBasic
 *
 *  Created by Bill Lindmeier on 2/23/11.
 *  Copyright 2011 R/GA. All rights reserved.
 *
 */

#include "AudioParticleEmitter.h"
#include "AudioParticle.h"

using namespace std;

AudioParticleEmitter::AudioParticleEmitter()
{
    mMaxParticles	= 1000;
}

void AudioParticleEmitter::addParticle(const ci::Vec2i &loc, float depth)
{    
    AudioParticle particle(loc, depth);		
    mParticles.push_back(particle);
	
	// If we've exceeded our limit, pop the first one
	int overflow = mParticles.size() - mMaxParticles;
	if(overflow > 0){
		for(int p=0;p<overflow;p++){
			mParticles.pop_front();
		}
	}
}

void AudioParticleEmitter::draw()
{
	for(list<AudioParticle>::iterator p = mParticles.begin(); p != mParticles.end(); ++p){
		p->draw();
	}
}

void AudioParticleEmitter::update()
{
	for(list<AudioParticle>::iterator p = mParticles.begin(); p != mParticles.end();){
		if(1){
            p->update();
			p++;
		}else{
			// TODO:
			// Remove the particle from the collection when it has died
			p = mParticles.erase(p);
		}		
	}    
}
