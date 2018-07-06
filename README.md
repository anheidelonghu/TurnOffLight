功能和定义：
    1. 此版本代码基于树莓派3b+，使用usb摄像头(测试用摄像头视场角为90和120)采集图像，usb转ttl作为通讯端口，串口波特率为9600.
    2. 目前仅实现了红灯检测，实测在场地范围内的红灯都能识别。
    3. 输出格式为   Spx py pointNumberA,其中px为红灯在图像x方向上的中心坐标，py为红灯在图像y方向上的中心坐标，pointNumber为红灯在图像中所占有的像素数量。例如   S100 200 1001A   ，表示红灯在图像x方向上的中心像素坐标为100，在y方向上为200，所占像素数为1001.S和A为起止符。
    
预安装：
    1. 安装OpenCV：用于图像处理。使用OpenCV3，推荐编译时开启对Arm的优化选项，代码运行速度提升40%-50%。具体方法请参考 https://www.pyimagesearch.com/2017/10/09/optimizing-opencv-on-the-raspberry-pi/
    2. 安装wiringPi：用于串口通信。安装过程见 http://wiringpi.com/download-and-install/
    
代码使用：
    1. 克隆代码 $ git clone https://github.com/anheidelonghu/TurnOffLight.git
    2. 进入代码目录 $ cd TurnOffLight/
    3. 删除build目录 $ rm -rf build/
    4. 新建build目录 $ mkdir build/
    5. 进入build目录 $ cd build/
    6. 编译 $ cmake .. && make
    7. 运行代码 $ ./main    (运行带preview的代码 $ ./main 1   ，仅用于测试效果，正常使用时不推荐使用，会拖慢运行速度)
    
效率：
    由于测试时使用的是大视场相机，故在图像处理时对图像进行了裁剪，小视场相机不需要裁剪。
    320*240 单线程：33ms
    320*240 多线程：14-22ms      cpu使用率：50%
    320*480 多线程：27-30ms      cpu使用率：56%

注意事项：
    可能需要根据自身硬件修改代码，代码目前基本无注释，后续会增加注释和修改程序功能。
    fps和cpu温度有关，需要增加散热。
    
email：
    hxx_zju@zju.edu.cn