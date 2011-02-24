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
    app::console() << depth << "\n";
}

#define kRadiusMultiplier	5.0

void AudioParticle::update()
{
    age++;
    // set radius based on depth
//    mRadius += (mRadius * depth) + 1.0;

    // The closer the faster
    mRadius += (depth * 1.5);// + 1.0;

    float amtOffset =  (float)mLoc.x / 640.0f;
    int color = 255 - age;
    // The older it is, the darker it is
    int g = color * amtOffset;
    mColor = Color8u(color, g, color - g);
}

void AudioParticle::draw()
{
	gl::color(mColor);
	// NOTE: Offsetting to put in lower left
	gl::drawSolidCircle(mLoc + Vec2i(0,480), mRadius);
}