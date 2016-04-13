#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
/*
先声明一个 mat 型的 变量
imread 读入 需要的图片
声明一个显示的窗口
用imshow命令显示这个图像

image.data用来检查是否读入了数据
*/


#include <iostream>
#include <stdio.h>
using namespace cv;
using namespace std;

Mat image;
int main(int argc, char**argv)
{
    image =imread("/Users/zhangyi/Desktop/1.jpg");
    namedWindow("image",1);
// 如果不确定数据是否读入，可以用这来检查
if (!image.data) 
{
	cout << "no image has been created... " << endl;
}
//
    while (1) {
           imshow("image", image);
		   //必须在while中加一个,否则不出图像 waitkey（）括号里面的数可以随便填//
		   cv::waitKey(1); 
		   //在mac下并没有出现此类问题
    }
 
}

//  highgui module provided by OpenCV huigui这个库是用来显示图片的。例如imread，imshow