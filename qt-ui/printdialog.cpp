#include "printdialog.h"
#include "printoptions.h"
#include "mainwindow.h"

#ifndef NO_PRINTING
#include <QProgressBar>
#include <QPrintPreviewDialog>
#include <QPrintDialog>
#include <QShortcut>
#include <QSettings>
#include <QMessageBox>

#define SETTINGS_GROUP "PrintDialog"

template_options::color_palette_struct almond_colors, blueshades_colors, custom_colors;

PrintDialog::PrintDialog(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	// initialize const colors
	almond_colors.color1 = QColor::fromRgb(243, 234, 207);
	almond_colors.color2 = QColor::fromRgb(253, 204, 156);
	almond_colors.color3 = QColor::fromRgb(136, 160, 150);
	almond_colors.color4 = QColor::fromRgb(187, 171, 139);
	almond_colors.color5 = QColor::fromRgb(239, 130, 117);
	blueshades_colors.color1 = QColor::fromRgb(182, 192, 206);
	blueshades_colors.color2 = QColor::fromRgb(142, 152, 166);
	blueshades_colors.color3 = QColor::fromRgb(31, 49, 75);
	blueshades_colors.color4 = QColor::fromRgb(21, 45, 84);
	blueshades_colors.color5 = QColor::fromRgb(5, 25, 56);

	// check if the options were previously stored in the settings; if not use some defaults.
	QSettings s;
	bool stored = s.childGroups().contains(SETTINGS_GROUP);
	if (!stored) {
		printOptions.print_selected = true;
		printOptions.color_selected = true;
		printOptions.landscape = false;
		printOptions.p_template = "one_dive.html";
		printOptions.type = print_options::DIVELIST;
		templateOptions.font_index = 0;
		templateOptions.font_size = 9;
		templateOptions.color_palette_index = ALMOND;
		templateOptions.line_spacing = 1;
		custom_colors = almond_colors;
	} else {
		s.beginGroup(SETTINGS_GROUP);
		printOptions.type = (print_options::print_type)s.value("type").toInt();
		printOptions.print_selected = s.value("print_selected").toBool();
		printOptions.color_selected = s.value("color_selected").toBool();
		printOptions.landscape = s.value("landscape").toBool();
		printOptions.p_template = s.value("template_selected").toString();
		qprinter.setOrientation((QPrinter::Orientation)printOptions.landscape);
		templateOptions.font_index = s.value("font").toInt();
		templateOptions.font_size = s.value("font_size").toDouble();
		templateOptions.color_palette_index = s.value("color_palette").toInt();
		templateOptions.line_spacing = s.value("line_spacing").toDouble();
		custom_colors.color1 = QColor(s.value("custom_color_1").toString());
		custom_colors.color2 = QColor(s.value("custom_color_2").toString());
		custom_colors.color3 = QColor(s.value("custom_color_3").toString());
		custom_colors.color4 = QColor(s.value("custom_color_4").toString());
		custom_colors.color5 = QColor(s.value("custom_color_5").toString());
	}

	// handle cases from old QSettings group
	if (templateOptions.font_size < 9) {
		templateOptions.font_size = 9;
	}
	if (templateOptions.line_spacing < 1) {
		templateOptions.line_spacing = 1;
	}

	switch (templateOptions.color_palette_index) {
	case ALMOND: // almond
		templateOptions.color_palette = almond_colors;
		break;
	case BLUESHADES: // blueshades
		templateOptions.color_palette = blueshades_colors;
		break;
	case CUSTOM: // custom
		templateOptions.color_palette = custom_colors;
		break;
	}

	// create a print options object and pass our options struct
	optionsWidget = new PrintOptions(this, &printOptions, &templateOptions);

	// create a new printer object
	printer = new Printer(&qprinter, &printOptions, &templateOptions, Printer::PRINT);

	QVBoxLayout *layout = new QVBoxLayout(this);
	setLayout(layout);

	layout->addWidget(optionsWidget);

	progressBar = new QProgressBar();
	progressBar->setMinimum(0);
	progressBar->setMaximum(100);
	progressBar->setValue(0);
	progressBar->setTextVisible(false);
	layout->addWidget(progressBar);

	QHBoxLayout *hLayout = new QHBoxLayout();
	layout->addLayout(hLayout);

	QPushButton *printButton = new QPushButton(tr("P&rint"));
	connect(printButton, SIGNAL(clicked(bool)), this, SLOT(printClicked()));

	QPushButton *previewButton = new QPushButton(tr("&Preview"));
	connect(previewButton, SIGNAL(clicked(bool)), this, SLOT(previewClicked()));

	QDialogButtonBox *buttonBox = new QDialogButtonBox;
	buttonBox->addButton(QDialogButtonBox::Cancel);
	buttonBox->addButton(printButton, QDialogButtonBox::AcceptRole);
	buttonBox->addButton(previewButton, QDialogButtonBox::ActionRole);

	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	hLayout->addWidget(buttonBox);

	setWindowTitle(tr("Print"));
	setWindowIcon(QIcon(":subsurface-icon"));

	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));

	// seems to be the most reliable way to track for all sorts of dialog disposal.
	connect(this, SIGNAL(finished(int)), this, SLOT(onFinished()));
}

void PrintDialog::onFinished()
{
	QSettings s;
	s.beginGroup(SETTINGS_GROUP);

	// save print paper settings
	s.setValue("type", printOptions.type);
	s.setValue("print_selected", printOptions.print_selected);
	s.setValue("color_selected", printOptions.color_selected);
	s.setValue("template_selected", printOptions.p_template);

	// save template settings
	s.setValue("font", templateOptions.font_index);
	s.setValue("font_size", templateOptions.font_size);
	s.setValue("color_palette", templateOptions.color_palette_index);
	s.setValue("line_spacing", templateOptions.line_spacing);

	// save custom colors
	s.setValue("custom_color_1", custom_colors.color1.name());
	s.setValue("custom_color_2", custom_colors.color2.name());
	s.setValue("custom_color_3", custom_colors.color3.name());
	s.setValue("custom_color_4", custom_colors.color4.name());
	s.setValue("custom_color_5", custom_colors.color5.name());
}

void PrintDialog::previewClicked(void)
{
	if (printOptions.type == print_options::STATISTICS) {
		QMessageBox msgBox;
		msgBox.setText("This feature is not implemented yet");
		msgBox.exec();
		return;
	}

	QPrintPreviewDialog previewDialog(&qprinter, this, Qt::Window
		| Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint
		| Qt::WindowTitleHint);
	connect(&previewDialog, SIGNAL(paintRequested(QPrinter *)), this, SLOT(onPaintRequested(QPrinter *)));
	previewDialog.exec();
}

void PrintDialog::printClicked(void)
{
	if (printOptions.type == print_options::STATISTICS) {
		QMessageBox msgBox;
		msgBox.setText("This feature is not implemented yet");
		msgBox.exec();
		return;
	}

	QPrintDialog printDialog(&qprinter, this);
	if (printDialog.exec() == QDialog::Accepted) {
		switch (printOptions.type) {
		case print_options::DIVELIST:
			connect(printer, SIGNAL(progessUpdated(int)), progressBar, SLOT(setValue(int)));
			printer->print();
			break;
		case print_options::STATISTICS:
			break;
		}
		close();
	}
}

void PrintDialog::onPaintRequested(QPrinter *printerPtr)
{
	connect(printer, SIGNAL(progessUpdated(int)), progressBar, SLOT(setValue(int)));
	printer->print();
	progressBar->setValue(0);
	disconnect(printer, SIGNAL(progessUpdated(int)), progressBar, SLOT(setValue(int)));
}
#endif
