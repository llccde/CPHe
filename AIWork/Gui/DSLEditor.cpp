#include "DSLEditor.h"

DSLEditor::DSLEditor(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::DSLEditorClass())
{
	ui->setupUi(this);
}

DSLEditor::~DSLEditor()
{
	delete ui;
}

