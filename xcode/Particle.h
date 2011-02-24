/*
 *  Particle.h
 *  kinectBasic
 *
 *  Created by William Lindmeier on 2/22/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PARTICLE
#define PARTICLE

#include "cinder/Vector.h"
#include "cinder/Channel.h"
#include "cinder/Surface.h"
#include "cinder/Color.h"

class Particle {
	
public:
	
	float		mRadius, mMass;
	bool		isDead;
	ci::Vec2f	mLoc, mVel, mAcc;
	ci::Color8u	mColor;
	
	Particle();
	Particle(const ci::Vec2f &loc);
    void update();
	void update(const ci::Channel8u &depthChannel, const ci::Surface8u &rgbSurface);
	void draw();
	void pullToCenter();
	
};

#endif