#include <ctime>
#include <iostream>
#include <raspicam/raspicam_cv.h>
#include <wiringPi.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#define ENABLE_PIN 15
#define PULSE_PIN 1
#define EMIT_PIN 5
#define DETECT_PIN 4
#define INDEX_COUNTS 3200

using namespace std;

void indexCarousel()
{
	for (int i = 0; i < INDEX_COUNTS; i++)
	{
		digitalWrite(PULSE_PIN, HIGH);
		delayMicroseconds(100);
		digitalWrite(PULSE_PIN, LOW);
		delayMicroseconds(100);
	}
} 

int main ( int argc, char ** argv )
{
	//Initialize camera.
	raspicam::RaspiCam_Cv Camera;
	cv::Mat image;
	cv::Mat baseImage;
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
	
	//Load Slides
	system("zenity --info --text=\"<b><big>Load Slides</big></b>\n\n To advance carousel, press SPACE. To finish, press ESC.\"");
	cout << "Load slides." << endl;
	int j;
	for (j = 0; j < 19; j++)
	{
		cout << "Press space to advance carousel. Press ESC when done." << endl;
		char k;
		k = cv::waitKey(0);
		if (k == 27) break;
		indexCarousel();
	}
	for (; j < 27; j++)
	{
		indexCarousel();
	}

	Camera.grab();
	Camera.retrieve(image);
	cv::imshow("AutoDimple", image);
	cv::waitKey(1000);
	for (int i = 0; i < 19; i++)
	{
		delay(1000);
		cout << "Grabbing image..." << endl;
		Camera.grab();
		Camera.retrieve(image);
		baseImage = image.clone();
		image = image(cv::Rect(260, 0, 700, 700));
		equalizeHist(image, image);
		blur(image, image, cv::Size(3, 3));
		std::vector<cv::Vec3f> circles;
		cv::HoughCircles(image, circles, CV_HOUGH_GRADIENT, image.rows, 200, 100, 1, 1000);
		cout << "Circles found: " << circles.size() << endl;
		for (int i = 0; i < circles.size(); i++)
		{
			cv::Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
			int radius = cvRound(circles[i][2]);
			printf("radius: %d", radius);
			cv::circle(image, center, 3, cv::Scalar(255, 255, 255), -1, 8, 0);
			cv::circle(image, center, radius, cv::Scalar(255, 255, 255), 3, 8, 0);
		}
		cv::imshow("AutoDimple", image);
		cv::waitKey(1000);
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
		cv::imwrite(os.str(), image);
	}
	Camera.release();

	//Eject Slides
	system("zenity --info --text=\"<b><big>Eject Slides</big></b>\n\nPull slides out of eject slot.\n\nTo advance carousel, press SPACE. To finish, press ESC.\"");
	for (j = 0; j < 19; j++)
	{
		char k;
		k = cv::waitKey(0);
		if (k ==  27) break;
		indexCarousel();
	}
	system("zenity --info --text=\"<b><big>Complete!</big></b>\n\nProcess complete! Data can be found on the desktop in the <i>AutoDimple Pictures</i> folder.\"");
	//system("pcmanfm /home/pi/Desktop/AutoDimple\\ Pictures/");
	digitalWrite(ENABLE_PIN, HIGH);
	return 0;
}

