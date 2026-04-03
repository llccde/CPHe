#include "NameMapView.h"

NameMapView::NameMapView(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::NameMapViewClass())
{
	ui->setupUi(this);
}

NameMapView::~NameMapView()
{
	delete ui;
}

