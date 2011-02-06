#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Surface.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "cinder/Rand.h"
#include "cinder/params/Params.h"

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
	
	Kinect				mKinect;
	gl::Texture			mColorTexture, mDepthTexture, mCvTexture;
	Surface				mDepthSurface;
	float				mThreshold, mBlobMin, mBlobMax;
	int					mBlur;
	params::InterfaceGl	mParams;
};

void kinectBasicApp::prepareSettings( Settings* settings )
{
	settings->setWindowSize( 1280, 480 );
	mThreshold = 70.0f;
	mBlur = 10.0;
	mBlobMin = 40.0;
	mBlobMax = 200.0;
	mParams = params::InterfaceGl("WakaWaka", Vec2i(10,10));
	mParams.addParam( "Threshold", &mThreshold, "min=0.0 max=255.0 step=1.0 keyIncr=2 keyDecr=1" );
	mParams.addParam( "Blur", &mBlur, "min=1.0 max=100.0 step=1.0 keyIncr=4 keyDecr=3" );
	mParams.addParam( "BlobMin", &mBlobMin, "min=1.0 max=500.0 step=5.0 keyIncr=6 keyDecr=5" );
	mParams.addParam( "BlobMax", &mBlobMax, "min=1.0 max=500.0 step=5.0 keyIncr=8 keyDecr=7" );
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
//		mDepthTexture = mKinect.getDepthImage();
		
		mDepthTexture = this->threshholdTexture(mKinect.getDepthImage());

	//	gl::Texture
		
/*		
		ImageSourceRef depthImage = mKinect.getDepthImage();
		if(depthImage){
		}else{
			console() << "no depthImage" << std::endl;
		}
		
		cv::Mat input(toOcv(Channel8u(depthImage))), blurred, thresholded, thresholded2, output;
		
		cv::blur(input, blurred, cv::Size(10,10));
		
		float threshF = 70.0f;
		
		// make two thresholded images one to display and one
		// to pass to find contours since its process alters the image
		cv::threshold( blurred, thresholded, threshF, 255, CV_THRESH_BINARY);
		cv::threshold( blurred, thresholded2, threshF, 255, CV_THRESH_BINARY);
		
		mDepthTexture = gl::Texture(fromOcv(thresholded)); //gl::Texture(fromOcv(output));
 */
		
	}
	
	if( mKinect.checkNewVideoFrame() ){
//		mColorTexture = mKinect.getVideoImage();
		mColorTexture = this->threshholdTexture(mKinect.getVideoImage());
	}
	
//	console() << "Accel: " << mKinect.getAccel() << std::endl;
}

void kinectBasicApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
	gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
	if( mDepthTexture )
		gl::draw( mDepthTexture );
	if( mColorTexture )
		gl::draw( mColorTexture, Vec2i( 640, 0 ) );
	
	params::InterfaceGl::draw();

//	if( mColorTexture )
//		gl::draw( mColorTexture);
}


CINDER_APP_BASIC( kinectBasicApp, RendererGl )
