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
	
	gl::Texture			threshholdTexture(ImageSourceRef depthImage);
	Channel				thresholdChannel(Channel channel);
	
	Kinect				mKinect;
	gl::Texture			mTextureTopLeft, mTextureTopRight, mTextureBottomLeft, mTextureBottomRight;
	//gl::Texture			mTextureTopLeft;
	float				mThreshold, mBlobMin, mBlobMax;
	float				xCalibration, yCalibration, depthCalibration, scaleCalibration;
	float				pointerX, pointerY;
	int					mBlur, mVthresh, mHthresh, mSthresh;
	int					maxColorThresh, minColorThresh;
	int					mColorMode, minSaturation, minVal;
	int					depthPointerX, depthPointerY;
	uint8_t pointerDepth;
	params::InterfaceGl	mParams;
	ParticleEmitter		mParticleEmitter;
	Channel8u			mDepthChannel;
	Surface				mRGBSurface;
	Vec2f				mPointerVel;
	
	// Audio
	void sineWave( uint64_t inSampleOffset, uint32_t ioSampleCount, audio::Buffer32f *ioBuffer );

	float mFreqTarget;
	float mPhase;
	float mPhaseAdjust;
	float mMaxFreq;
	
};

void kinectBasicApp::prepareSettings( Settings* settings )
{
	
	mColorMode = 0;
	minVal = 100;
	minSaturation = 100;
	minColorThresh = 0;
	maxColorThresh = 180;
	mVthresh = 200;
	mHthresh = 500;
	mSthresh = 500;
	
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
		
	// Orange Ping Pong Ball
	minColorThresh = 100;
	maxColorThresh = 130;
	minSaturation  = 170;
	minVal		   = 180; // Val == Darkness	
	
	xCalibration = -22.0;
	yCalibration = -34.0;
	scaleCalibration = 1.08;	
	pointerX = -100; //100.0;
	pointerY = -100; //100.0;
	depthPointerX = -100;
	depthPointerY = -100;
	
	settings->setWindowSize( 1280, 960 ); // 480
	mThreshold = 70.0f;
	mBlur = 10.0;
	mBlobMin = 5.0;
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
	mMaxFreq = 1000.0f;
	mFreqTarget = 0.0f;
	mPhase = 0.0f;
	mPhaseAdjust = 0.0f;
	
	// Audio reactive
//	audio::Output::play( audio::createCallback( this, &kinectBasicApp::sineWave ) );
	
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
		
		ioBuffer->mData[i*ioBuffer->mNumberChannels] = val;
		ioBuffer->mData[i*ioBuffer->mNumberChannels + 1] = val;
	}
}

Channel kinectBasicApp::thresholdChannel(Channel channel){
	cv::Mat chInput(toOcv(channel)), chBlurred, chThreshholded;
	cv::blur(chInput, chBlurred, cv::Size(mBlur,mBlur));	
	cv::threshold( chBlurred, chThreshholded, mThreshold, 255, CV_THRESH_BINARY );
	return Channel(fromOcv(chThreshholded));
}

gl::Texture kinectBasicApp::threshholdTexture(ImageSourceRef depthImage){

	cv::Mat input(toOcv(Channel8u(depthImage))), blurred, thresholded, thresholded2, output;
	cv::blur(input, blurred, cv::Size(mBlur,mBlur));
	
	cv::threshold( blurred, thresholded, mThreshold, 255, CV_THRESH_BINARY);
	cv::threshold( blurred, thresholded2, mThreshold, 255, CV_THRESH_BINARY);
	
	vector< vector<cv::Point> > contours;
	cv::findContours(thresholded, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
	cv::cvtColor(thresholded2, output, CV_GRAY2RGB );
	
	for (vector<vector<cv::Point> >::iterator it=contours.begin() ; it < contours.end(); it++ ){
		cv::Point2f center;
		float radius;
		vector<cv::Point> pts = *it;
		cv::Mat pointsMatrix = cv::Mat(pts);
		cv::minEnclosingCircle(pointsMatrix, center, radius);
		cv::Scalar color( 0, 255, 0 );
		if (radius > mBlobMin && radius < mBlobMax) {
			cv::circle(output, center, radius, color);
			
			// Lets point to the first/last one we find
			// NOTE: Adding the radius to account for the camera parallax offset.
			// The closer the ball is, the more pronounced the offset is so we can use 
			// the radius as a rough guide
			int newPointerX = math<int>::clamp(center.x, 0, 640);
			int newPointerY = math<int>::clamp(center.y, 0, 480);
			
			if(pointerX != -100){
				// Get the pointer velocity if it's already on stage
				mPointerVel = Vec2i(pointerX, pointerY) - Vec2i(newPointerX, newPointerY);
			}else{
				mPointerVel = Vec2i(0,0);
			}
			
			pointerX = newPointerX;
			pointerY = newPointerY;
			
			depthCalibration = (radius * 0.75);
			depthPointerX = pointerX + depthCalibration;
			depthPointerY = pointerY;

		}
	}
	
	return gl::Texture(fromOcv(output));
	
}

void kinectBasicApp::update()
{	
	if( mKinect.checkNewDepthFrame() ){

		mTextureTopLeft = mKinect.getDepthImage();
		
		cv::Mat input(toOcv(mKinect.getDepthImage())), blurred;
		cv::blur(input, blurred, cv::Size(mBlur, mBlur));
//		Channel8u blurry(fromOcv(blurred));
		mDepthChannel = fromOcv(blurred);

		if(depthPointerX >= 0 && depthPointerX < mDepthChannel.getSize().x &&
		   depthPointerY >= 0 && depthPointerY < mDepthChannel.getSize().y){

			pointerDepth = *mDepthChannel.getData(Vec2i(depthPointerX, depthPointerY));			
			// Set the tone according to the pointer depth
			mFreqTarget = math<float>::clamp( pointerDepth / 255.0 * mMaxFreq, 0.0, mMaxFreq );
			
		}else{
			
			mFreqTarget = 0.0;
			
		}
		
	}
	
	if( mKinect.checkNewVideoFrame() ){

		Surface rgbImage(mKinect.getVideoImage());
		
//		mTextureBottomLeft = rgbImage;

		// Resize and crop the rgb to callibrate
		mRGBSurface = Surface(640, 480, false);
		int x1 = xCalibration / scaleCalibration * -1.0;
		int y1 = yCalibration / scaleCalibration * -1.0;
		int x2 = x1 + (640 / scaleCalibration);
		int y2 = x1 + (480 / scaleCalibration);
		ci::ip::resize(rgbImage, Area(x1,y1,x2,y2), &mRGBSurface, Area(0,0,640,480));

		mTextureTopRight = mRGBSurface;
		
		// NOTE: Using the resized image to track the color
		cv::Mat input(toOcv(mRGBSurface)), img_hsv_;
		
		cv::cvtColor(input, img_hsv_, CV_RGB2HSV);

		Surface output(img_hsv_.cols, img_hsv_.rows, false);
		
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
					output.setPixel(Vec2i(j,i), Color8u(255,255,255));
				} else {
					output.setPixel(Vec2i(j,i), Color8u(0,0,0));
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

		threshholdTexture(output);
//		mTextureBottomRight = threshholdTexture(output);

	}
	
	// Add particles to the mix
	// NOTE: We're always adding 1 particle per frame, even if the ball isn't on the screen	
	mParticleEmitter.addParticles(1, Vec2i(pointerX, pointerY), mPointerVel);
	mParticleEmitter.update(mDepthChannel, mRGBSurface);
}

void kinectBasicApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
	gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
	
	if( mTextureTopLeft )
		gl::draw( mTextureTopLeft, Vec2i(0, 0) );
	if( mTextureTopRight )
		//gl::draw( mTextureTopRight, Vec2i( 640 + xCalibration, 0 + yCalibration) );
		gl::draw( mTextureTopRight, Vec2i( 640, 0) );
	if( mTextureBottomLeft )
		gl::draw( mTextureBottomLeft, Vec2i( 0, 480 ) );
	if( mTextureBottomRight )
		gl::draw( mTextureBottomRight, Vec2i( 640, 480 ) );

	gl::drawSolidCircle(Vec2f(640+pointerX, pointerY), 10.0);
	gl::drawSolidCircle(Vec2f(depthPointerX, depthPointerY), 10.0);	 
		
	params::InterfaceGl::draw();
	
	mParticleEmitter.draw();
	
}


CINDER_APP_BASIC( kinectBasicApp, RendererGl )
