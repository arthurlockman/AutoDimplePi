#include <ctime>
#include <iostream>
#include <raspicam/raspicam_cv.h>
#include <wiringPi.h>

#define DIR_PIN 0
#define PULSE_PIN 1
#define INDEX_COUNTS 400

using namespace std;

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
	//Do stuff here
	
	Camera.release();
	return 0;
}

static void indexCarousel()
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

