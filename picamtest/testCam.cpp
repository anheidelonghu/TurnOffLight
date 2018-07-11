#include <iostream>
#include <raspicam/raspicam_cv.h>
#include <opencv2/opencv.hpp>
#include <chrono>
using namespace std; 
using namespace cv;

using ns = chrono::nanoseconds;
using get_time = chrono::steady_clock;
 
int main ( int argc,char **argv ) {
   
    time_t timer_begin,timer_end;
    raspicam::RaspiCam_Cv Camera;
    cv::Mat image;
    int nCount=100;
    //set camera params
    Camera.set( CV_CAP_PROP_FORMAT, CV_8UC3 );
    Camera.set(CV_CAP_PROP_FRAME_WIDTH, 320);
    Camera.set(CV_CAP_PROP_FRAME_HEIGHT, 240);
    Camera.set(CV_CAP_PROP_EXPOSURE, 1);
    Camera.set(CV_CAP_PROP_FPS, 120);
    //Open camera
    cout<<"Opening Camera..."<<endl;
    if (!Camera.open()) {cerr<<"Error opening the camera"<<endl;return -1;}
    //Start capture
    while (true) {
	auto t0 = get_time::now();
        Camera.grab();
        Camera.retrieve ( image);
	
	cout << image.cols << "   " << image.rows << endl;
        imshow("image",image);
	char c = waitKey(1);
	if(c==27) break;
	auto t6 = get_time::now();
	auto diff = t6 - t0;
	cout << "add imshow operation: " << chrono::duration_cast<ns>(diff).count() << endl;
    }
    cout<<"Stop camera..."<<endl;
    Camera.release();
    //show time statistics
}