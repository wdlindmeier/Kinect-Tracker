#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Surface.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "cinder/Rand.h"
#include "cinder/params/Params.h"
#include "cinder/ip/Resize.h"

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
	uint8_t pointerDepth;
	params::InterfaceGl	mParams;
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
	
//	xCalibration, yCalibration, depthCalibration = 0.0;
//	scaleCalibration = 1.0;
	
	xCalibration = -22.0;
	yCalibration = -34.0;
	scaleCalibration = 1.08;	
	pointerX = 100.0;
	pointerY = 100.0;
	
	settings->setWindowSize( 1280, 800); //960 );
	mThreshold = 70.0f;
	mBlur = 10.0;
	mBlobMin = 40.0;
	mBlobMax = 200.0;
	mParams = params::InterfaceGl("WakaWaka", Vec2i(200,100));
	/*
	mParams.addParam( "minColorThresh", &minColorThresh, "min=0.0 max=180.0 step=5.0 keyIncr=2 keyDecr=1");
	mParams.addParam( "maxColorThresh", &maxColorThresh, "min=0.0 max=180.0 step=5.0 keyIncr=0 keyDecr=9");
	mParams.addParam( "minSaturation", &minSaturation, "min=0.0 max=255.0 step=5.0 keyIncr=w keyDecr=s");
	mParams.addParam( "minVal", &minVal, "min=0.0 max=255.0 step=5.0 keyIncr=f keyDecr=v");	
	*/
	mParams.addParam( "xCalibration", &xCalibration, "min=-100.0 max=100.0 step=1.0 keyIncr=x keyDecr=z");	
	mParams.addParam( "yCalibration", &yCalibration, "min=-100.0 max=100.0 step=1.0 keyIncr=y keyDecr=t");	
	mParams.addParam( "depthCalibration", &depthCalibration, "min=-100.0 max=100.0 step=0.1 keyIncr=d keyDecr=s");	
	mParams.addParam( "scaleCalibration", &scaleCalibration, "min=0.0 max=2.0 step=0.01 keyIncr=k keyDecr=j");		
	mParams.addParam( "pointerX", &pointerX, "min=0.0 max=700.0 step=5.0 keyIncr=2 keyDecr=1");		
	mParams.addParam( "pointerY", &pointerY, "min=0.0 max=700.0 step=5.0 keyIncr=0 keyDecr=9");		
	//settings->setWindowSize( 640, 480 );
}

void kinectBasicApp::setup()
{
	console() << "There are " << Kinect::getNumDevices() << " Kinects connected." << std::endl;
	
	mKinect = Kinect( Kinect::Device() ); // the default Device implies the first Kinect connected
}

void kinectBasicApp::mouseDown( MouseEvent event )
{
	mKinect.setLedColor( Kinect::LED_RED );
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
		if (radius > mBlobMin && radius < mBlobMax) {
			cv::circle(output, center, radius, color);
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
		for(int x=pointerX-halfBlur;x<pointerX+halfBlur;x++){
			for(int y=pointerY-halfBlur;y<pointerX+halfBlur;y++){
				depthTotal += *blurry.getData(Vec2i(pointerX, pointerY));
			}
		}
		pointerDepth = depthTotal / (mBlur * mBlur);

	}
	
	if( mKinect.checkNewVideoFrame() ){

		Surface rgbImage(mKinect.getVideoImage());
		
		mTextureBottomLeft = rgbImage;

		Surface scaledRGB(int(640.0 * scaleCalibration), int(480.0 * scaleCalibration), false);
		ci::ip::resize(rgbImage, &scaledRGB);
		
		mTextureTopRight = scaledRGB;
				
		/*
		Surface redImage(640, 480, false), greenImage(640, 480, false), blueImage(640, 480, false);

		// HSV Detection
		cv::Mat input(toOcv(rgbImage)), img_hsv_;
		cv::cvtColor(input, img_hsv_, CV_RGB2HSV);
		//		cv::cvtColor(input, img_hsv_, CV_RGB2HLS);
		Surface output(img_hsv_.rows, img_hsv_.cols, false);
		
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
					//img_bin_.at(i,j) = 255;
					output.setPixel(Vec2i(i,j), Color8u(255,255,255));
				} else {
					output.setPixel(Vec2i(i,j), Color8u(0,0,0));
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
		
		mTextureTopLeft = this->threshholdTexture(output);
		*/		
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
	
	
	// xCalibration, yCalibration, depthCalibration;
	

	//float xPos, yPos = 0.0;
	
	/*
	float scaleCalibratedXOff = (640 - (640 * scaleCalibration)) * 0.5;
	float scaleCalibratedYOff = (480 - (480 * scaleCalibration)) * 0.5;
	*/
	
	float calibratedX = ((xCalibration / scaleCalibration) * -1) + (pointerX / scaleCalibration);
	float calibratedY = ((yCalibration / scaleCalibration) * -1) + (pointerY / scaleCalibration);
	
	// The closer the point is to the camera, the more the X should be negatively offset, 
	// since the lenses are offset on the xAxis.
	// NOTE: depthOffsetMultiplier is just a shot in the dark.
	// Roughly, 255 (white) * 0.05 = 12.75 pixels
	float depthOffsetMultiplier = 0.05;
	calibratedX -= (pointerDepth * depthOffsetMultiplier);
	
	gl::drawSolidCircle(Vec2f(pointerX, pointerY), 10.0);
	gl::drawSolidCircle(Vec2f(calibratedX, 480+calibratedY), 10.0);
		
	params::InterfaceGl::draw();
	
}


CINDER_APP_BASIC( kinectBasicApp, RendererGl )
