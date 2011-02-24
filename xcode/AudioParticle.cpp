/*
 *  AudioParticle.cpp
 *  kinectBasic
 *
 *  Created by Bill Lindmeier on 2/23/11.
 *  Copyright 2011 R/GA. All rights reserved.
 *
 */

#include "AudioParticle.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"
#include "cinder/app/AppBasic.h"

using namespace ci;

AudioParticle::AudioParticle(const Vec2f &loc, float aDepth)
{
	mLoc		= loc;
	mRadius		= 0.0;
	mVel		= Vec2f::zero();
	mColor		= Color8u(255,255,255);
	mAcc		= Vec2f::zero();
    age         = 0;
    depth       = aDepth;
}

#define kRadiusMultiplier	5.0

void AudioParticle::update()
{
    age++;

    // The closer the faster growing
    mRadius += (depth * 1.5);

    float amtOffset =  1.0 - ((float)mLoc.x / 640.0f);
    int val = 255 - age;
	
    // The older it is, the darker it is
    int r = math<int>::clamp(val * amtOffset, 0, 255);
	int g = math<int>::clamp(val * depth, 0, 255);
	int b = math<int>::clamp(val - r, 0, 255);
    mColor = Color8u(r, g, b);
	
	isDead = age > 255;
}

void AudioParticle::draw()
{
	gl::color(mColor);
	// NOTE: Offsetting to put in lower left
	// NOTE: Mirroring to look like you're controlling it
	gl::drawSolidCircle(Vec2i((640 - mLoc.x),(mLoc.y)), mRadius, 8);
}