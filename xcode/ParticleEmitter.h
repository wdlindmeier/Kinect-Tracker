/*
 *  ParticleEmitter.h
 *  kinectBasic
 *
 *  Created by William Lindmeier on 2/22/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "cinder/Vector.h"
#include "cinder/Channel.h"
#include "cinder/Surface.h"
#include "Particle.h"
#include <list>

class ParticleEmitter {
	
public:
	
	std::list<Particle>	mParticles;
	int					mMaxParticles;
	
	ParticleEmitter();
	void update(const ci::Channel8u &depthChannel, const ci::Surface8u &rgbSurface);
	void draw();
	void addParticles(int numParticles, const ci::Vec2i &loc, const ci::Vec2f &velocity);
	
};