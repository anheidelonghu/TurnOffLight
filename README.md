功能和定义：
    1. 此版本代码基于树莓派3b+，使用usb摄像头(测试用摄像头视场角为90和120)采集图像，usb转ttl作为通讯端口，串口波特率为9600.
    2. 实现了红灯检测，实测在场地范围内的红灯都能识别。
    3. 红灯检测串口输出格式为   Spx py pointNumberA,其中px为红灯在图像x方向上的中心坐标，py为红灯在图像y方向上的中心坐标，pointNumber为红灯在图像中所占有的像素数量。例如   S100 200 1001A   ，表示红灯在图像x方向上的中心像素坐标为100，在y方向上为200，所占像素数为1001.S和A为起止符。
    4. 实现了障碍物检测，目前检测范围设置为0.5m左右，可根据需求在main.cpp中自行修改
	5. 障碍物检测串口输出为 T px py widthB，没有障碍物输出TB，可能会输出多个障碍物T px1 py1 width1 px2 py2 width2B
	6. 输出都print出来了，可以自己看看。

预安装：
    1. 安装OpenCV：用于图像处理。使用OpenCV3，推荐编译时开启对Arm的优化选项，代码运行速度提升40%-50%。具体方法请参考 https://www.pyimagesearch.com/2017/10/09/optimizing-opencv-on-the-raspberry-pi/
    2. 安装wiringPi：用于串口通信。安装过程见 http://wiringpi.com/download-and-install/
    
硬件配置：
	1. 使用了两个摄像头：rgb usb摄像头和红外窄带850nm usb摄像头(视场角90°)
	2. 使用了850nm 90°红外一字激光器，20mW，链接都发在群里了
	3. 由于使用了两个usb摄像头，需要在代码中指定设备名：
		<1> $ cd /dev/v4l/by-path
		<2> $ ls    查看usb摄像头设备名，usb摄像头的设备名仅和插的usb口位置相关！
		<3> 在main.cpp中修改deviceRGB deviceIR，对应两个摄像头
	4. 已测试，树莓派专用摄像头PiCamera可以使用此版本代码跑通，需要进行一下操作：
		<1> 树莓派摄像头红外、可见光均可见，可以到我这边领取850nm窄带滤光片（仅剩1枚），850nm红外一字激光器需要自行购买。
		<2> 插入PiCamera后，运行命令 $ sudo modprobe bcm2835-v4l2  ，PiCamera会被作为USB video设备
		<3> $ ls /dev | grep video ，在不插USB摄像头的情况下运行此命令，会看到以下结果：video0，表示PiCamera路径为/dev/video0
		<4> 修改PiCamera在main.cpp中的路径名称： deviceIR

代码使用：
    1. 克隆代码 $ git clone https://github.com/anheidelonghu/TurnOffLight.git
    2. 进入代码目录 $ cd TurnOffLight/
    3. 删除build目录 $ rm -rf build/
    4. 新建build目录 $ mkdir build/
    5. 进入build目录 $ cd build/
    6. 编译 $ cmake .. && make
    7. 运行代码 $ ./main    (rgb结果图显示 $ ./main 1  ，ir结果图显示：$ ./main 1 1   ,仅用于测试效果，正常使用时不推荐使用，会拖慢运行速度)
    
效率：
    考虑树莓派性能，把摄像头获取图像速度改为15ms，图像处理速度改为25m。因此理论fps=40，实际fps<40
	cpu使用率：60-95%

注意事项：
    此版本代码还有些许bug，遇到bug请截图发给我，如果能自行解决更好，解决了可以commit过来。
	树莓派对电源的电流和纹波要求较高，电源很关键！
    fps和cpu温度有关，需要增加散热。
    

email：
    hxx_zju@zju.edu.cn