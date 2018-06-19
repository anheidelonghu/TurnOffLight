#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <chrono>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <wiringPi.h>
#include <wiringSerial.h>

using namespace std;
using namespace cv;
using ns = chrono::nanoseconds;
using get_time = chrono::steady_clock;


//#define showImg


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


	Mat src, hsv, hsvOut;
	auto cap = VideoCapture(0);
	//cap.set(CV_CAP_PROP_EXPOSURE, -9);
	if (!cap.isOpened())
	{
		cerr << "can not open camera!" << endl;
		return -1;
	}

	cap >> src;
	cout << "rows: " << src.rows << " , cols: " << src.cols << endl;

	//namedWindow("preview");
	while (true)
	{	
		auto t0 = get_time::now();
		cap >> src;
		if (src.empty())
		{
			cerr << "blank frame grabbed" << endl;
		}
		
		auto t1 = get_time::now();
		auto diff = t1-t0;
		cout << "get image: " << chrono::duration_cast<ns>(diff).count() << endl;

		cvtColor(src, hsv, CV_BGR2HSV);
		inRange(hsv, Scalar(hLow, sLow, vLow), Scalar(hHigh, sHigh, vHigh), hsvOut);
		auto t2 = get_time::now();
		diff = t2-t1;
		cout << "hsv and inrange: " << chrono::duration_cast<ns>(diff).count() << endl;
		//形态学可以不做，节省运算资源，尤其是第二步。
		//除去小物体
		//erode(hsvOut, hsvOut, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		//dilate(hsvOut, hsvOut, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		//除去小孔
		//dilate(hsvOut, hsvOut, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		//erode(hsvOut, hsvOut, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		auto t3 = get_time::now();
		diff = t3-t2;
		cout << "erode and dilated: " << chrono::duration_cast<ns>(diff).count() << endl;

		//中心点(px,py),像素点数量cnt
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
		auto t4 = get_time::now();
		diff = t4-t3;
		cout << "pixel opration: " << chrono::duration_cast<ns>(diff).count() << endl;
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
		string message = head + to_string(px)+' '+to_string(py)+' '+to_string(cnt)+sufix;
		const char* str1 = message.c_str();
		char* str = const_cast<char*>(str1);
		serialPuts(fd,str);
		auto t5 = get_time::now();
		diff = t5-t4;
		cout << "serial opration: " << chrono::duration_cast<ns>(diff).count() << endl;
		diff = t5-t0;
		cout << "basic operation: " << chrono::duration_cast<ns>(diff).count() << endl;
		
		if(argc>1)
		{
			cvtColor(hsvOut, hsvOut, CV_GRAY2BGR);
			if(cnt>10)
				circle(hsvOut, Point(px, py), 5, Scalar(0, 0, 255), 5);
			imshow("preview", hsvOut);
			imshow("src",src);
			char c = waitKey(1);
			if (c == 27)
			{
				break;
			}
		}
		auto t6 = get_time::now();
		diff = t6-t0;
		cout << "add imshow operation: " << chrono::duration_cast<ns>(diff).count() << endl;


	}
	
	serialClose(fd);
	return 0;
}