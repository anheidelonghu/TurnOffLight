#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <iostream>

#include <wiringPi.h>
#include <wiringSerial.h>
#include <time.h>

using namespace std;

int main()
{
    const char *device = "/dev/ttyUSB0";
    int baudrate = 9600;
    int fd;
    string head = "S ";
    string sufix = "A";

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

    int px = 320;
    int py = 240;
    int pointNum = 66729;
    string message = head + to_string(px)+' '+to_string(py)+' '+to_string(pointNum)+sufix;
    //string message = to_string(px);
    const char* str1 = message.c_str();
    char* str = const_cast<char*>(str1);
    while(true)
    {
        serialPuts(fd,str);
	//usleep(100);
	cout << str <<endl;	
    }
    serialClose(fd);
}




