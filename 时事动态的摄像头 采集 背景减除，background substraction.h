//#include "opencv2/highgui/highgui.hpp"
//#include <iostream>
//#include<opencv\cv.h>
//#include<opencv\highgui.h>
//#include<opencv2\objdetect\objdetect.hpp>
//#include<opencv2\highgui\highgui.hpp>
//#include<opencv2\imgproc\imgproc.hpp>
//#include<stdio.h>
//#include<math.h>
//#include<vector>
//
//using namespace std;
//using namespace cv;
//Mat frame;
//int main(int argc, char*argv[])
//{
//	VideoCapture cap(0);
//	if (!cap.isOpened())
//	{
//		cout << "Cannot open  cam" << endl;
//		return -1;
//	}
//	/*namedWindow("test", CV_WINDOW_AUTOSIZE);*/
//
//	while (1)
//	{
//		cap >> frame;
//		imshow("test", frame);
//		char c = waitKey(1);
//		if (c == 27)
//			break;
//		//cout << frame.size() << endl;
//	}
//	return 0;
//}


#include "opencv2/core/core.hpp"
#include "opencv2/video/background_segm.hpp"
#include<opencv2\objdetect\objdetect.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/opencv.hpp>
#include <Windows.h>
#include<iostream>
#include<math.h>
#include<vector>
//#include"codebookstructure.h"
using namespace std;
using namespace cv;


#define CHANNELS 3
typedef struct ce {
	uchar learnHigh[CHANNELS]; //High side threshold for learning
	uchar learnLow[CHANNELS]; //Low side threshold for learning
	uchar max[CHANNELS]; //High side of box boundary
	uchar min[CHANNELS]; //Low side of box boundary
	int t_last_update; //Allow us to kill stale entries
	int stale; //max negative run (longest period of inactivity)
} code_element;

//码书结构
typedef struct code_book {
	code_element **cb; //指向码字的指针
	int numEntries; //码书包含的码字数量
	int t; //count every access
} codeBook;

codeBook* TcodeBook;//包括所有像素的码书集合



int update_codebook(uchar *p, codeBook &c, unsigned *cbBounds, int numChannels)
{
	//unsigned int high[3],low[3];
	int high[3], low[3];
	int n;

	// 此处已删除。。测试过不太需要

	//if (c.numEntries == 0) 
	//	c.t = 0;
	//c.t = c.t + 1;//自己添加

	// 此处以上是删除部分，因为不需要。


	for (n = 0; n<numChannels; n++)
	{
		high[n] = *(p + n) + *(cbBounds + n);
		if (high[n] > 255)
		{
			high[n] = 255;
		}
		low[n] = *(p + n) - *(cbBounds + n);
		if (low[n] < 0)
			low[n] = 0;
	}
	int matchChannel;
	// SEE IF THIS FITS AN EXISTING CODEWORD
	//
	int i;
	//for(int i=0; i<c.numEntries; i++)
	for (i = 0; i<c.numEntries; i++)
	{
		matchChannel = 0;
		for (n = 0; n<numChannels; n++)
		{
			if ((c.cb[i]->learnLow[n] <= *(p + n)) &&
				//Found an entry for this channel
				(*(p + n) <= c.cb[i]->learnHigh[n]))
			{
				matchChannel++;
			}
		}
		if (matchChannel == numChannels) //If an entry was found
		{
			c.cb[i]->t_last_update = c.t;
			//adjust this codeword for the first channel
			for (n = 0; n<numChannels; n++){
				if (c.cb[i]->max[n] < *(p + n))
				{
					c.cb[i]->max[n] = *(p + n);
				}
				else if (c.cb[i]->min[n] > *(p + n))
				{
					c.cb[i]->min[n] = *(p + n);
				}
			}
			break;
		}
	}
	// OVERHEAD TO TRACK POTENTIAL STALE ENTRIES
	//
	for (int s = 0; s<c.numEntries; s++)
	{
		// Track which codebook entries are going stale:
		//
		int negRun = c.t - c.cb[s]->t_last_update;
		if (c.cb[s]->stale < negRun) c.cb[s]->stale = negRun;
	}
	// ENTER A NEW CODEWORD IF NEEDED
	//
	if (i == c.numEntries) //if no existing codeword found, make one
	{
		code_element **foo = new code_element*[c.numEntries + 1];
		for (int ii = 0; ii<c.numEntries; ii++)
		{
			foo[ii] = c.cb[ii];
		}
		foo[c.numEntries] = new code_element;
		if (c.numEntries) delete[] c.cb;
		c.cb = foo;
		for (n = 0; n<numChannels; n++)
		{
			c.cb[c.numEntries]->learnHigh[n] = high[n];
			c.cb[c.numEntries]->learnLow[n] = low[n];
			c.cb[c.numEntries]->max[n] = *(p + n);
			c.cb[c.numEntries]->min[n] = *(p + n);
		}
		c.cb[c.numEntries]->t_last_update = c.t;
		c.cb[c.numEntries]->stale = 0;
		c.numEntries += 1;
	}
	// SLOWLY ADJUST LEARNING BOUNDS
	//
	for (n = 0; n<numChannels; n++)
	{
		if (c.cb[i]->learnHigh[n] < high[n])
			c.cb[i]->learnHigh[n] += 1;
		if (c.cb[i]->learnLow[n] > low[n])
			c.cb[i]->learnLow[n] -= 1;
	}
	return(i);
}

///////////////////////////////////////////////////////////////////
//int clear_stale_entries(codeBook &c)
// During learning, after you’ve learned for some period of time,
// periodically call this to clear out stale codebook entries
//
// c Codebook to clean up
//
// Return
// number of entries cleared
//
int clear_stale_entries(codeBook &c){
	int staleThresh = c.t >> 1;
	int *keep = new int[c.numEntries];
	int keepCnt = 0;
	// SEE WHICH CODEBOOK ENTRIES ARE TOO STALE
	//
	for (int i = 0; i<c.numEntries; i++){
		if (c.cb[i]->stale > staleThresh)
			keep[i] = 0; //Mark for destruction
		else
		{
			keep[i] = 1; //Mark to keep
			keepCnt += 1;
		}
	}
	// KEEP ONLY THE GOOD
	//
	c.t = 0; //Full reset on stale tracking
	code_element **foo = new code_element*[keepCnt];
	int k = 0;
	for (int ii = 0; ii<c.numEntries; ii++){
		if (keep[ii])
		{
			foo[k] = c.cb[ii];
			//We have to refresh these entries for next clearStale
			foo[k]->t_last_update = 0;
			k++;
		}
	}
	// CLEAN UP
	//
	delete[] keep;
	delete[] c.cb;
	c.cb = foo;
	int numCleared = c.numEntries - keepCnt;
	c.numEntries = keepCnt;
	return(numCleared);
}

////////////////////////////////////////////////////////////
// uchar background_diff( uchar *p, codeBook &c,
// int minMod, int maxMod)
// Given a pixel and a codebook, determine if the pixel is
// covered by the codebook
//
// p Pixel pointer (YUV interleaved)
// c Codebook reference
// numChannels Number of channels we are testing
// maxMod Add this (possibly negative) number onto

// max level when determining if new pixel is foreground
// minMod Subract this (possibly negative) number from
// min level when determining if new pixel is foreground
//
// NOTES:
// minMod and maxMod must have length numChannels,
// e.g. 3 channels => minMod[3], maxMod[3]. There is one min and
// one max threshold per channel.
//
// Return
// 0 => background, 255 => foreground
//
uchar background_diff(
	uchar* p,
	codeBook& c,
	int numChannels,
	int* minMod,
	int* maxMod
	)
{
	int matchChannel;
	// SEE IF THIS FITS AN EXISTING CODEWORD
	//
	int i;
	for (i = 0; i<c.numEntries; i++) {
		matchChannel = 0;
		for (int n = 0; n<numChannels; n++) {
			if ((c.cb[i]->min[n] - minMod[n] <= *(p + n)) &&
				(*(p + n) <= c.cb[i]->max[n] + maxMod[n])) {
				matchChannel++; //Found an entry for this channel
			}
			else {
				break;
			}
		}
		if (matchChannel == numChannels) {
			break; //Found an entry that matched all channels
		}
	}
	if (i >= c.numEntries) return(255);
	return(0);
}

IplImage* pFrame = NULL;
IplImage* pFrameHSV = NULL;
IplImage* pFrImg = NULL;
CvCapture* pCapture = NULL;
int nFrmNum = 0;
//IplImage* pFrImg = NULL;
//IplImage* pBkImg = NULL;

unsigned cbBounds[3] = { 10, 10, 10 };

int height, width;
int nchannels;
int minMod[3] = { 35, 8, 8 }, maxMod[3] = { 25, 8, 8 };//和这两个值的选择有联系
Mat frame;
int main(int argc, char * argv[])
{
	//创建窗口
	//cvNamedWindow("video", 1);
	namedWindow("video", CV_WINDOW_AUTOSIZE);

	cvNamedWindow("HSV空间图像", 1);
	cvNamedWindow("foreground", 1);
	//使窗口有序排列
	cvMoveWindow("video", 30, 0);
	cvMoveWindow("HSV空间图像", 360, 0);
	cvMoveWindow("foreground", 690, 0);
	//打开摄像头；
	/*CvCapture*capture = cvCreateCameraCapture(0);*/
	VideoCapture cap(0);
	if (!cap.isOpened())
	{
		cout << "Cannot open  cam" << endl;
		return -1;
	}
	//while (1)
	//{
	//	cap >> frame;
	//	imshow("video", frame);
	//	char c = waitKey(1);
	//	if (c == 27)
	//		break;
	//	pFrame = &IplImage(frame);
	//}


	////打开视频文件，
	//if (!(pCapture = cvCaptureFromFile("intelligentroom_raw.avi")))/**/
	//{
	//	fprintf(stderr, "Can not open video file %s\n");
	//	return -2;
	//}

	int j;
	//逐帧读取视频
	while (1)//pFrame = cvQueryFrame(pCapture)
	{
		//
		//pFrame = cvQueryFrame(pCapture);
		cap >> frame;
		imshow("video", frame);
		char c = waitKey(1);
		pFrame = &IplImage(frame);

		if (!pFrame)
			break;

		//
		cvSmooth(pFrame, pFrame, CV_GAUSSIAN, 3, 3);//高斯滤波
		nFrmNum++;
		//printf("第几%d帧\n", nFrmNum);


		//cvShowImage("video", pFrame);

		if (nFrmNum == 1)
		{
			height = pFrame->height;
			width = pFrame->width;
			nchannels = pFrame->nChannels;
			pFrameHSV = cvCreateImage(cvSize(pFrame->width, pFrame->height), IPL_DEPTH_8U, 3);
			pFrImg = cvCreateImage(cvSize(pFrame->width, pFrame->height), IPL_DEPTH_8U, 1);
			//cvCvtColor(pFrame, pFrameHSV, CV_BGR2HSV);//色彩空间转化
			TcodeBook = new codeBook[width*height];

			for (j = 0; j < width*height; j++)
			{
				TcodeBook[j].numEntries = 0;
				TcodeBook[j].t = 0;
			}


		}

		if (nFrmNum<50)
		{
			cvCvtColor(pFrame, pFrameHSV, CV_BGR2YCrCb);//色彩空间转化
			//学习背景
			for (j = 0; j < width*height; j++)
			{
				update_codebook((uchar*)pFrameHSV->imageData + j*nchannels, TcodeBook[j], cbBounds, 3);
			}

		}
		else
		{
			cvCvtColor(pFrame, pFrameHSV, CV_BGR2YCrCb);//色彩空间转化
			if (nFrmNum == 50)
			{

				// for(j = 0; j < width*height; j++)
				// update_codebook((uchar*)pFrameHSV->imageData+j*nchannels, TcodeBook[j],cbBounds,3);
				for (j = 0; j < width*height; j++)
					clear_stale_entries(TcodeBook[j]);
			}

			//if(nFrmNum%30 == 0)
			//{
			// for(j = 0; j < width*height; j++)
			// update_codebook((uchar*)pFrameHSV->imageData+j*nchannels, TcodeBook[j],cbBounds,3);
			//}
			//if(nFrmNum%60 == 0)
			//{
			// for(j = 0; j < width*height; j++)
			// clear_stale_entries(TcodeBook[j]);
			//}
			for (j = 0; j < width*height; j++)
			{
				if (background_diff((uchar*)pFrameHSV->imageData + j*nchannels, TcodeBook[j], 3, minMod, maxMod))
				{
					pFrImg->imageData[j] = 255;
				}
				else
				{
					pFrImg->imageData[j] = 0;
				}
			}
			cvShowImage("foreground", pFrImg);
			cvShowImage("HSV空间图像", pFrameHSV);
		}
		if (cvWaitKey(2) >= 0)
			break;

	} // end of while-loop

	for (j = 0; j < width*height; j++)
	{
		if (!TcodeBook[j].cb)
			delete[] TcodeBook[j].cb;
	}
	if (!TcodeBook)
		delete[] TcodeBook;
	//销毁窗口

	cvDestroyWindow("video");
	cvDestroyWindow("HSV空间图像");
	cvDestroyWindow("foreground");
	return 0;
}