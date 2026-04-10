#pragma once
#include"memory.h"
#include"gui/NameMapView.h"
template<class T>
using uniquePtr = std::unique_ptr<T>;
class MainClass {
public:
	uniquePtr<NameMapView> nameMapView_ui;
	MainClass():nameMapView_ui(new NameMapView){
	
	}
	void showAll() {
		nameMapView_ui->show();
	}
};