#include <iostream>
#include "Behavior_GUI.h"

Behavior_GUI::Behavior_GUI(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
}

void Behavior_GUI::on_horizontalSlider_valueChanged(int value) {
	std::ofstream log("log.txt");
	log << value << std::endl;
	log.close();
}

int run_camera() // change this into callback for pushbutton
{
	BGAPI_RESULT res = BGAPI_RESULT_FAIL;
	BGAPI_FeatureState state; state.cbSize = sizeof(BGAPI_FeatureState);
	BGAPIX_CameraStatistic statistics; statistics.cbSize = sizeof(BGAPIX_CameraStatistic);

	res = pCamera->setStart(true);
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::Camera::setStart Errorcode: %d\n", res);
		return EXIT_FAILURE;
	}
	printf("Acquisition started\n");

	printf("\n\n=== ENTER TO STOP ===\n\n");
	int d;
	scanf("&d", &d);
	while ((d = getchar()) != '\n' && d != EOF)

		res = pCamera->setStart(false);
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::Camera::setStart Errorcode: %d\n", res);
		return EXIT_FAILURE;
	}

	res = pCamera->getStatistic(&state, &statistics);
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::Camera::getStatistic Errorcode: %d\n", res);
		return EXIT_FAILURE;
	}
	cout << endl << "Camera statistics:" << endl
		<< "  Received Frames Good: " << statistics.statistic[0] << endl
		<< "  Received Frames Corrupted: " << statistics.statistic[1] << endl
		<< "  Lost Frames: " << statistics.statistic[2] << endl
		<< "  Resend Requests: " << statistics.statistic[3] << endl
		<< "  Resend Packets: " << statistics.statistic[4] << endl
		<< "  Lost Packets: " << statistics.statistic[5] << endl
		<< "  Bandwidth: " << statistics.statistic[6] << endl
		<< endl;

	// release all resources ?

	res = pSystem->release();
	if (res != BGAPI_RESULT_OK) {
		printf("BGAPI::System::release Errorcode: %d System index: %d\n", res, sys);
		return EXIT_FAILURE;
	}
	printf("System released: System index %d\n", sys);

	return EXIT_SUCCESS;
}