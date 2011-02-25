#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Surface.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "cinder/Rand.h"
#include "cinder/Area.h"
#include "cinder/params/Params.h"
#include "cinder/ip/Resize.h"
#include "cinder/audio/Output.h"
#include "cinder/audio/Callback.h"
#include "cinder/CinderMath.h"

#include "Kinect.h"
#include "CinderOpenCV.h"

#include "ParticleEmitter.h"
#include "AudioParticleEmitter.h"

#define kMinPointerDepth	0.09
#define kFieldWidth			640
#define kFieldHeight		480

using namespace ci;
using namespace ci::app;
using namespace std;


class kinectBasicApp : public AppBasic {
public:
	void prepareSettings( Settings* settings );
	void setup();
	void mouseDown( MouseEvent event );
	void update();
	void draw();
	
	Kinect				mKinect;
	gl::Texture			mTextureTopRight, mTextureBottomLeft, mTextureBottomRight; //, mTextureTopLeft;
	float				mThreshold, mBlobMin, mBlobMax;
	float				xCalibration, yCalibration, depthCalibration, scaleCalibration;
	float				pointerX, pointerY, pointerDepth, volume;
    float               mFreqTarget;
	float               mPhase;
	float               mPhaseAdjust;
	float               mMaxFreq, mMinFreq;    
    float               mDepthCalibrarionRadiusMultiplier;
    bool                peaked;
	int					mBlur, mVthresh, mHthresh, mSthresh;
	int					maxColorThresh, minColorThresh;
	int					mColorMode, minSaturation, minVal;
	int					depthPointerX, depthPointerY;
    int                 whichColor;

	params::InterfaceGl	mParams;
	ParticleEmitter         mParticleEmitter;
    AudioParticleEmitter	mAudioParticleEmitter;
	Channel8u               mDepthChannel;
	Surface                 mRGBSurface;
	Vec2f                   mPointerVel;
	
	// Audio
	void sineWave( uint64_t inSampleOffset, uint32_t ioSampleCount, audio::Buffer32f *ioBuffer );

	
};

void kinectBasicApp::prepareSettings( Settings* settings )
{
    
    volume = 0.0f;
	
	mColorMode = 0;
	minVal = 100;
	minSaturation = 100;
	minColorThresh = 0;
	maxColorThresh = 180;
	mVthresh = 200;
	mHthresh = 500;
	mSthresh = 500;
    
    peaked = false;
	
	// Color tracking
	
	// Spectrum maxes out at 180
	// SO, all spectrum colors are halved
	// Color breakdown (approx):
	
	// RED:			100-140
	// PURPLE/BLUE:	150-180 
	// CYAN:		0-20
	// GREEN:		20-120
	// YELLOW:		100-125
	// ORANGE:		120-140
    
    whichColor      =   2; // 0 == orange ball, 1 == green cap, 2 == yellow ball
		
    switch (whichColor) {
        case 0:
            // Orange Ping Pong Ball
            minColorThresh = 100;
            maxColorThresh = 130;
            minSaturation  = 170;
            minVal		   = 180; // Val == Darkness	
            mDepthCalibrarionRadiusMultiplier = 0.6;
            break;
        case 1:
            // Green Marker Cap
            minColorThresh = 50;
            maxColorThresh = 90;
            minSaturation  = 35;
            minVal		   = 30; // Val == Darkness	
            mDepthCalibrarionRadiusMultiplier = 1.2;            
            break;
        case 2:
            // Yellow Ball
            minColorThresh = 90;
            maxColorThresh = 110;
            minSaturation  = 170;
            minVal		   = 155; // Val == Darkness	    
            mDepthCalibrarionRadiusMultiplier = 0.3;
            break;
    }
	
	xCalibration = -22.0;
	yCalibration = -34.0;
	scaleCalibration = 1.08;	
	pointerX = -100; //100.0;
	pointerY = -100; //100.0;
	depthPointerX = -100;
	depthPointerY = -100;
	pointerDepth = 0.0;
	
	//settings->setWindowSize( 1280, 960 ); // 480
    
    settings->setWindowSize( kFieldWidth, kFieldHeight );
    
	mThreshold = 70.0f;
	mBlur = 10.0;
	mBlobMin = 3.0;
	mBlobMax = 100.0;

	mParams = params::InterfaceGl("WakaWaka", Vec2i(200,100));
	
	mParams.addParam( "minColorThresh", &minColorThresh, "min=0.0 max=180.0 step=5.0 keyIncr=2 keyDecr=1");
	mParams.addParam( "maxColorThresh", &maxColorThresh, "min=0.0 max=180.0 step=5.0 keyIncr=0 keyDecr=9");
	mParams.addParam( "minSaturation", &minSaturation, "min=0.0 max=255.0 step=5.0 keyIncr=w keyDecr=s");
	mParams.addParam( "minVal", &minVal, "min=0.0 max=255.0 step=5.0 keyIncr=f keyDecr=v");	
	
}

void kinectBasicApp::setup()
{

	mKinect = Kinect( Kinect::Device() ); // the default Device implies the first Kinect connected
    
    //mKinect.setTilt(15.5);
    mKinect.setTilt(5.0);
    
	mMaxFreq = 1000.0f;
	mMinFreq = 50.0f;
	mFreqTarget = 0.0f;
	mPhase = 0.0f;
	mPhaseAdjust = 0.0f;
	
	// Audio reactive
	audio::Output::play( audio::createCallback( this, &kinectBasicApp::sineWave ) );

}

void kinectBasicApp::mouseDown( MouseEvent event )
{
	mKinect.setLedColor( Kinect::LED_RED );
}

void kinectBasicApp::sineWave( uint64_t inSampleOffset, uint32_t ioSampleCount, audio::Buffer32f *ioBuffer ) {
	
    mPhaseAdjust = mPhaseAdjust * 0.95f + ( mFreqTarget / 44100.0f ) * 0.05f;
    
	for( int  i = 0; i < ioSampleCount; i++ ) {
        
		mPhase += mPhaseAdjust;
		mPhase = mPhase - math<float>::floor( mPhase );

		float val = math<float>::sin( mPhase * 2.0f * M_PI );
        
        // Changes the tone
        val = math<float>::clamp(val, 0, 1000000);
                
        // Undulate the volume according to depth (closer == faster)
        volume += (0.001 * (pointerDepth * 1.5));        
        val *= sin(volume);

        // No Panning
//		ioBuffer->mData[i*ioBuffer->mNumberChannels] = val;
//		ioBuffer->mData[i*ioBuffer->mNumberChannels + 1] = val;

        // Panning
		float xOffset = (float)pointerX / 640.0;
		float lVal = val * xOffset;
		float rVal = val - lVal;

        // Also make it generally quieter when its further away
        if(pointerDepth > kMinPointerDepth){
            lVal *= pointerDepth;
            rVal *= pointerDepth;            
        }

		ioBuffer->mData[i*ioBuffer->mNumberChannels] = lVal;
		ioBuffer->mData[i*ioBuffer->mNumberChannels + 1] = rVal;
		
	}
}

void kinectBasicApp::update()
{	
    
	if( mKinect.checkNewDepthFrame() ){

		mTextureBottomLeft = mKinect.getDepthImage();
		
		cv::Mat input(toOcv(mKinect.getDepthImage())), blurred;
		cv::blur(input, blurred, cv::Size(mBlur, mBlur));
		mDepthChannel = fromOcv(blurred);

		if(depthPointerX >= 0 && depthPointerX < mDepthChannel.getSize().x &&
		   depthPointerY >= 0 && depthPointerY < mDepthChannel.getSize().y){


			// Set the tone according to pointerX
            float xAmt = 1.0 - (pointerX / kFieldWidth);
			mFreqTarget = math<float>::clamp( mMinFreq + (xAmt * (mMaxFreq - mMinFreq)), mMinFreq, mMaxFreq );
            
            // Set the tone according to momentum
//            float xAmt = mPointerVel.length() / 20.0;
//            mFreqTarget = math<float>::clamp( mMinFreq + (xAmt * (mMaxFreq - mMinFreq)), mMinFreq, mMaxFreq );

            int depth = *mDepthChannel.getData(Vec2i(depthPointerX, depthPointerY));			
			float newPointerDepth = (depth / 255.0);            

			// NOTE: We're just tossing zero depth data
			if(newPointerDepth > kMinPointerDepth) pointerDepth = newPointerDepth;
			
		}else{
			
			mFreqTarget = 0.0;
            mPhaseAdjust = 0.0;
			
		}
		
	}
	
	if( mKinect.checkNewVideoFrame() ){

		Surface rgbImage(mKinect.getVideoImage());
		
		// Resize and crop the rgb to callibrate
		mRGBSurface = Surface(kFieldWidth, kFieldHeight, false);
		int x1 = xCalibration / scaleCalibration * -1.0;
		int y1 = yCalibration / scaleCalibration * -1.0;
		int x2 = x1 + (kFieldWidth / scaleCalibration);
		int y2 = x1 + (kFieldHeight / scaleCalibration);
		ci::ip::resize(rgbImage, Area(x1,y1,x2,y2), &mRGBSurface, Area(0,0,kFieldWidth,kFieldHeight));

		if(app::getWindowWidth() > kFieldWidth) mTextureTopRight = mRGBSurface;
		
		// NOTE: Using the resized image to track the color
		cv::Mat input(toOcv(mRGBSurface)), img_hsv_;
		
		cv::cvtColor(input, img_hsv_, CV_RGB2HSV);

        Channel8u rangeOutput(img_hsv_.cols, img_hsv_.rows);
		
		cv::Mat img_hue_, img_sat_, img_val_;
		img_hue_ = cv::Mat::zeros(img_hsv_.rows, img_hsv_.cols, CV_8UC1);
		img_sat_ = cv::Mat::zeros(img_hsv_.rows, img_hsv_.cols, CV_8UC1);
		img_val_ = cv::Mat::zeros(img_hsv_.rows, img_hsv_.cols, CV_8UC1);
		// HSV Channel 0 -> img_hue_ & HSV Channel 1 -> img_sat_
		int from_to[] = { 0,0, 1,1, 2,2};
		cv::Mat img_split[] = { img_hue_, img_sat_, img_val_};
		cv::mixChannels(&img_hsv_,3,img_split,3,from_to,3);
		
		for(int i = 0; i < img_hsv_.rows; i++)
		{
			for(int j = 0; j < img_hsv_.cols; j++)
			{
				// The output pixel is white if the input pixel
				// hue is orange and saturation is reasonable
				if(img_hue_.at<uchar>(i,j) > minColorThresh &&
				   img_hue_.at<uchar>(i,j) < maxColorThresh &&
				   img_sat_.at<uchar>(i,j) > minSaturation && 
				   img_val_.at<uchar>(i,j) > minVal ) {
					//rangeOutput.setPixel(Vec2i(j,i), Color8u(255,255,255));
                    rangeOutput.setValue(Vec2i(j,i), 255);
				} else {
					//rangeOutput.setPixel(Vec2i(j,i), Color8u(0,0,0));
                    rangeOutput.setValue(Vec2i(j,i), 0);
				}
			}
		}
		
		// Spectrum maxes out at 180
		// SO, all spectrum colors are halved
		// Color breakdown (approx):
		
		// RED:			100-140
		// PURPLE/BLUE:	150-180 
		// CYAN:		0-20
		// GREEN:		20-120
		// YELLOW:		100-125
		// ORANGE:		120-140
		
		// Tracking the red card:
		// minColorThresh = 115
		// maxColorThresh = 145
		// minSaturation  = 180
		// minVal		  = 65

        // TEST
        // putting threshold inline
        
        cv::Mat rangeInput(toOcv(rangeOutput)), blurred, thresholded, output;
        cv::blur(rangeInput, blurred, cv::Size(mBlur,mBlur));        
        cv::threshold( blurred, thresholded, mThreshold, 255, CV_THRESH_BINARY);
		// For display only
		cv::threshold( blurred, output, mThreshold, 255, CV_THRESH_BINARY);
        
        vector< vector<cv::Point> > contours;
        cv::findContours(thresholded, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
        
		float largestHitRadius = 0.0;
		
        for (vector<vector<cv::Point> >::iterator it=contours.begin() ; it < contours.end(); it++ ){
            cv::Point2f center;
            float radius;
            vector<cv::Point> pts = *it;
            cv::Mat pointsMatrix = cv::Mat(pts);
            cv::minEnclosingCircle(pointsMatrix, center, radius);
            
			//cv::Scalar color( 0, 255, 0 );
			
            if (radius > mBlobMin && 
				radius < mBlobMax && 
				radius > largestHitRadius) {
								
				largestHitRadius = radius;
				
				// This shows up as a white line on the Channel
                // cv::circle(output, center, radius, color);
                
                // Lets point to the first/last one we find
                // NOTE: Adding the radius to account for the camera parallax offset.
                // The closer the ball is, the more pronounced the offset is so we can use 
                // the radius as a rough guide
                int newPointerX = math<int>::clamp(center.x, 0, kFieldWidth);
                int newPointerY = math<int>::clamp(center.y, 0, kFieldHeight);
                
                if(pointerX != -100){
                    // Get the pointer velocity if it's already on stage
                    mPointerVel = Vec2i(newPointerX - pointerX, newPointerY - pointerY);// - Vec2i(newPointerX, newPointerY);
                }else{
                    mPointerVel = Vec2i(0,0);
                }
                
                pointerX = newPointerX;
                pointerY = newPointerY;
                
                // This works for a ping pong ball, but not for other things
                depthCalibration = (radius * mDepthCalibrarionRadiusMultiplier);
                
                depthPointerX = pointerX + depthCalibration;
                depthPointerY = pointerY;
                                
            }
        }
                
		if(app::getWindowWidth() > kFieldWidth && app::getWindowHeight() > kFieldHeight) mTextureBottomRight = fromOcv(output);
		
        // The number of particles should be determined by phase.
        // Use the depth 

        // mParticleEmitter.addParticles(30 * pointerDepth + 1, Vec2i(pointerX, pointerY), mPointerVel);            
        // mParticleEmitter.update(mDepthChannel, mRGBSurface);		
        
		if(pointerDepth > kMinPointerDepth){
			
			//mAudioParticleEmitter.addParticle(Vec2i(pointerX, pointerY), pointerDepth);

			// For each point between current and previous, make a particle
			// Divide the number of intermediate particles by the phase (use depth) 
			
			// We expect that the closer you are, the more the gaps are filled from one frame to the next
			float distance = mPointerVel.length();
			int numParticles = (distance * pointerDepth * 0.1) + 1;
			float xInterval = (float)mPointerVel.x / (float)numParticles;
			float yInterval = (float)mPointerVel.y / (float)numParticles;
			for(int p=0;p<numParticles;p++){
				int newX = pointerX - (xInterval * p);
				int newY = pointerY - (yInterval * p);
				mAudioParticleEmitter.addParticle(Vec2i(newX, newY), pointerDepth);
			}
		
		}
        mAudioParticleEmitter.update();
        
	}    
}

void kinectBasicApp::draw()
{

	gl::clear( Color( 0, 0, 0 ) ); 	
	gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
	gl::color(Color::white());

//	if( mTextureTopLeft )
//		gl::draw( mTextureTopLeft, Vec2i(0, 0) );
	if( mTextureTopRight )
		gl::draw( mTextureTopRight, Vec2i( kFieldWidth, 0) );
	if( mTextureBottomLeft )
		gl::draw( mTextureBottomLeft, Vec2i( 0, kFieldHeight ) );
	if( mTextureBottomRight )
		gl::draw( mTextureBottomRight, Vec2i( kFieldWidth, kFieldHeight ) );
     	
	params::InterfaceGl::draw();
	
	gl::drawSolidCircle(Vec2f(kFieldWidth+pointerX, pointerY), 10.0);
	gl::drawSolidCircle(Vec2f(depthPointerX, kFieldHeight + depthPointerY), 10.0);	 
    
	// mParticleEmitter.draw();
    mAudioParticleEmitter.draw();
    
    //console() << "volumePhase: " << volumePhase << std::endl;
	
}


CINDER_APP_BASIC( kinectBasicApp, RendererGl )
