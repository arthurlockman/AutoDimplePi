#include <ctime>
#include <iostream>
#include <raspicam/raspicam_cv.h>
#include <wiringPi.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <signal.h>
#include <math.h>

#define ENABLE_PIN 15
#define PULSE_PIN 1
#define EMIT_PIN 5
#define DETECT_PIN 4
#define INDEX_COUNTS 3200
#define PULSE_TIME 75

using namespace std;

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

void gracefulShutdown(int s) 
{
	digitalWrite(ENABLE_PIN, HIGH);
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
	
	//Initialize GPIO.
	wiringPiSetup();
	pinMode(ENABLE_PIN, OUTPUT);
	pinMode(PULSE_PIN, OUTPUT);

	digitalWrite(ENABLE_PIN, LOW);
	
	//Initialize UI
	cv::namedWindow("Image", 1);
	cout << "window initialized" << endl;
	
	for (;;)
	{
		Camera.grab();
		Camera.retrieve(image);
		image = image(cv::Rect(360, 100, 500, 500));

		cv::imshow("Image", image);
		char k = cv::waitKey(0);
		if (k == 83 || k == 115) //if S key pressed
		{
			//Save image.
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
		driveCarousel(INDEX_COUNTS);
	}
}

