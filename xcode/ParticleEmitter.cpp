/*
 *  ParticleEmitter.cpp
 *  kinectBasic
 *
 *  Created by William Lindmeier on 2/22/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "ParticleEmitter.h"
#include "cinder/Rand.h"

using namespace std;
using namespace ci;

ParticleEmitter::ParticleEmitter()
{
	mMaxParticles	= 500;
}

void ParticleEmitter::addParticles(int numParticles, const ci::Vec2i &loc, const ci::Vec2f &velocity)
{
	for(int p=0;p<numParticles;p++){		
		Vec2f pLoc = loc + Rand::randVec2f() * Rand::randFloat( 5.0f );
		Particle particle(pLoc);		
		Vec2f velOffset = Rand::randVec2f() * Rand::randFloat( 1.0f, 3.0f );
		particle.mVel = velocity * 5.0f + velOffset;				
		mParticles.push_back(particle);
	}
	
	// If we've exceeded our limit, pop the first one
	int overflow = mParticles.size() - mMaxParticles;
	if(overflow > 0){
		for(int p=0;p<overflow;p++){
			mParticles.pop_front();
		}
	}
}

void ParticleEmitter::draw()
{
	for(list<Particle>::iterator p = mParticles.begin(); p != mParticles.end(); ++p){
		p->draw();
	}
}

void ParticleEmitter::update(const Channel8u &depthChannel, const Surface8u &rgbSurface)
{
	for(list<Particle>::iterator p = mParticles.begin(); p != mParticles.end();){
		if(1){
			p->update(depthChannel, rgbSurface);
			++p;
		}else{
			// TODO:
			// Remove the particle from the collection when it has died
			p = mParticles.erase(p);
		}		
	}
}