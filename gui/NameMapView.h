#pragma once

#include <QWidget>
#include "ui_NameMapView.h"

QT_BEGIN_NAMESPACE
namespace Ui { class NameMapViewClass; };
QT_END_NAMESPACE

class NameMapView : public QWidget
{
	Q_OBJECT

public:
	NameMapView(QWidget *parent = nullptr);
	~NameMapView();

private:
	Ui::NameMapViewClass *ui;
};

