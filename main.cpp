#include "opencv2/opencv.hpp"
#include <iostream>

using namespace std;
using namespace cv;

int main()
{
	Mat src;
	auto cap = VideoCapture(0);
	//cap.set(CV_CAP_PROP_EXPOSURE, -6);
	if (!cap.isOpened())
	{
		cerr << "can not open camera!" << endl;
		return -1;
	}

	cap >> src;
	cout << "rows: " << src.rows << " , cols: " << src.cols << endl;

	namedWindow("preview");
	while (true)
	{
		cap >> src;
		if (src.empty())
		{
			cerr << "blank frame grabbed" << endl;
		}

		imshow("preview", src);
		char c = waitKey(10);
		if (c == 27)
		{
			break;
		}
	}

	return 0;
}
