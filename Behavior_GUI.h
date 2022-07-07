#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Behavior_GUI.h"
#include <fstream>

class Behavior_GUI : public QMainWindow
{
	Q_OBJECT

public:
	Behavior_GUI(QWidget *parent = Q_NULLPTR);

private:
	Ui::Behavior_GUIClass ui;

private slots:
	void on_horizontalSlider_valueChanged(int value);
};
