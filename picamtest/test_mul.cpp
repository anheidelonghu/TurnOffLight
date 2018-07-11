#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <chrono>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <raspicam/raspicam_cv.h>

#include <pthread.h>

#include <wiringPi.h>
#include <wiringSerial.h>

#include <thread>
#include <mutex>

using namespace std;
using namespace cv;
using ns = chrono::nanoseconds;
using get_time = chrono::steady_clock;





int hLow = 160;
int hHigh = 179;
int sLow = 100;
int sHigh = 255;
int vLow = 128;
int vHigh = 255;

const char* device = "/dev/ttyUSB0";
int baudrate = 9600;

string head = "S ";
string sufix = "A";

mutex mtx;
Mat cut,hsv,hsvOut;

void get_cut(raspicam::RaspiCam_Cv& Camera, Mat& img)
{
	while (true)
	{
		//cout << "begin thread 1" << endl;
		Camera.grab();
        	Camera.retrieve (img);
		//imshow("img",img);
		//cout << "image grabed" << endl;
		if (img.empty())
		{
			cerr << "blank frame grabbed" << endl;
		}
		if (mtx.try_lock())
		{
			cut = img(Rect(0, 40, 640, 440));
			//cut = img(Rect(160, 120, 320, 240));
			mtx.unlock();
		}
		//cout << "thread 1" << endl;
	}
}

void process(int fd, bool ifshow)
{
	while (true)
	{
		auto t0 = get_time::now();
		if (mtx.try_lock())
		{
			if (cut.cols > 0)
			{
				cvtColor(cut, hsv, CV_BGR2HSV);
				mtx.unlock();
			}
			else
			{
				mtx.unlock();
				continue;
			}
			

		}
		if (hsv.cols == 0) continue;
		inRange(hsv, Scalar(hLow, sLow, vLow), Scalar(hHigh, sHigh, vHigh), hsvOut);
		int cols = hsvOut.cols;
		int rows = hsvOut.rows;
		int px = 0;
		int py = 0;
		int cnt = 0;
		for (int col = 0; col < cols; col++)
		{
			for (int row = 0; row < rows; row++)
			{
				if (hsvOut.at<uchar>(cv::Point(col, row)) == 255)
				{
					px += col;
					py += row;
					cnt++;
				}
			}
		}
		cout << cnt << endl;
		if (cnt > 10)
		{
			px /= cnt;
			py /= cnt;

		}
		else
		{
			px = 0;
			py = 0;
			cnt = 0;
		}
		string message = head + to_string(px) + ' ' + to_string(py) + ' ' + to_string(cnt) + sufix;
		const char* str1 = message.c_str();
		char* str = const_cast<char*>(str1);
		serialPuts(fd,str);
		if (ifshow)
		{
			cvtColor(hsvOut, hsvOut, CV_GRAY2BGR);
			if (cnt > 10)
				circle(hsvOut, Point(px, py), 5, Scalar(0, 0, 255), 5);
			imshow("preview", hsvOut);
			imshow("cut", cut);
			char c = waitKey(1);
			if (c == 27)
			{
				break;
			}
		}
		auto t6 = get_time::now();
		auto diff = t6 - t0;
		cout << "add imshow operation: " << chrono::duration_cast<ns>(diff).count() << endl;
	}
}

int main(int argc, char** argv)
{
	int fd;
	if((fd = serialOpen(device, baudrate)) < 0)
    	{
        	cerr << "Unable to open serial device" << endl;
	        return -1;
    	}
    
    	if (wiringPiSetup () == -1)
    	{
	        cerr << "Unable to start wiringPi" << endl;
	        return 1 ;
	}

	bool ifshow = (argc > 1);
	raspicam::RaspiCam_Cv Camera;
	Camera.set( CV_CAP_PROP_FORMAT, CV_8UC3 );
        Camera.set(CV_CAP_PROP_FRAME_WIDTH, 640);
        Camera.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
        Camera.set(CV_CAP_PROP_EXPOSURE, 1);
	Camera.set(CV_CAP_PROP_GAIN, 5);
	//Camera.set(CV_CAP_PROP_WHITE_BALANCE_RED_V, 1);
	//Camera.set(CV_CAP_PROP_WHITE_BALANCE_BLUE_U, 1);
        Camera.set(CV_CAP_PROP_FPS, 120);
	//cap.set(CV_CAP_PROP_EXPOSURE, -10);
	if (!Camera.open())
	{
		cerr << "can not open camera!" << endl;
		return -1;
	}

	Mat src;
	Camera.grab();
        Camera.retrieve (src);
	cout << "rows: " << src.rows << " , cols: " << src.cols << endl;

	thread t0 = thread(get_cut, std::ref(Camera), std::ref(src));
	thread t1 = thread(process, fd, ifshow);
	t0.join();
	t1.join();

	return 0;




}