#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Surface.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "cinder/Rand.h"
#include "cinder/params/Params.h"
#include "cinder/ip/Resize.h"
#include "cinder/audio/Output.h"
#include "cinder/audio/Callback.h"
#include "cinder/CinderMath.h"

#include "Kinect.h"
#include "CinderOpenCV.h"

using namespace ci;
using namespace ci::app;
using namespace std;


class kinectBasicApp : public AppBasic {
public:
	void prepareSettings( Settings* settings );
	void setup();
	//	void mouseUp( MouseEvent event );
	void mouseDown( MouseEvent event );
	void update();
	void draw();
	gl::Texture threshholdTexture(ImageSourceRef depthImage);
	Surface colorThreshholdChannels(Surface rgbImage);
	//	void colorThreshholdChannels(Surface rgbImage);
	Channel thresholdChannel(Channel channel);
	Surface surfaceFromChannel(Surface rgbImage, int rgb);
	gl::Texture threshholdTextureFromChannelSurface(Surface rSurface);
	Surface threshholdSurfaceFromChannelSurface(Surface rSurface);
	
	Kinect				mKinect;
	gl::Texture			mTextureTopLeft, mTextureTopRight, mTextureBottomLeft, mTextureBottomRight;
	float				mThreshold, mBlobMin, mBlobMax;
	float				xCalibration, yCalibration, depthCalibration, scaleCalibration;
	float				pointerX, pointerY;
	int					mBlur, mVthresh, mHthresh, mSthresh;
	int					maxColorThresh, minColorThresh;
	int					mColorMode, minSaturation, minVal;
	int					depthPointerX, depthPointerY;
	uint8_t pointerDepth;
	params::InterfaceGl	mParams;
	
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
	
//	xCalibration, yCalibration, depthCalibration = 0.0;
//	scaleCalibration = 1.0;
	
	xCalibration = -22.0;
	yCalibration = -34.0;
	scaleCalibration = 1.08;	
	pointerX = -100; //100.0;
	pointerY = -100; //100.0;
	depthPointerX = -100;
	depthPointerY = -100;
	
	settings->setWindowSize( 1280, 800); //960 );
	mThreshold = 70.0f;
	mBlur = 10.0;
	mBlobMin = 5.0;
	mBlobMax = 100.0;
	mParams = params::InterfaceGl("WakaWaka", Vec2i(200,100));
	
	mParams.addParam( "minColorThresh", &minColorThresh, "min=0.0 max=180.0 step=5.0 keyIncr=2 keyDecr=1");
	mParams.addParam( "maxColorThresh", &maxColorThresh, "min=0.0 max=180.0 step=5.0 keyIncr=0 keyDecr=9");
	mParams.addParam( "minSaturation", &minSaturation, "min=0.0 max=255.0 step=5.0 keyIncr=w keyDecr=s");
	mParams.addParam( "minVal", &minVal, "min=0.0 max=255.0 step=5.0 keyIncr=f keyDecr=v");	
	
	/*
	mParams.addParam( "xCalibration", &xCalibration, "min=-100.0 max=100.0 step=1.0 keyIncr=x keyDecr=z");	
	mParams.addParam( "yCalibration", &yCalibration, "min=-100.0 max=100.0 step=1.0 keyIncr=y keyDecr=t");	
	mParams.addParam( "depthCalibration", &depthCalibration, "min=-100.0 max=100.0 step=0.1 keyIncr=d keyDecr=s");	
	mParams.addParam( "scaleCalibration", &scaleCalibration, "min=0.0 max=2.0 step=0.01 keyIncr=k keyDecr=j");		
	mParams.addParam( "pointerX", &pointerX, "min=0.0 max=700.0 step=5.0 keyIncr=2 keyDecr=1");		
	mParams.addParam( "pointerY", &pointerY, "min=0.0 max=700.0 step=5.0 keyIncr=0 keyDecr=9");		
	*/
	//settings->setWindowSize( 640, 480 );
}

void kinectBasicApp::setup()
{
	//console() << "There are " << Kinect::getNumDevices() << " Kinects connected." << std::endl;
	
	mKinect = Kinect( Kinect::Device() ); // the default Device implies the first Kinect connected
	
	
	mMaxFreq = 1000.0f;
	mFreqTarget = 0.0f;
	mPhase = 0.0f;
	mPhaseAdjust = 0.0f;
	
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
		
		ioBuffer->mData[i*ioBuffer->mNumberChannels] = val;
		ioBuffer->mData[i*ioBuffer->mNumberChannels + 1] = val;
	}
}

/*
 void kinectBasicApp::mouseUp( MouseEvent event )
 {
 writeImage( getHomeDirectory() + "kinect_video.png", mKinect.getVideoImage() );
 //	writeImage( getHomeDirectory() + "kinect_depth.png", mKinect.getDepthImage() );
 
 // set tilt to random angle
 //	mKinect.setTilt( Rand::randFloat() * 62 - 31 );
 
 // make the LED yellow
 mKinect.setLedColor( Kinect::LED_YELLOW );
 
 // toggle infrared video
 //	mKinect.setVideoInfrared( ! mKinect.isVideoInfrared() );
 }
 */

Surface kinectBasicApp::colorThreshholdChannels(Surface rgbImage){
	
	Surface newSurface(rgbImage.getWidth(), rgbImage.getHeight(), true);
	Channel rChannel = this->thresholdChannel(rgbImage.getChannelRed());
	Channel gChannel = this->thresholdChannel(rgbImage.getChannelGreen());
	Channel bChannel = this->thresholdChannel(rgbImage.getChannelBlue());
	
	Surface::Iter sIter = newSurface.getIter();
	Channel::Iter rIter = rChannel.getIter();
	Channel::Iter gIter = gChannel.getIter();
	Channel::Iter bIter = bChannel.getIter();
	int x = -1;
	int y = -1;
	while( sIter.line()){ // && rIter.line() && gIter.line() && bIter.line() ) {
		y++;
		while( sIter.pixel()){ // && rIter.pixel() && gIter.pixel() && bIter.pixel() ) {
			x++;
			sIter.r() = rIter.vClamped(x, y); //rIter. v();
			sIter.g() = gIter.vClamped(x, y); //gIter.v();
			sIter.b() = bIter.vClamped(x, y); //bIter.v();			
			sIter.a() = 255;
		}
	}
	
	return newSurface;
}

Channel kinectBasicApp::thresholdChannel(Channel channel){
	cv::Mat chInput(toOcv(channel)), chBlurred, chThreshholded;
	cv::blur(chInput, chBlurred, cv::Size(mBlur,mBlur));	
	cv::threshold( chBlurred, chThreshholded, mThreshold, 255, CV_THRESH_BINARY );
	return Channel(fromOcv(chThreshholded));
}

// NOTE: This should probably take a return Surfaces 
gl::Texture kinectBasicApp::threshholdTexture(ImageSourceRef depthImage){
	/*	
	 cv::Mat input(toOcv(Channel8u(depthImage))), blurred, thresholded, thresholded2, output;
	 
	 cv::blur(input, blurred, cv::Size(mBlur,mBlur));
	 
	 // make two thresholded images one to display and one
	 // to pass to find contours since its process alters the image
	 cv::threshold( blurred, thresholded, mThreshold, 255, CV_THRESH_BINARY);
	 
	 return gl::Texture(fromOcv(thresholded));
	 */	
	
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
//		console() << "radius: " << radius;
		if (radius > mBlobMin && radius < mBlobMax) {
			cv::circle(output, center, radius, color);
			
			// Lets point to the first/last one we find
			// NOTE: Adding the radius to account for the camera parallax offset.
			// The closer the ball is, the more pronounced the offset is so we can use 
			// the radius as a rough guide
			pointerX = math<int>::clamp(center.x + xCalibration, 0, 640); //math<int>::clamp( (center.x + (xCalibration / scaleCalibration)) * scaleCalibration, 0.0, 640 );
			pointerY = math<int>::clamp(center.y + yCalibration, 0, 480); // math<int>::clamp( (center.y + (yCalibration / scaleCalibration)) * scaleCalibration, 0.0, 480 );
			
			depthCalibration = (radius * 0.75);
			depthPointerX = pointerX + depthCalibration;
			depthPointerY = pointerY;

//			console() << "x " << (center.x + ((xCalibration / scaleCalibration)) * scaleCalibration) << " y " << ((center.y + (yCalibration / scaleCalibration)) * scaleCalibration);
		}
	}
	
	return gl::Texture(fromOcv(output));
	
}

void kinectBasicApp::update()
{	
	if( mKinect.checkNewDepthFrame() ){
		mTextureTopLeft = mKinect.getDepthImage();
		//mTextureTopLeft = this->threshholdTexture(mKinect.getDepthImage());
		
		cv::Mat input(toOcv(Channel8u(mKinect.getDepthImage()))), blurred;
		cv::blur(input, blurred, cv::Size(mBlur, mBlur));
		Channel8u blurry(fromOcv(blurred));

		int depthTotal = 0;
		int halfBlur = (mBlur * 0.5);
		// NOTE: Getting the average depth over the area of blur squared.
		// This might not be necessary since the data is already blurred
		/*
		for(int x=pointerX-halfBlur;x<pointerX+halfBlur;x++){
			for(int y=pointerY-halfBlur;y<pointerX+halfBlur;y++){
				depthTotal += *blurry.getData(Vec2i(pointerX, pointerY));
			}
		}

		pointerDepth = depthTotal / (mBlur * mBlur);
		*/

		if(depthPointerX >= 0 && depthPointerX < blurry.getSize().x &&
		   depthPointerY >= 0 && depthPointerY < blurry.getSize().y){

			pointerDepth = *blurry.getData(Vec2i(depthPointerX, depthPointerY));			
			// Set the tone according to the pointer depth
			mFreqTarget = math<float>::clamp( pointerDepth / 255.0 * mMaxFreq, 0.0, mMaxFreq );
			
		}else{
			
			mFreqTarget = 0.0;
			
		}

//		console() << "pointerX: " << pointerX << " pointerY: " << pointerY << std::endl;		
//		console() << "pointerDepth: " << int(pointerDepth) << " mFreqTarget: " << mFreqTarget << std::endl;
		
	}
	
	if( mKinect.checkNewVideoFrame() ){

		Surface rgbImage(mKinect.getVideoImage());
		
		mTextureBottomLeft = rgbImage;

		Surface scaledRGB(int(640.0 * scaleCalibration), int(480.0 * scaleCalibration), false);
		ci::ip::resize(rgbImage, &scaledRGB);
		
		mTextureTopRight = scaledRGB;
		
		// HSV Detection
//		cv::Mat input(toOcv(rgbImage)), img_hsv_;

		// NOTE: Using the resized image to track the color
		cv::Mat input(toOcv(scaledRGB)), img_hsv_;
		
		cv::cvtColor(input, img_hsv_, CV_RGB2HSV);

		Surface output(img_hsv_.cols, img_hsv_.rows, false);
		// cv::Mat output(640, 480, CV_8UC3);
		
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
					/*
					output.at<uchar>(i,j*3+0) = 255;
					output.at<uchar>(i,j*3+1) = 255;
					output.at<uchar>(i,j*3+2) = 255;
					 */
				} else {
					output.setPixel(Vec2i(j,i), Color8u(0,0,0));
					/*
					output.at<uchar>(i,j*3+0) = 0;
					output.at<uchar>(i,j*3+1) = 0;
					output.at<uchar>(i,j*3+2) = 0;
					*/ 
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

		//cv::Mat input(toOcv(Channel8u(depthImage))), blurred, thresholded, thresholded2, output;
		
		// mTextureBottomRight = this->threshholdTexture(fromOcv(output));
		mTextureBottomRight = this->threshholdTexture(output);

	}
	
	//	console() << "Accel: " << mKinect.getAccel() << std::endl;
}

Surface kinectBasicApp::surfaceFromChannel(Surface rgbImage, int rgb){
	Surface rSurface(rgbImage.getWidth(), rgbImage.getHeight(), true);//(rgbImage.getChannelRed());
	Channel rChannel;// = rgbImage.getChannelRed();
	
	switch (rgb) {
		case 0:
			rChannel = rgbImage.getChannelRed();
			break;
		case 1:
			rChannel = rgbImage.getChannelGreen();
			break;
		case 2:
			rChannel = rgbImage.getChannelBlue();
			break;
		case 3:	
			rChannel = rgbImage.getChannelAlpha();
			break;
	}
	
	Channel::Iter chIter = rChannel.getIter(); // using const because we're not modifying it
	Surface::Iter surIter( rSurface.getIter() ); // not using const because we are modifying it
	while( chIter.line() && surIter.line() ) { // line by line
		while( chIter.pixel() && surIter.pixel() ) { // pixel by pixel
			int chValue = chIter.v();
			surIter.r() = chValue;
			surIter.g() = chValue;
			surIter.b() = chValue;
			surIter.a() = 255.0;
		}
	}
	return rSurface;
}

gl::Texture kinectBasicApp::threshholdTextureFromChannelSurface(Surface rSurface)
{
	cv::Mat input(toOcv(Channel(rSurface))), blurred, thresholded, thresholded2, output;
	// NOTE: Stretches out image
	//			cv::Mat input(toOcv(rgbImage.getChannelRed())), blurred, thresholded, thresholded2, output;
	cv::blur(input, blurred, cv::Size(mBlur, mBlur));
	cv::threshold( blurred, thresholded2, mThreshold, 255, CV_THRESH_BINARY);
	cv::cvtColor(thresholded2, output, CV_GRAY2RGB );
	return gl::Texture(fromOcv(output));
}

Surface kinectBasicApp::threshholdSurfaceFromChannelSurface(Surface rSurface)
{
	cv::Mat input(toOcv(Channel(rSurface))), blurred, thresholded, thresholded2, output;
	// NOTE: Stretches out image
	//			cv::Mat input(toOcv(rgbImage.getChannelRed())), blurred, thresholded, thresholded2, output;
	cv::blur(input, blurred, cv::Size(mBlur, mBlur));
	cv::threshold( blurred, thresholded2, mThreshold, 255, CV_THRESH_BINARY);
	cv::cvtColor(thresholded2, output, CV_GRAY2RGB );
	return Surface(fromOcv(output));
}

void kinectBasicApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
	gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
	
	if( mTextureTopRight )
		gl::draw( mTextureTopRight, Vec2i( 640 + xCalibration, 0 + yCalibration) );
	if( mTextureTopLeft )
		gl::draw( mTextureTopLeft, Vec2i(0, 0) );
	if( mTextureBottomLeft )
		gl::draw( mTextureBottomLeft, Vec2i( 0, 480 ) );
	if( mTextureBottomRight )
		gl::draw( mTextureBottomRight, Vec2i( 640, 480 ) );
	
/*
	// Translate the point point FROM the scaled image TO the color image
	 
	float calibratedX = ((xCalibration / scaleCalibration) * -1) + (pointerX / scaleCalibration);
	float calibratedY = ((yCalibration / scaleCalibration) * -1) + (pointerY / scaleCalibration);
	
	// The closer the point is to the camera, the more the X should be negatively offset, 
	// since the lenses are offset on the xAxis.
	// NOTE: depthOffsetMultiplier is just a shot in the dark.
	// Roughly, 255 (white) * 0.05 = 12.75 pixels
	// float depthOffsetMultiplier = 0.05;
	// calibratedX -= (pointerDepth * depthOffsetMultiplier);
*/	

	gl::drawSolidCircle(Vec2f(640+pointerX, pointerY), 10.0);
	gl::drawSolidCircle(Vec2f(depthPointerX, depthPointerY), 10.0);	 
		
	params::InterfaceGl::draw();
	
}


CINDER_APP_BASIC( kinectBasicApp, RendererGl )
