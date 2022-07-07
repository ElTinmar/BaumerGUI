#include "Behavior_GUI.h"
#include <QtWidgets/QApplication>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <iterator>
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <inttypes.h>
using namespace std;

// Baumer SDK : camera SDK
#include "bgapi.hpp"

// OPENCV : display preview and save video 
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>

using namespace std;

#define NUMBUFFER 100

// Global variables-------------------------------------------------------------
int sys = 0;
int cam = 0;
BGAPI::System * pSystem = NULL;
BGAPI::Camera * pCamera = NULL;
BGAPI::Image * pImage[NUMBUFFER];
cv::VideoWriter writer;
std::ofstream file;
BGAPIX_TypeINT iTimeHigh, iTimeLow, iFreqHigh, iFreqLow;
BGAPIX_CameraImageFormat cformat;
cv::Mat img_display;
uint64_t first_ts = 0;
uint64_t current_ts = 0;
uint64_t previous_ts = 0;
int cropleft = 0;
int croptop = 0;
int height = 100;
int width = 100;
int gainvalue = 0;
int gainmax = 0;
int exposurevalue = 0;
int exposuremax = 0;
int subsamplemax = 4;
int triggers = 0;
int fps = 0;
int fpsmax = 0;
int formatindex = 0;
int formatindexmax = 0;
int preview;
int subsample;
string result_dir;

BGAPI_RESULT BGAPI_CALLBACK imageCallback(void * callBackOwner, BGAPI::Image* pCurrImage)
{
	cv::Mat img;
	cv::Mat img_resized;
	int swc;
	int hwc;
	int timestamplow = 0;
	int timestamphigh = 0;
	uint32_t timestamplow_u = 0;
	uint32_t timestamphigh_u = 0;
	BGAPI_RESULT res = BGAPI_RESULT_OK;

	unsigned char* imagebuffer = NULL;
	res = pCurrImage->get(&imagebuffer);
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::Image::get Errorcode: %d\n", res);
		return 0;
	}

	res = pCurrImage->getNumber(&swc, &hwc);
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::Image::getNumber Errorcode: %d\n", res);
		return 0;
	}

	res = pCurrImage->getTimeStamp(&timestamphigh, &timestamplow);
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::Image::getTimeStamp Errorcode: %d\n", res);
		return 0;
	}
	timestamplow_u = timestamplow;
	timestamphigh_u = timestamphigh;
	if (swc == 0) {
		first_ts = (uint64_t)timestamphigh_u << 32 | timestamplow_u;
	}
	current_ts = (uint64_t)timestamphigh_u << 32 | timestamplow_u;
	double current_time = (double)(current_ts - first_ts) / (double)iFreqLow.current;
	double fps_hat = (double)iFreqLow.current / (double)(current_ts - previous_ts);
	previous_ts = current_ts;

	img = cv::Mat(cv::Size(subsample*width, subsample*height), CV_8U, imagebuffer);
	cv::resize(img, img_resized, cv::Size(), 1.0 / subsample, 1.0 / subsample);

	if (!preview) {
		file << setprecision(3) << std::fixed << 1000 * current_time << std::endl;
		writer << img_resized;
	}

	img_resized.copyTo(img_display);
	cv::putText(img_display, to_string(swc), cv::Point(10, 30), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 0, 0), 2, cv::LINE_AA, false);
	cv::putText(img_display, to_string(fps_hat), cv::Point(width - 100, 30), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 0, 0), 2, cv::LINE_AA, false);
	cv::putText(img_display, to_string(current_time) + " s", cv::Point(10, height - 10), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 0, 0), 2, cv::LINE_AA, false);

	res = ((BGAPI::Camera*)callBackOwner)->setImage(pCurrImage);
	if (res != BGAPI_RESULT_OK) {
		printf("setImage failed with %d\n", res);
	}
	return res;
}

int setup_camera() {

	BGAPI_RESULT res = BGAPI_RESULT_FAIL;
	BGAPI_FeatureState state;
	BGAPIX_TypeROI roi;
	BGAPIX_TypeRangeFLOAT gain;
	BGAPIX_TypeRangeFLOAT framerate;
	BGAPIX_TypeRangeINT exposure;
	BGAPIX_TypeListINT imageformat;

	cformat.cbSize = sizeof(BGAPIX_CameraImageFormat);
	state.cbSize = sizeof(BGAPI_FeatureState);
	iTimeHigh.cbSize = sizeof(BGAPIX_TypeINT);
	iTimeLow.cbSize = sizeof(BGAPIX_TypeINT);
	iFreqHigh.cbSize = sizeof(BGAPIX_TypeINT);
	iFreqLow.cbSize = sizeof(BGAPIX_TypeINT);
	roi.cbSize = sizeof(BGAPIX_TypeROI);
	gain.cbSize = sizeof(BGAPIX_TypeRangeFLOAT);
	exposure.cbSize = sizeof(BGAPIX_TypeRangeINT);
	framerate.cbSize = sizeof(BGAPIX_TypeRangeFLOAT);
	imageformat.cbSize = sizeof(BGAPIX_TypeListINT);

	// Initializing the system--------------------------------------------------
	res = BGAPI::createSystem(sys, &pSystem);
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::createSystem Errorcode: %d System index: %d\n", res, sys);
		return EXIT_FAILURE;
	}
	printf("Created system: System index %d\n", sys);

	res = pSystem->open();
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::System::open Errorcode: %d System index: %d\n", res, sys);
		return EXIT_FAILURE;
	}
	printf("System opened: System index %d\n", sys);

	res = pSystem->createCamera(cam, &pCamera);
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::System::createCamera Errorcode: %d Camera index: %d\n", res, cam);
		return EXIT_FAILURE;
	}
	printf("Created camera: Camera index %d\n", cam);

	res = pCamera->open();
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::Camera::open Errorcode: %d Camera index: %d\n", res, cam);
		return EXIT_FAILURE;
	}
	printf("Camera opened: Camera index %d\n", cam);

	res = pCamera->setDataAccessMode(BGAPI_DATAACCESSMODE_QUEUEDINTERN, NUMBUFFER);
	if (res != BGAPI_RESULT_OK)
	{
		printf("BGAPI::Camera::setDataAccessMode Errorcode %d\n", res);
		return EXIT_FAILURE;
	}

	int i = 0;
	for (i = 0; i < NUMBUFFER; i++)
	{
		res = BGAPI::createImage(&pImage[i]);
		if (res != BGAPI_RESULT_OK)
		{
			printf("BGAPI::createImage for Image %d Errorcode %d\n", i, res);
			break;
		}
	}
	printf("Images created successful!\n");

	for (i = 0; i < NUMBUFFER; i++)
	{
		res = pCamera->setImage(pImage[i]);
		if (res != BGAPI_RESULT_OK)
		{
			printf("BGAPI::System::setImage for Image %d Errorcode %d\n", i, res);
			break;
		}
	}
	printf("Images allocated successful!\n");

	res = pCamera->registerNotifyCallback(pCamera, (BGAPI::BGAPI_NOTIFY_CALLBACK) &imageCallback);
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::Camera::registerNotifyCallback Errorcode: %d\n", res);
		return EXIT_FAILURE;
	}

	res = pCamera->getImageFormat(&state, &imageformat);
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::Camera::setImageFormat Errorcode: %d\n", res);
	}
	formatindex = imageformat.current;
	formatindexmax = imageformat.length;

	res = pCamera->getImageFormatDescription(formatindex, &cformat);
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::Camera::getImageFormatDescription Errorcode: %d\n", res);
		return EXIT_FAILURE;
	}

	res = pCamera->getGain(&state, &gain);
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::Camera::setGain Errorcode: %d\n", res);
		return EXIT_FAILURE;
	}
	gainvalue = gain.current;
	gainmax = gain.maximum;

	res = pCamera->getExposure(&state, &exposure);
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::Camera::setExposure Errorcode: %d\n", res);
		return EXIT_FAILURE;
	}
	exposurevalue = exposure.current;
	exposuremax = exposure.maximum;

	res = pCamera->getPartialScan(&state, &roi);
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::Camera::getImageFormat Errorcode: %d\n", res);
		return EXIT_FAILURE;
	}
	cropleft = roi.curleft;
	croptop = roi.curtop;
	width = roi.curright - roi.curleft;
	height = roi.curbottom - roi.curtop;

	res = pCamera->getFramesPerSecondsContinuous(&state, &framerate);
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::Camera::getFramesPerSecondsContinuous Errorcode: %d\n", res);
		return EXIT_FAILURE;
	}
	fps = framerate.current;
	fpsmax = framerate.maximum;

	res = pCamera->getTrigger(&state);
	if (res != BGAPI_RESULT_OK)
	{
		printf("BGAPI::Camera::getTrigger Errorcode: %d\n", res);
		return EXIT_FAILURE;
	}
	triggers = state.bIsEnabled;

	res = pCamera->setReadoutMode(BGAPI_READOUTMODE_OVERLAPPED);
	if (res != BGAPI_RESULT_OK)
	{
		if (res == BGAPI_RESULT_FEATURE_NOTIMPLEMENTED) {
			printf("BGAPI::Camera::setReadoutMode not implemented, ignoring\n");
		}
		else {
			printf("BGAPI::Camera::setReadoutMode Errorcode: %d\n", res);
			return EXIT_FAILURE;
		}
	}

	res = pCamera->setSensorDigitizationTaps(BGAPI_SENSORDIGITIZATIONTAPS_SIXTEEN);
	if (res != BGAPI_RESULT_OK)
	{
		if (res == BGAPI_RESULT_FEATURE_NOTIMPLEMENTED) {
			printf("BGAPI::Camera::setSensorDigitizationTaps not implemented, ignoring\n");
		}
		else {
			printf("BGAPI::Camera::setSensorDigitizationTaps Errorcode: %d\n", res);
			return EXIT_FAILURE;
		}
	}

	res = pCamera->setExposureMode(BGAPI_EXPOSUREMODE_TIMED);
	if (res != BGAPI_RESULT_OK)
	{
		if (res == BGAPI_RESULT_FEATURE_NOTIMPLEMENTED) {
			printf("BGAPI::Camera::setExposureMode not implemented, ignoring\n");
		}
		else {
			printf("BGAPI::Camera::setExposureMode Errorcode: %d\n", res);
			return EXIT_FAILURE;
		}
	}

	res = pCamera->getTimeStamp(&state, &iTimeHigh, &iTimeLow, &iFreqHigh, &iFreqLow);
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::Camera::getTimeStamp Errorcode: %d\n", res);
		return EXIT_FAILURE;
	}
	printf("Timestamps frequency [%d,%d]\n", iFreqHigh.current, iFreqLow.current);

	res = pCamera->setFrameCounter(0, 0);
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::Camera::setFrameCounter Errorcode: %d\n", res);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	setup_camera();

	QApplication a(argc, argv);
	Behavior_GUI w;
	w.show();
	return a.exec();
}

