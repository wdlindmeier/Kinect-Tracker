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
	mLoc		= loc;
	mRadius		= 1.0;
	mVel		= Rand::randVec2f() * 3.0;
	mColor		= Color8u(255,255,255);
	mAcc		= Vec2f::zero();
}

void Particle::pullToCenter()
{
	Vec2f dirToCenter = mLoc - Vec2f(320,260); //app::getWindowCenter();
	mAcc -= dirToCenter * mMass;// * 0.025f;
}	

#define kRadiusMultiplier	5.0

void Particle::update(const Channel8u &depthChannel, const Surface8u &rgbSurface)
{
	mVel += mAcc;
	mAcc.set( 0, 0 );
	
	// Update it's position
	mLoc += mVel;

	// Only update if the particle is in the image
	if(mLoc.x >= 0 && mLoc.y >= 0 &&
	   mLoc.x < depthChannel.getWidth() &&
	   mLoc.y < depthChannel.getHeight()){
		
		// Update it's color
		mColor = rgbSurface.getPixel(Vec2i(mLoc.x, mLoc.y));
		
		// Update it's radius based on the depth image
		// val == 0..255		
		int val = *depthChannel.getData(Vec2i(mLoc.x, mLoc.y));
		
		float depth = ((float)val / 255.0f);
		
		mRadius = depth * kRadiusMultiplier;
		
		//mMass = mRadius * mRadius * 0.0001f + 0.01f;
	}
}

void Particle::draw()
{
	gl::color(mColor);
	// NOTE: Offsetting to put in lower left
	gl::drawSolidCircle(mLoc + Vec2i(0,480), mRadius);
}