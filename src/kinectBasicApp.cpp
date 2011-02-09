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
	Surface colorThreshholdChannels(Surface rgbImage);
	//	void colorThreshholdChannels(Surface rgbImage);
	Channel thresholdChannel(Channel channel);
	Surface surfaceFromChannel(Surface rgbImage, int rgb);
	gl::Texture threshholdTextureFromChannelSurface(Surface rSurface);
	Surface threshholdSurfaceFromChannelSurface(Surface rSurface);
	
	Kinect				mKinect;
	gl::Texture			mTextureTopLeft, mTextureTopRight, mTextureBottomLeft, mTextureBottomRight;
	float				mThreshold, mBlobMin, mBlobMax;
	int					mBlur, mVthresh, mHthresh, mSthresh;
	int					maxColorThresh, minColorThresh;
	int					mColorMode, minSaturation, minVal;
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
	
	settings->setWindowSize( 1280, 800); //960 );
	mThreshold = 70.0f;
	mBlur = 10.0;
	mBlobMin = 40.0;
	mBlobMax = 200.0;
	mParams = params::InterfaceGl("WakaWaka", Vec2i(200,100));
	//	mParams.addParam( "Threshold", &mThreshold, "min=0.0 max=255.0 step=1.0 keyIncr=2 keyDecr=1" );
	//	mParams.addParam( "Blur", &mBlur, "min=1.0 max=100.0 step=1.0 keyIncr=4 keyDecr=3" );
	//	mParams.addParam( "BlobMin", &mBlobMin, "min=1.0 max=500.0 step=5.0 keyIncr=6 keyDecr=5" );
	//	mParams.addParam( "BlobMax", &mBlobMax, "min=1.0 max=500.0 step=5.0 keyIncr=8 keyDecr=7" );
	//	mParams.addParam( "mColorMode", &mColorMode, "min=0.0 max=2.0 step=1.0 keyIncr=2 keyDecr=1");
	mParams.addParam( "minColorThresh", &minColorThresh, "min=0.0 max=180.0 step=5.0 keyIncr=2 keyDecr=1");
	mParams.addParam( "maxColorThresh", &maxColorThresh, "min=0.0 max=180.0 step=5.0 keyIncr=0 keyDecr=9");
	mParams.addParam( "minSaturation", &minSaturation, "min=0.0 max=255.0 step=5.0 keyIncr=w keyDecr=s");
	mParams.addParam( "minVal", &minVal, "min=0.0 max=255.0 step=5.0 keyIncr=f keyDecr=v");	
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
		//		mDepthTexture = mKinect.getDepthImage();
		
		//		mTextureBottomRight = this->threshholdTexture(mKinect.getDepthImage());
		
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
		Surface rgbImage(mKinect.getVideoImage());
		//		mColorTexture = gl::Texture(rgbImage); //mKinect.getVideoImage();
		
		//		mColorTexture = this->threshholdTexture(mKinect.getVideoImage());
		
		// TODO / NOTE: Do we need to convert between the Surface and the glTexture here?
		// mColorTexture = this->colorThreshholdChannels(mKinect.getVideoImage());
		
		
		//Surface newSurface(rgbImage.getWidth(), rgbImage.getHeight(), true);
		//Channel rChannel = this->thresholdChannel(rgbImage.getChannelRed());
		//Channel gChannel = this->thresholdChannel(rgbImage.getChannelGreen());
		//Channel bChannel = this->thresholdChannel(rgbImage.getChannelBlue());
		/*
		 mTexR = gl::Texture(this->thresholdChannel(rgbImage.getChannelRed()));
		 mTexG = gl::Texture(this->thresholdChannel(rgbImage.getChannelGreen()));
		 mTexB = gl::Texture(this->thresholdChannel(rgbImage.getChannelBlue()));
		 */
		
		//		mTexR = gl::Texture(rgbImage.getChannelRed());
		
		/*
		 Surface rSurface(rgbImage.getWidth(), rgbImage.getHeight(), true);//(rgbImage.getChannelRed());
		 Channel rChannel = rgbImage.getChannelRed();
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
		 
		 if(true){
		 //			mTexR = gl::Texture(rSurface);
		 cv::Mat input(toOcv(Channel(rSurface))), blurred, thresholded, thresholded2, output;
		 // NOTE: Stretches out image
		 //			cv::Mat input(toOcv(rgbImage.getChannelRed())), blurred, thresholded, thresholded2, output;
		 cv::blur(input, blurred, cv::Size(mBlur, mBlur));
		 cv::threshold( blurred, thresholded2, mThreshold, 255, CV_THRESH_BINARY);
		 cv::cvtColor(thresholded2, output, CV_GRAY2RGB );
		 mTexR = gl::Texture(fromOcv(output));
		 }else{
		 cv::Mat input(toOcv(rSurface)), blurred, thresholded, thresholded2, output;
		 cv::blur(input, blurred, cv::Size(mBlur, mBlur));
		 
		 //		cv::threshold( blurred, thresholded, mThreshold, 255, CV_THRESH_BINARY);
		 cv::threshold( blurred, thresholded2, mThreshold, 255, CV_THRESH_BINARY);
		 cv::cvtColor(thresholded2, output, CV_GRAY2RGB );
		 */ 
		/*
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
		 */
		//mTexR = gl::Texture(fromOcv(output));
		//}
		
		//		Surface redSurface = this->surfaceFromChannel(rgbImage, 0);
		/*
		 mTexR = this->threshholdTextureFromChannelSurface(this->surfaceFromChannel(rgbImage, 0));
		 mTexG = this->threshholdTextureFromChannelSurface(this->surfaceFromChannel(rgbImage, 1));
		 mTexB = this->threshholdTextureFromChannelSurface(this->surfaceFromChannel(rgbImage, 2));
		 */
		
		// Threshold Channel overlap
		/*		
		 // 1 Get surfaces for each channel
		 Surface rSurface = this->surfaceFromChannel(rgbImage, 0);
		 Surface gSurface = this->surfaceFromChannel(rgbImage, 1);
		 Surface bSurface = this->surfaceFromChannel(rgbImage, 2);
		 
		 // threshhold each surface
		 Surface rThresh = this->threshholdSurfaceFromChannelSurface(rSurface);
		 Surface gThresh = this->threshholdSurfaceFromChannelSurface(gSurface);
		 Surface bThresh = this->threshholdSurfaceFromChannelSurface(bSurface);
		 
		 // recombine them into 1 surface
		 Surface rgbThresh(640, 480, true);
		 //		Channel::Iter chIter = rChannel.getIter(); // using const because we're not modifying it
		 Surface::Iter surIter( rgbThresh.getIter() ); // not using const because we are modifying it
		 while( surIter.line() ) { // line by line
		 while( surIter.pixel() ) { // pixel by pixel
		 int x = surIter.x();
		 int y = surIter.y();
		 surIter.r() = *rThresh.getDataRed(Vec2i(x,y)); //.getChannelRed().getValue(Vec2i(x,y));//chValue;
		 surIter.g() = *gThresh.getDataGreen(Vec2i(x,y)); //.getChannelGreen().getValue(Vec2i(x,y));//gThresh.getDataRed(Vec2i(x,y));//chValue;
		 surIter.b() = *bThresh.getDataBlue(Vec2i(x,y)); //.getChannelBlue().getValue(Vec2i(x,y));//*bThresh.getDataRed(Vec2i(x,y));//chValue;
		 surIter.a() = 255; //255.0;
		 }
		 }
		 
		 mRGBThreshTexture = gl::Texture(rgbThresh);
		 */
		
		Surface redImage(640, 480, false), greenImage(640, 480, false), blueImage(640, 480, false);
		
		// Hand rolled color detection
		/*
		 Surface::ConstIter rgbIter = rgbImage.getIter();
		 while(rgbIter.line()){
		 while (rgbIter.pixel()) {
		 int x = rgbIter.x();
		 int y = rgbIter.y();
		 Vec2i pixPos(x,y);
		 ColorA8u rgbPixel = rgbImage.getPixel(pixPos);
		 float normalRgb = rgbPixel.r + rgbPixel.g + rgbPixel.b;
		 float normalR = rgbPixel.r / normalRgb;
		 float normalG = rgbPixel.g / normalRgb;
		 float normalB = rgbPixel.b / normalRgb;
		 
		 // Check for color rangeS <<<<
		 if(normalR > 0.5){
		 redImage.setPixel(pixPos, ColorA8u(255,0,0));
		 }else{
		 redImage.setPixel(pixPos, ColorA8u(0,0,0));
		 }
		 if(normalG > 0.5){
		 greenImage.setPixel(pixPos, ColorA8u(0,255,0));
		 }else{
		 greenImage.setPixel(pixPos, ColorA8u(0,0,0));
		 }
		 if(normalB > 0.5){
		 blueImage.setPixel(pixPos, ColorA8u(0,0,255));
		 }else{
		 blueImage.setPixel(pixPos, ColorA8u(0,0,0));
		 }
		 }
		 }
		 */
		
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
		//		mTextureTopRight = greenImage;
		//		mTextureBottomLeft = blueImage;
		mTextureBottomRight = rgbImage;
		
		/*		
		 // Color range isolation
		 
		 // 1) Convert Color to HSV
		 cv::Mat input(toOcv(rgbImage)), img_hsv_;
		 cv::cvtColor(input, img_hsv_, CV_RGB2HSV);
		 Surface hsvImage(fromOcv(img_hsv_)), hueImage(640, 480, false);
		 
		 
		 Surface::ConstIter hsvIter = hsvImage.getIter();
		 Surface::Iter hueIter = hueImage.getIter();
		 while(hsvIter.line()){
		 while (hsvIter.pixel()) {
		 int x = hsvIter.x();
		 int y = hsvIter.y();
		 ColorA8u hsvPix = hsvImage.getPixel(Vec2i(x,y));
		 int h = 0;
		 // r == saturation // r > 100 only shows saturated red and it's not affected by shadow
		 // g == color
		 // b == brightness?
		 if((hsvPix.g > 230 || hsvPix.g < 30) && hsvPix.r > 100) h = 255;
		 hueImage.setPixel(Vec2i(x,y), ColorA8u(h,h,h));
		 }
		 }
		 */ 
		/*
		 cv::Mat hueInput(toOcv(hueImage)), blurred, thresholded, output;
		 cv::blur(hueInput, blurred, cv::Size(10, 10));
		 cv::threshold(blurred, thresholded, mThreshold, 255, CV_THRESH_BINARY);
		 cv::cvtColor(thresholded, output, CV_GRAY2RGB );
		 mRGBThreshTexture = gl::Texture(fromOcv(output));
		 */
		
		//		mRGBThreshTexture = this->threshholdTexture(hueImage);
		
		//		Surface hsvImage(fromOcv(img_hsv_)), hueImage(640, 480, false);
		
		/*
		 Surface::ConstIter hsvIter = hsvImage.getIter();
		 Surface::Iter hueIter = hueImage.getIter();
		 while( hsvIter.line() && hueIter.line() ) { // line by line
		 while( hsvIter.pixel() && hueIter.pixel()) { // pixel by pixel
		 int x = hsvIter.x();
		 int y = hsvIter.y();
		 Vec2i loc(x,y);				
		 int h = *hsvImage.getDataRed(loc);// This really seems like brightness
		 int s = *hsvImage.getDataGreen(loc); // Brightness?
		 int v = *hsvImage.getDataBlue(loc);				
		 //				bool show = v < mVthresh && h < mHthresh && s < mSthresh;
		 bool show = h < 60 || h > 200;
		 hueIter.r() = show ? 255 : 0; //*rThresh.getDataRed(Vec2i(x,y)); //.getChannelRed().getValue(Vec2i(x,y));//chValue;
		 hueIter.g() = show ? 255 : 0; //*gThresh.getDataGreen(Vec2i(x,y)); //.getChannelGreen().getValue(Vec2i(x,y));//gThresh.getDataRed(Vec2i(x,y));//chValue;
		 hueIter.b() = show ? 255 : 0; //*bThresh.getDataBlue(Vec2i(x,y)); //.getChannelBlue().getValue(Vec2i(x,y));//*bThresh.getDataRed(Vec2i(x,y));//chValue;
		 //				hueIter.a() = 255; //255.0;
		 }
		 }
		 
		 mRGBThreshTexture = hueImage;
		 */
		
		//		cv::cvtColor(input, img_hsv_, CV_RGBA2RGB);
		//Channel8u hueChannel(rows, cols);		
		
		//console() << "cols: " << cols << " rows: " << rows << std::endl;
		
		//		cv::Mat img_out_(rows, cols, CV_RGB);
		
		// Zero Matrices
		/*		cv::Mat img_out_ = input.clone();
		 cv::Mat img_hue_ = cv::Mat::zeros(img_hsv_.cols, img_hsv_.rows, CV_8U);
		 cv::Mat img_sat_ = cv::Mat::zeros(img_hsv_.cols, img_hsv_.rows, CV_8U);
		 cv::Mat img_bin_ = cv::Mat::zeros(img_hsv_.cols, img_hsv_.rows, CV_8U);
		 int from_to[] = {0,0, 1,1};
		 cv::Mat img_split[] = { img_hue_, img_sat_ };
		 cv::mixChannels(&img_hsv_, 3, img_split, 2, from_to, 2);
		 */		
		
		//		console() << "img_hsv_.cols: " << img_hsv_.cols << " img_hsv_.rows: " << img_hsv_.rows << std::endl;
		//		console() << "hueImage.cols: " << hueImage.getWidth() << " hueImage.rows: " << hueImage.getHeight() << std::endl;
		// 2) Isolate pixels that have an H within range
		
		
		
		/*		// Display Input image
		 cv::imshow ("input", img_in_);
		 // Display Binary Image
		 cv::imshow ("binary image", img_bin_);
		 // Display segmented image
		 cv::imshow ("segmented output", img_out_);
		 */
		//		mRGBThreshTexture = gl::Texture(fromOcv(img_bin_));
		//		mRGBThreshTexture = hueImage;
		//		mRGBThreshTexture = gl::Texture(Channel(fromOcv(img_bin_)));
		
		//		mRGBThreshTexture = gl::Texture(fromOcv(img_bin_));
		//		mRGBThreshTexture = gl::Texture(hueChannel);
		//		console() << "mRGBThreshTexture: " << mRGBThreshTexture << std::endl;
		
		// 3) Create new channel that only shows those pixels
		// ... test 
		// 4) Blur
		// 5) Threshold
		// 6) Blob detect
		
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
	
	if( mTextureTopLeft )
		gl::draw( mTextureTopLeft, Vec2i(0, 0) );
	if( mTextureTopRight )
		gl::draw( mTextureTopRight, Vec2i( 640, 0 ) );
	if( mTextureBottomLeft )
		gl::draw( mTextureBottomLeft, Vec2i( 0, 480 ) );
	if( mTextureBottomRight )
		gl::draw( mTextureBottomRight, Vec2i( 640, 480 ) );
	
	params::InterfaceGl::draw();
	
}


CINDER_APP_BASIC( kinectBasicApp, RendererGl )
