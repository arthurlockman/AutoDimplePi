#include <ctime>
#include <iostream>
#include <raspicam/raspicam_cv.h>
#include <wiringPi.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <signal.h>
#include <math.h>
#include <fstream>

#define ENABLE_PIN 15
#define PULSE_PIN 1
#define EMIT_PIN 5
#define DETECT_PIN 4
#define INDEX_COUNTS 3200
#define PULSE_TIME 75

using namespace std;

int carousel_index = 1;
int THRESHOLD_VAL = 255;
int EROSION_SIZE = 15;
int DILATION_SIZE = 15;
int ADAPTIVE_1 = 45;
int ADAPTIVE_2 = 12;

void driveCarousel(int counts)
{
	cout << "Indexing carousel " << counts << " counts." <<endl;
	int spindown = 0;
	for (int i = 1000; i > PULSE_TIME; i -= 4)
	{
		digitalWrite(PULSE_PIN, HIGH);
		delayMicroseconds(i);
		digitalWrite(PULSE_PIN, LOW);
		delayMicroseconds(i);
		counts--;
	}
	for (int i = PULSE_TIME; i < 1000; i += 4)
	{
		spindown++;
	}
	for (int i = 0; i < counts - spindown; i++)
	{
		digitalWrite(PULSE_PIN, HIGH);
		delayMicroseconds(PULSE_TIME);
		digitalWrite(PULSE_PIN, LOW);
		delayMicroseconds(PULSE_TIME);
	}
	for (int i = PULSE_TIME; i < 1000; i += 4)
	{
		digitalWrite(PULSE_PIN, HIGH);
		delayMicroseconds(i);
		digitalWrite(PULSE_PIN, LOW);
		delayMicroseconds(i);
	}
	digitalWrite(PULSE_PIN, LOW);
}

void indexCarousel()
{
	cout << "Carousel at " << carousel_index << endl;
	driveCarousel(INDEX_COUNTS);
	(carousel_index == 19) ? carousel_index = 1 : carousel_index++;
}

void goToIndex(int index)
{
	cout << "Driving to index " << index << endl;
	int slots = abs((index + 19) - carousel_index);
	driveCarousel(INDEX_COUNTS * slots);
	carousel_index = index;
}

void homeCarousel()
{
	//while (carousel_index != 1) { indexCarousel(); }
	goToIndex(1);
}

void gracefulShutdown(int s) 
{
	cout << "Caught SIGINT, shutting down." << endl;
	digitalWrite(ENABLE_PIN, HIGH);
	exit(0);
}

void checkWindow()
{
	int c = cv::waitKey(1);
	if (!cvGetWindowHandle("AutoDimple")) gracefulShutdown(2);
}

int main ( int argc, char ** argv )
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
	
	//Initialize GPIO.
	wiringPiSetup();
	pinMode(ENABLE_PIN, OUTPUT);
	pinMode(PULSE_PIN, OUTPUT);
	pinMode(EMIT_PIN, OUTPUT);
	pinMode(DETECT_PIN, INPUT);

	digitalWrite(EMIT_PIN, HIGH);
	digitalWrite(ENABLE_PIN, LOW);
	
	//Initialize UI
	cv::namedWindow("AutoDimple", 1);
	cout << "window initialized" << endl;
	//cv::createTrackbar("Threshold", "AutoDimple", &THRESHOLD_VAL, 255);
	//cv::setTrackbarPos("Threshold", "AutoDimple", 255);

	//Load Slides
	system("zenity --info --text=\"<b><big>Load Slides</big></b>\n\n To advance carousel, press SPACE. To finish, press ESC.\"");
	cout << "Load slides." << endl;
	int j;
	int loaded_slides = 0;
	for (j = 0; j < 18; j++)
	{
		if (j == 11) indexCarousel();
		cout << "Press space to advance carousel. Press ESC when done." << endl;
		loaded_slides++;
		char k;
		k = cv::waitKey(0);
		if (k == 27) break;
		indexCarousel();
		checkWindow();
	}
	goToIndex(9);
	Camera.grab();
	Camera.retrieve(image);
	cv::imshow("AutoDimple", image);
	cv::waitKey(1000);
	cout << "Loaded slides: " << loaded_slides << endl;
	
	
	//Write image
	time_t t = time(0);
	struct tm * now = localtime(&t);
	ostringstream dataFileName;
	dataFileName << "/home/pi/Desktop/AutoDimple Pictures/images_" << (now->tm_year + 1900) 
		<< "_" << (now->tm_mon + 1) << "_" << (now->tm_mday) <<  "-" 
		<< (now->tm_hour) << "_" << (now->tm_min) << "_" << (now->tm_sec) 
		<< ".csv";
	cout << dataFileName.str() << endl;
	ofstream dataFile;
	dataFile.open(dataFileName.str());
	dataFile << "Image,DVT Radius,DVT Volume,DVT X Offset,DVT Y Offset" << endl;
	for (int i = 0; i < loaded_slides; i++)
	{
		checkWindow();
		if (i == 11) indexCarousel();
		
		cout << "Grabbing image..." << endl;
		Camera.grab();
		Camera.retrieve(image);
		image = image(cv::Rect(360, 100, 500, 500));
		//cv::imshow("AutoDimple", image);
		cv::waitKey(1);
		
		cv::Mat origImage;
		image.copyTo(origImage);
		equalizeHist(image, image);
		image.copyTo(roi);
		blur(image, image, cv::Size(3, 3));
		
		cv::adaptiveThreshold(image, image, 255, cv::ADAPTIVE_THRESH_MEAN_C,
					cv::THRESH_BINARY, 45, 12);
		cv::Canny(image, image, 255, 255 * 2);

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
			for (int i = 0; i < dimpleCircles.size(); i++)
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
		cv::waitKey(1);
		cv::waitKey(1);
		indexCarousel();
		
		//Write image
		time_t t = time(0);
		struct tm * now = localtime(&t);
		ostringstream os;
		os << "/home/pi/Desktop/AutoDimple Pictures/" << (now->tm_year + 1900) 
			<< "_" << (now->tm_mon + 1) << "_" << (now->tm_mday) <<  "-" 
			<< (now->tm_hour) << "_" << (now->tm_min) << "_" << (now->tm_sec) 
			<< ".jpg";
		cout << os.str() << endl;
		cv::imwrite(os.str(), origImage);
		
		//Write data file
		dataFile << os.str() << "," << dimpleRadius << "," << "vol" << "," 
			<< (dimpleCenter.x - center.x) << "," << (center.y - dimpleCenter.y) << endl;
		checkWindow();
	}
	Camera.release();
	dataFile.close();
	//Eject Slides
	goToIndex(4);
	//while (carousel_index != 4) { indexCarousel(); }
	system("zenity --info --text=\"<b><big>Eject Slides</big></b>\n\nPull slides out of eject slot.\n\nTo advance carousel, press SPACE. To finish, press ESC.\"");
	for (j = 0; j < 19; j++)
	{
		checkWindow();
		char k;
		k = cv::waitKey(0);
		if (k ==  27) break;
		indexCarousel();
	}

	//system("pcmanfm /home/pi/Desktop/AutoDimple\\ Pictures/");
	homeCarousel();
	digitalWrite(ENABLE_PIN, HIGH);
	system("zenity --info --text=\"<b><big>Complete!</big></b>\n\nProcess complete! Data can be found on the desktop in the <i>AutoDimple Pictures</i> folder.\"");
	return 0;
}

