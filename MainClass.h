#pragma once
#include"memory.h"
#include"gui/NameMapView.h"
#include"gui/PSCMainWindow.h"
template<class T>
using uniquePtr = std::unique_ptr<T>;
class MainClass {
public:
	uniquePtr<NameMapView> nameMapView_ui;
	uniquePtr<PSCMainWindow> mainwindow;
	
	MainClass():nameMapView_ui(new NameMapView){
		mainwindow.reset(new PSCMainWindow());
	}
	void showAll() {
		mainwindow->show();
	}
};