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
int THRESHOLD_VAL;
int EROSION_SIZE = 0;
int DILATION_SIZE;
int ADAPTIVE_1;
int ADAPTIVE_2;

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
	cv::createTrackbar("Threshold", "AutoDimple", &THRESHOLD_VAL, 255);
	cv::setTrackbarPos("Threshold", "AutoDimple", 255);
	cv::createTrackbar("ErosionSize", "AutoDimple", &EROSION_SIZE, 255);
	cv::setTrackbarPos("ErosionSize", "AutoDimple", 2);
	cv::createTrackbar("DilationSize", "AutoDimple", &DILATION_SIZE, 255);
	cv::setTrackbarPos("DilationSize", "AutoDimple", 11);
	cv::createTrackbar("AdaptiveThresh1", "AutoDimple", &ADAPTIVE_1, 255);	
	cv::setTrackbarPos("AdaptiveThresh1", "AutoDimple", 45);
	cv::createTrackbar("AdaptiveThresh2", "AutoDimple", &ADAPTIVE_2, 255);	
	cv::setTrackbarPos("AdaptiveThresh2", "AutoDimple", 12);
	
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
					cv::THRESH_BINARY, ADAPTIVE_1, ADAPTIVE_2);
		cv::Canny(image, image, THRESHOLD_VAL, THRESHOLD_VAL * 2);

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
		for (int i = 0; i < circles.size(); i++)
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
			int right, top, width, height;
			right = center.x - radius + 10;
			top = center.y - radius + 10;
			(top < 0)? top = 0 : top = top;
			(right < 0)? right = 0: right = right;
			cout << "Top: " << top << " right: " << right;
			width = 2 * radius - 20;
			height = 2 * radius - 20;
			(width + right > 500)? width = 500 : width = width;
			(height + top > 500)? height = 500 : height = height;
			cout << " width: " << width << " height: " << height << endl;
			//roi = roi(cv::Rect(right, top, width, height));

			cv::adaptiveThreshold(roi, roi, 255, cv::ADAPTIVE_THRESH_MEAN_C, 
					cv::THRESH_BINARY, 75, 10);
			
			cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, 
					cv::Size(2 * EROSION_SIZE + 1, 2 * EROSION_SIZE + 1), 
					cv::Point(EROSION_SIZE, EROSION_SIZE));
			cv::erode(roi, roi, element);
			element = cv::getStructuringElement(cv::MORPH_ELLIPSE, 
					cv::Size(2 * DILATION_SIZE + 1, 2 * DILATION_SIZE + 1), 
					cv::Point(DILATION_SIZE, DILATION_SIZE));
			//cv::dilate(roi, roi, element);
			
			cv::GaussianBlur(roi, roi, cv::Size(9, 9), 2, 2);
			cv::Canny(roi, roi, THRESHOLD_VAL, THRESHOLD_VAL * 2);
			cv::HoughCircles(roi, dimpleCircles, CV_HOUGH_GRADIENT, 
					1, image.rows / 2, 100, 25, 10, 170);
			cout << "Circles found: " << dimpleCircles.size() << endl;
			//cv::Mat circleDrawing(cv::Size(roi.cols, roi.rows), CV_8UC1);
			for (int i = 0; i < dimpleCircles.size(); i++)
			{
				dimpleCenter = cv::Point(cvRound(dimpleCircles[i][0]), 
						cvRound(dimpleCircles[i][1]));
				dimpleRadius = cvRound(dimpleCircles[i][2]);
				cv::circle(roi, dimpleCenter, 3, 
						cv::Scalar(255, 255, 255), -1, 8, 0);
				cv::circle(roi, dimpleCenter, dimpleRadius, 
						cv::Scalar(255, 255, 255), 3, 8, 0);
				cv::circle(image, cv::Point(dimpleCenter.x + right - 10, 
							dimpleCenter.y + top - 10), 
						3, cv::Scalar(255, 255, 255), -1, 8, 0);
				cv::circle(image, cv::Point(dimpleCenter.x + right - 10, 
							dimpleCenter.y + top - 10), 
						dimpleRadius, cv::Scalar(255, 255, 255), 3, 8, 0);
				cout << "Center at (" << dimpleCenter.x << "," 
				     << dimpleCenter.y << "), radius "<< dimpleRadius << endl;
			}
		}
		cv::imshow("AutoDimple", origImage);
		cv::imshow("Threshold", roi);
		cv::waitKey(0);
	}

}

