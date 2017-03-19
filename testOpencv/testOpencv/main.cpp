//
//  main.cpp
//  testOpencv
//
//  Created by zhanglei on 12/31/15.
//  Copyright © 2015 zhanglei. All rights reserved.
//

#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

int main (int argc, const char * argv[])
{
    Mat img = imread("/Users/lee/Desktop/search.jpg"); //Change the image path here.
    if (img.data == 0) {
        cerr << "Image not found!" << endl;
        return -1;
    }
    cout<<"success"<<endl;

    namedWindow("image", CV_WINDOW_AUTOSIZE);
    imshow("image", img);
    waitKey();
}
