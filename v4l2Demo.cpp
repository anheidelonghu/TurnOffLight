#include "myv4l2.hpp"

int main()
{
	unsigned char *yuv422frame = NULL;
	unsigned long yuvframeSize = 0;

	string videoDev = "/dev/v4l/by-path/platform-3f980000.usb-usb-0:1.4:1.0-video-index0";
	V4L2Capture *vcap = new V4L2Capture(const_cast<char*>(videoDev.c_str()), 320, 240, 15);
	vcap->openDevice();
	vcap->initDevice();
	vcap->startCapture();

	cvNamedWindow("Capture",CV_WINDOW_AUTOSIZE);
	IplImage* img;
	CvMat cvmat;
	double t;
	while(1){
		t = (double)cvGetTickCount();
		vcap->getFrame((void **) &yuv422frame, (size_t *)&yuvframeSize);
		cvmat = cvMat(IMAGEHEIGHT,IMAGEWIDTH,CV_8UC3,(void*)yuv422frame);		//CV_8UC3

		//解码
		img = cvDecodeImage(&cvmat,1);
		if(!img){
			printf("DecodeImage error!\n");
		}

		//cvShowImage("Capture",img);
		cvReleaseImage(&img);

		vcap->backFrame();
		/*
		if((cvWaitKey(1)&255) == 27){
			exit(0);
		}
		*/
		t = (double)cvGetTickCount() - t;
		//printf("Used time is %g ms\n",( t / (cvGetTickFrequency()*1000)));
		cout << ( t / (cvGetTickFrequency()*1000)) <<" ms" <<endl;
	}		
	vcap->stopCapture();
	vcap->freeBuffers();
	vcap->closeDevice();
}