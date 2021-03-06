#include <ctime>
#include <iostream>
#include <raspicam/raspicam_cv.h>
#include <wiringPi.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <signal.h>
#include <math.h>

using namespace std;
int THRESHOLD_VAL = 255;
int EROSION_SIZE = 15;
int DILATION_SIZE = 15;
int ADAPTIVE_1 = 45;
int ADAPTIVE_2 = 12;

void gracefulShutdown(int s) 
{
	cout << "Caught SIGINT, shutting down." << endl;
	exit(0);
}

int main (int argc, char ** argv)
{
	//Setup graceful shutdown handler
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = gracefulShutdown;
	sigemptyset(&sigIntHandler.sa_mask);
	sigaction(SIGINT, &sigIntHandler, NULL);

	//Initialize camera.
	raspicam::RaspiCam_Cv Camera;
	cv::Mat image;
	cv::Mat roi;
	Camera.set(CV_CAP_PROP_FORMAT, CV_8UC1);
	cout << "Opening camera device..." << endl;
	if (!Camera.open()) { cerr << "Error opening camera!" << endl; return -1; }
	
	//Initialize UI
	cv::namedWindow("AutoDimple", 1);
	cout << "window initialized" << endl;
	//cv::createTrackbar("Threshold", "AutoDimple", &THRESHOLD_VAL, 255);
	//cv::setTrackbarPos("Threshold", "AutoDimple", 255);
	//cv::createTrackbar("ErosionSize", "AutoDimple", &EROSION_SIZE, 255);
	//cv::setTrackbarPos("ErosionSize", "AutoDimple", 2);
	//cv::createTrackbar("DilationSize", "AutoDimple", &DILATION_SIZE, 255);
	//cv::setTrackbarPos("DilationSize", "AutoDimple", 11);
	//cv::createTrackbar("AdaptiveThresh1", "AutoDimple", &ADAPTIVE_1, 255);	
	//cv::setTrackbarPos("AdaptiveThresh1", "AutoDimple", 45);
	//cv::createTrackbar("AdaptiveThresh2", "AutoDimple", &ADAPTIVE_2, 255);	
	//cv::setTrackbarPos("AdaptiveThresh2", "AutoDimple", 12);
	
	Camera.grab();
	Camera.retrieve(image);
	cv::imshow("AutoDimple", image);
	cv::waitKey(1);
	
	for (;;)
	{
		Camera.grab();
		Camera.retrieve(image);
		image = image(cv::Rect(360, 100, 500, 500));
		//cv::imshow("AutoDimple", image);
		//cv::waitKey(1);
		
		cv::Mat origImage;
		image.copyTo(origImage);
		equalizeHist(image, image);
		image.copyTo(roi);
		blur(image, image, cv::Size(3, 3));
		
		cv::adaptiveThreshold(image, image, 255, cv::ADAPTIVE_THRESH_MEAN_C,
					cv::THRESH_BINARY, 45, 12);
		cv::Canny(image, image, 255, 255 * 2);

		cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, 
				cv::Size(2 * DILATION_SIZE + 1, 2 * DILATION_SIZE + 1), 
				cv::Point(DILATION_SIZE, DILATION_SIZE));
		//cv::dilate(image, image, element);
		element = cv::getStructuringElement(cv::MORPH_ELLIPSE, 
				cv::Size(2 * EROSION_SIZE + 1, 2 * EROSION_SIZE + 1), 
				cv::Point(EROSION_SIZE, EROSION_SIZE));
		//cv::erode(image, image, element);
	
		cv::Point center;
		int radius = -1;
		std::vector<cv::Vec3f> circles;
		cv::HoughCircles(image, circles, CV_HOUGH_GRADIENT, 1, 
				image.rows / 2, 200, 25, 190, 210);
		cout << "Circles found: " << circles.size() << endl;
		for (int i = 0; i < (int)circles.size(); i++)
		{
			center = cv::Point(cvRound(circles[i][0]), cvRound(circles[i][1]));
			radius = cvRound(circles[i][2]);
			cv::circle(origImage, center, 3, cv::Scalar(255, 255, 255), -1, 8, 0);
			cv::circle(origImage, center, radius, cv::Scalar(255, 255, 255), 3, 8, 0);
		}
		cout << "Center at (" << center.x << "," << center.y 
		     << "), radius "<< radius << endl;
		
		//Process ROI
		cv::Point dimpleCenter;
		int dimpleRadius;
		vector<cv::Vec3f> dimpleCircles;
		if (circles.size() > 0)
		{
			cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, 
					cv::Size(2 * DILATION_SIZE + 1, 2 * DILATION_SIZE + 1), 
					cv::Point(DILATION_SIZE, DILATION_SIZE));
			cv::dilate(roi, roi, element);
			element = cv::getStructuringElement(cv::MORPH_ELLIPSE, 
					cv::Size(2 * EROSION_SIZE + 1, 2 * EROSION_SIZE + 1), 
					cv::Point(EROSION_SIZE, EROSION_SIZE));
			cv::erode(roi, roi, element);
			cv::HoughCircles(roi, dimpleCircles, CV_HOUGH_GRADIENT, 
					1, roi.rows, 100, 25, 0, 175);
			cout << "Circles found: " << dimpleCircles.size() << endl;
			for (int i = 0; i < (int)dimpleCircles.size(); i++)
			{
				dimpleCenter = cv::Point(cvRound(dimpleCircles[i][0]), 
						cvRound(dimpleCircles[i][1]));
				dimpleRadius = cvRound(dimpleCircles[i][2]);
				cv::circle(origImage, dimpleCenter, 3, cv::Scalar(255, 255, 255), -1, 8, 0);
				cv::circle(roi, dimpleCenter, 3, cv::Scalar(255, 255, 255), -1, 8, 0);
				cv::circle(origImage, dimpleCenter, dimpleRadius, cv::Scalar(255, 255, 255), 3, 8, 0);
				cv::circle(roi, dimpleCenter, dimpleRadius, cv::Scalar(255, 255, 255), 3, 8, 0);
				cout << "Center at (" << dimpleCenter.x << "," 
				     << dimpleCenter.y << "), radius "<< dimpleRadius << endl;
			}
		}
		cv::imshow("AutoDimple", origImage);
		//cv::imshow("Threshold", roi);
		cv::waitKey(0);
	}

}

