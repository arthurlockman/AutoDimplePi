#include <ctime>
#include <iostream>
#include <raspicam/raspicam_cv.h>
extern "C" {
#include <wiringPi.h>
}
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
	pinMode(0, OUTPUT);
	//Do stuff here
	
	Camera.release();
	return 0;
}

