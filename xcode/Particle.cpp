/*
 *  Particle.cpp
 *  kinectBasic
 *
 *  Created by William Lindmeier on 2/22/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "Particle.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"

using namespace std;
using namespace ci;

Particle::Particle()
{
}

Particle::Particle(const Vec2f &loc)
{
	mLoc = loc;
	mRadius = 1.0;
	mVel = Rand::randVec2f();
}

#define kRadiusMultiplier	3.0

void Particle::update(const Channel8u &depthChannel, const Surface8u &rgbSurface)
{
	// Update it's position
	mLoc += mVel;
	
	// Update it's radius based on the depth image
	// val == 0..255
	if(mLoc.x >= 0 && mLoc.y >= 0 &&
	   mLoc.x < depthChannel.getWidth() &&
	   mLoc.y < depthChannel.getHeight()){
		int val = *depthChannel.getData(Vec2i(mLoc.x, mLoc.y));
		mRadius = ((float)val / 255.0f) * kRadiusMultiplier;
	}
}

void Particle::draw()
{
	gl::drawSolidCircle(mLoc, mRadius);
}