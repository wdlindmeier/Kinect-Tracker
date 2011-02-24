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
	mMaxParticles	= 10000;
}

void ParticleEmitter::addParticles(int numParticles, const ci::Vec2i &loc, const ci::Vec2f &velocity)
{
	for(int p=0;p<numParticles;p++){		
		Vec2f pLoc = loc + Rand::randVec2f() * Rand::randFloat( 10.0f );
		Particle particle(pLoc);		
		Vec2f velOffset = Rand::randVec2f() * Rand::randFloat( 1.0f, 3.0f );
		particle.mVel = Rand::randVec2f(); //velocity * 5.0f + velOffset;				
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

void ParticleEmitter::pullToCenter()
{
	for( list<Particle>::iterator p = mParticles.begin(); p != mParticles.end(); ++p ){
		p->pullToCenter();
	}
}


void ParticleEmitter::repulseParticles()
{
	for( list<Particle>::iterator p1 = mParticles.begin(); p1 != mParticles.end(); ++p1 ){
		
		list<Particle>::iterator p2 = p1;
		for( ++p2; p2 != mParticles.end(); ++p2 ) {
			Vec2f dir = p1->mLoc - p2->mLoc;
			
			float thresh = ( p1->mRadius + p2->mRadius ) * 5.0f;
			if( dir.x > -thresh && dir.x < thresh && dir.y > -thresh && dir.y < thresh ){
				float distSqrd = dir.lengthSquared() * dir.length();
				
				if( distSqrd > 0.0f ){
					float F = 1.0f/distSqrd;
					dir.normalize();
					
					// acceleration = force / mass
					p1->mAcc += ( F * dir ) / p1->mMass;
					p2->mAcc -= ( F * dir ) / p2->mMass;

					// TMP
					p1->mAcc *= 0.005;
					p2->mAcc *= 0.005;
					
				}
			}
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
		if(!p->isDead){
			p->update(depthChannel, rgbSurface);
			++p;
		}else{
			p = mParticles.erase(p);
		}		
	}
}