/*
 *  Particle.h
 *  kinectBasic
 *
 *  Created by William Lindmeier on 2/22/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "cinder/Vector.h"
#include "cinder/Channel.h"
#include "cinder/Surface.h"

class Particle {
	
public:
	
	float		mRadius;
	ci::Vec2f	mLoc, mVel;
	
	Particle();
	Particle(const ci::Vec2f &loc);
	void update(const ci::Channel8u &depthChannel, const ci::Surface8u &rgbSurface);
	void draw();
	
};