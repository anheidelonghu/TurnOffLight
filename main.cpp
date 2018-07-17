/*
@author hxx
@email	hxx_zju@zju.edu.cn
@date	2018/07/17
@brief	rgb图像用于检测红灯，ir图像用于检测障碍物，
*/
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

#include <pthread.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "myv4l2.hpp"

using namespace std;
using namespace cv;
using ns = chrono::nanoseconds;
using Ms = std::chrono::milliseconds;
using get_time = chrono::steady_clock;

//设置rgb图像和ir图像的大小，以及对应相机的曝光时间，rgbExp=1对应设置rgb图像曝光时间为0.1ms，曝光时间根据相机调整
#define rgbW 640
#define rgbH 480
#define rgbExp 15
#define irW 320
#define irH 240
#define irExp 20
//设置4个线程的时间间隔，由于树莓派性能限制，实际值>=设定值
#define rgbGetInterval 15	//rgb图像输入时间间隔15ms
#define irGetInterval 15	
#define rgbProInterval 25	//rgb图像处理时间间隔为25ms
#define irProInterval 25
//红灯hsv分割阈值
int hLow = 160;
int hHigh = 179;
int sLow = 100;
int sHigh = 255;
int vLow = 128;
int vHigh = 255;
//串口设备信息
const char* device = "/dev/ttyUSB0";
int baudrate = 9600;
//红灯检测和障碍物检测的标志信息
string head = "S ";
string sufix = "A";
string head_ir = "T";
string sufix_ir = "B";
//线程锁
timed_mutex mtx, mtx_ir, mtx_usb;
condition_variable conval_rgb, conval_ir;
//图像标志位，ture表示从摄像头获取了一帧图像，处理后被置为false
bool flag_rgb = false, flag_ir = false;
//bool if_rgb_ok() {return flag_rgb;}
//bool if_ir_ok() {return flag_ir;}
Mat cut,hsv,hsvOut;
Mat cut_ir;
//时间相关
auto t0 = get_time::now();
auto t0_ir = get_time::now();
auto t0_get = get_time::now();
auto t0_ir_get = get_time::now();
//v4l2 视频获取
unsigned char *RGBframe = NULL;
unsigned char *IRframe = NULL;
unsigned long RGBframeSize = 0;
unsigned long IRframeSize = 0;
Mat matRGB, matIR;
//获取rgb图像，大小：640*380，可根据需要修改
void get_cut(V4L2Capture& cap, Mat& img)
{
	unique_lock<timed_mutex> lock(mtx, defer_lock);
	while (true)
	{
		if(!flag_rgb)
		{
			if (lock.try_lock())
			{
				cap.getFrame((void **) &RGBframe, (size_t *)&RGBframeSize);
				matRGB = Mat(rgbW,rgbH,CV_8UC3,(void*)RGBframe);
				img = imdecode(matRGB,1);
				
				if (img.empty())
				{
					cerr << "blank frame grabbed" << endl;
					cap.backFrame();
	
					lock.unlock();
					continue;
				}
				cap.backFrame();
				cut = img(Rect(0, 100, 640, 380));
				//cut = img.clone();
				flag_rgb = true;
				lock.unlock();
				auto diff = get_time::now() - t0_get;
				//限制帧率
				if(chrono::duration_cast<Ms>(diff) < Ms(rgbGetInterval))
					this_thread::sleep_for(Ms(rgbGetInterval)-chrono::duration_cast<Ms>(diff));
				
				//diff = get_time::now() - t0_get;
				//cout << "rgb get and cut image: " << chrono::duration_cast<Ms>(diff).count() << endl;
				t0_get = get_time::now();
				
			}
		}
		
	}
}
//获取ir图像，大小：640*480
void get_cut_ir(V4L2Capture& cap, Mat& img)
{
	unique_lock<timed_mutex> lock(mtx_ir, defer_lock);
	while (true)
	{
		//cout << "ir while" << endl;
		if (lock.try_lock())
		{
			cap.getFrame((void **) &IRframe, (size_t *)&IRframeSize);
			matIR = Mat(irW,irH,CV_8UC3,(void*)IRframe);
			img = imdecode(matIR,1);//directly get gray image
			//cout << img.cols << endl;
			//cap >> img;
			if (img.empty())
			{
				cerr << "blank frame grabbed" << endl;
				cap.backFrame();

				lock.unlock();
				continue;
			}
			//cout << "check ir image: " << img.cols << "   " << img.rows << endl;
			cap.backFrame();
			//cut_ir = img(Rect(0, 0, 640, 480));
			cut_ir = img.clone();
			//resize(img,cut_ir,Size(320,240));
			flag_ir = true;
			lock.unlock();
			auto diff = get_time::now() - t0_ir_get;
			if(chrono::duration_cast<Ms>(diff) < Ms(irGetInterval))
				this_thread::sleep_for(Ms(irGetInterval)-chrono::duration_cast<Ms>(diff));
			//cout << "ir get and cut image: " << chrono::duration_cast<Ms>(diff).count() << endl;
			//diff = get_time::now() - t0_ir_get;
			//cout << "ir get and cut image: " << chrono::duration_cast<Ms>(diff).count() << endl;
			t0_ir_get = get_time::now();
		}
		
	}
}
//rgb图像处理，得到红灯坐标和大小，通过串口输出，显示图像还有bug
void process(int fd, bool ifshow)
{
	unique_lock<timed_mutex> lock(mtx, defer_lock);
	unique_lock<timed_mutex> lock_usb(mtx_usb, defer_lock);
	vector<vector<cv::Point>> contours;
	
	while (true)
	{
		if(flag_rgb)
		{	
			if (mtx.try_lock_for(Ms(8)))
			{
				flag_rgb = false;
				if (!cut.empty() && cut.data)
				{					
					cvtColor(cut, hsv, COLOR_BGR2HSV);
					mtx.unlock();
				}
				else
				{
					mtx.unlock();
					continue;
				}
				
	
			}
		}
		if (hsv.cols == 0) continue;
		//flag_rgb = false;
		inRange(hsv, Scalar(hLow, sLow, vLow), Scalar(hHigh, sHigh, vHigh), hsvOut);
		int cols = hsvOut.cols;
		int rows = hsvOut.rows;
		int px = 0;
		int py = 0;
		int cnt = 0;
		//auto t0_temp = get_time::now();
		findContours(hsvOut, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);	//获取红灯轮廓
		Moments mu;
		//Point2f mc;
		//最大的轮廓代表红灯，计算红灯质心和像素数量
		double maxArea = 0;
		for (size_t i = 0; i < contours.size(); i++)
		{
			double tmpArea = contourArea(contours[i]);
			if(tmpArea > maxArea)
			{
				maxArea = tmpArea;
				mu = moments(contours[i], false);
				//mc = Point2f(mu.m10 / mu.m00, mu.m01 / mu.m00);
				px = mu.m10 / mu.m00;
				py = mu.m01 / mu.m00;
			}
		}
		//auto t1_temp = get_time::now();
		//auto diff_temp = t1_temp-t0_temp;
		//cout << "my operation: " << chrono::duration_cast<Ms>(diff_temp).count() << endl;
		cnt = maxArea;
		//cout << cnt << endl;
		//最小像素数量=8
		if(cnt < 8)
		{
			px = 0;
			py = 0;
			cnt = 0;
		}
		else
		{
			//px += 50;
			py += 100;
		}
		//串口数据发送
		string message = head + to_string(px) + ' ' + to_string(py) + ' ' + to_string(cnt) + sufix;
		const char* str1 = message.c_str();
		char* str = const_cast<char*>(str1);
		if(lock_usb.try_lock_for(std::chrono::milliseconds(2)))
		{
			serialPuts(fd,str);
			lock_usb.unlock();
		}
		//显示rgb处理结果，有bug
		if (ifshow)
		{
			cvtColor(hsvOut, hsvOut, CV_GRAY2BGR);
			if (cnt > 8)
				circle(hsvOut, Point(px, py-100), 5, Scalar(0, 0, 255), 5);
			imshow("preview", hsvOut);
			//imshow("cut", cut);
			char c = waitKey(1);
			if (c == 27)
			{
				break;
			}
		}
		cout << message << endl;
		//auto t6 = get_time::now();
		auto diff = get_time::now() - t0;
		//限制帧率
		if(chrono::duration_cast<Ms>(diff) < Ms(rgbProInterval))
				this_thread::sleep_for(Ms(rgbProInterval)-chrono::duration_cast<Ms>(diff));
		
		diff = get_time::now() - t0;
		cout << "rgb time per image: " << chrono::duration_cast<Ms>(diff).count() << endl;
		t0 = get_time::now();
	}
	//usleep(20000);
}
//处理ir图像，获得障碍物信息，发送障碍物的位置和宽度，理论上这个线程会有bug，实际确实有
//todo 修复bug
void process_ir(int fd, bool ifshow)
{
	unique_lock<timed_mutex> lock(mtx_ir, defer_lock);
	unique_lock<timed_mutex> lock_usb(mtx_usb, defer_lock);
	//名字取的不好，实际上是最小的被认为是障碍物的像素块大小。。。。。。减小此参数可以增加检测距离，提高灵敏度，太小会出玄学bug哦
	double maxArea = 40;
	int px = 0;
	int py = 0;
	int width = 0;
	Mat gray, erodeImg, bwImg;
	vector<vector<cv::Point>> contours;
	while(true)
	{
		//t0_ir = get_time::now();
		if (mtx_ir.try_lock_for(Ms(8)) && flag_ir)
		{
			flag_ir = false;
			cout << cut_ir.cols << endl;
			if (cut_ir.cols > 0)
			{
				cvtColor(cut_ir, gray, CV_BGR2GRAY);
				//gray = cut_ir.clone();
				//imshow("cut_ir",cut_ir);
				mtx_ir.unlock();
			}
			else
			{
				mtx_ir.unlock();
				continue;
			}
		}
		else
		{
			mtx_ir.unlock();
			continue;
		}
		
		if(gray.cols == 0) continue;
		//flag_ir = false;
		//二值化，阈值与曝光时间设置有关
		threshold(gray, bwImg, 50, 255, THRESH_BINARY);//threshold image
		//腐蚀
		erode(bwImg, erodeImg, getStructuringElement(MORPH_ELLIPSE, Size(3, 3)));//erode	
		//找轮廓
		cv::findContours(erodeImg, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
		//标有 ****************** 的都可以去掉，仅图像显示用
		Mat result;//******************
		cvtColor(gray,result,CV_GRAY2BGR);//******************
		string message = head_ir;
		//找到所有可能是障碍物的东西
		for (size_t i = 0; i < contours.size(); i++)
		{
			//cout << contourArea(contours[i]) << endl;
			if (contourArea(contours[i]) > maxArea)
			{
				cv::Rect r = cv::boundingRect(contours[i]);
				px = r.x*2 + r.width; py = r.y*2 + r.height; width = r.width*2;
				message += ' ' + to_string(px) + ' ' + to_string(py) + ' ' + to_string(width);
				
				cv::rectangle(result, r, cv::Scalar(0,0,255));//******************
			}
		}
		
		//imshow("bwImg",erodeImg);//******************
		if(ifshow)
		{
			imshow("result",result);//******************
			waitKey(1);//******************
		}
		message += sufix_ir;
		const char* str1 = message.c_str();
		char* str = const_cast<char*>(str1);
		if(lock_usb.try_lock_for(std::chrono::milliseconds(2)))
		{
			serialPuts(fd,str);
			lock_usb.unlock();
		}
		cout << message << endl;

		auto diff = get_time::now() - t0_ir;
		
		if(chrono::duration_cast<Ms>(diff) < Ms(irProInterval))
				this_thread::sleep_for(Ms(irProInterval)-chrono::duration_cast<Ms>(diff));
		diff = get_time::now() - t0_ir;
		cout << "ir time per image: " << chrono::duration_cast<Ms>(diff).count() << endl;
		t0_ir = get_time::now();
	}
}

int main(int argc, char** argv)
{
	//串口设置
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
	//show 传1个参数显示rgb结果，传两个参数显示ir处理结果
	bool ifshow, ifshow_ir;
	if(argc==2)
	{
		ifshow = 1;
		ifshow_ir = 0;
	}
	else if(argc == 3)
	{
		ifshow = 0;
		ifshow_ir = 1;
	}
	else
	{
		ifshow = 0;
		ifshow_ir = 0;
	}
	//device path，相当于设备的名字，查看方式见README.txt，很关键！！！！！！！！！！！！！！！
	string deviceRGB = "/dev/v4l/by-path/platform-3f980000.usb-usb-0:1.2:1.0-video-index0";
	string deviceIR = "/dev/v4l/by-path/platform-3f980000.usb-usb-0:1.4:1.0-video-index0";
	/*********************RGB capture******************/
	V4L2Capture capRGB(const_cast<char*>(deviceRGB.c_str()), rgbW, rgbH, rgbExp);//set lowest exposure
	capRGB.openDevice();
	capRGB.initDevice();
	capRGB.startCapture();
	/*********************RGB capture******************/
	V4L2Capture capIR(const_cast<char*>(deviceIR.c_str()), irW, irH, irExp);
	capIR.openDevice();
	capIR.initDevice();
	capIR.startCapture();

	//打印图像信息
	Mat src, src_ir;
	cout << "display image property" << endl;
	while (capRGB.getFrame((void **)&RGBframe, (size_t *)&RGBframeSize) == -1) capRGB.backFrame();
	matRGB = Mat(rgbW,rgbH,CV_8UC3,(void*)RGBframe);
	src = imdecode(matRGB,1);
	imwrite("src.png",src);
	cout << "RGB image property -- rows: " << src.rows << " , cols: " << src.cols << endl;
	capRGB.backFrame();
	cout << "**********************************" << endl;
	while (capIR.getFrame((void **)&IRframe, (size_t *)&IRframeSize) == -1) capIR.backFrame();
	matIR = Mat(irW,irH,CV_8UC3,(void*)RGBframe);
	src = imdecode(matRGB,1);//directly get gray image
	cout << "IR image property -- rows: " << src_ir.rows << " , cols: " << src_ir.cols << endl;
	capIR.backFrame();
	
	//分别是获取rgb、处理rgb、获取ir、处理ir线程，不需要可以注释一部分
	thread t0 = thread(get_cut, std::ref(capRGB), std::ref(src));
	thread t1 = thread(process, fd, ifshow);
	thread t0_ir = thread(get_cut_ir, std::ref(capIR), std::ref(src_ir));
	thread t1_ir = thread(process_ir, fd, ifshow_ir);

	t0.join();
	t1.join();
	t0_ir.join();
	t1_ir.join();
	//释放相机，其实没啥用，哈哈（—_—！）
	capRGB.stopCapture();
	capRGB.freeBuffers();
	capRGB.closeDevice();
	capIR.stopCapture();
	capIR.freeBuffers();
	capIR.closeDevice();

	return 0;

}