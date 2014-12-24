#include <ctime>
#include <iostream>
#include <raspicam/raspicam_cv.h>
#include <wiringPi.h>

#define DIR_PIN 0
#define PULSE_PIN 1
#define INDEX_COUNTS 400

using namespace std;


void indexCarousel()
{
	digitalWrite(DIR_PIN, HIGH);
	for (int i = 0; i < INDEX_COUNTS; i++)
	{
		digitalWrite(PULSE_PIN, HIGH);
		delay(1);
		digitalWrite(PULSE_PIN, LOW);
		delay(1);
	}
} 

int main ( int argc, char ** argv )
{
	//Initialize camera.
	raspicam::RaspiCam_Cv Camera;
	cv::Mat image;
	Camera.set(CV_CAP_PROP_FORMAT, CV_8UC1);
	cout << "Opening camera device..." << endl;
	if (!Camera.open()) { cerr << "Error opening camera!" << endl; return -1; }
	
	//Initialize GPIO.
	wiringPiSetup();
	pinMode(DIR_PIN, OUTPUT);
	pinMode(PULSE_PIN, OUTPUT);
	
	//Initialize UI
	cv::namedWindow("AutoDimple", 1);

	//Do stuff
	Camera.grab();
	Camera.retrieve(image);
	cv::imshow("AutoDimple", image);
	for (int i = 0; i < 16; i++)
	{
		Camera.grab();
		Camera.retrieve(image);
		cv::imshow("AutoDimple", image);
		indexCarousel();
		cv::waitKey(0);
	}
	Camera.release();
	cv::waitKey(0);
	return 0;
}


