///////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer
// Copyright (C) 2014 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version, but always including the *additional*
// restrictions defined in the "License.txt" file.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
///////////////////////////////////////////////////////////////////////////////

//Std
#include <cstdlib>
#include <cfloat>

//Qt
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QTabWidget>
#include <QPlastiqueStyle>
#include <QRegExp>

//Internal
#include "3rd_party/qcustomplot.h"

typedef struct
{
	QVector<double> original;
	QVector<double> minimal;
	QVector<double> smoothed;
}
LogFileData;

static void makeAxisBold(QCPAxis *axis)
{
	QFont font = axis->labelFont();
	font.setBold(true);
	axis->setLabelFont(font);

}

static bool parseHeader(QTextStream &textStream, unsigned int &channels)
{
	QRegExp header("^DynamicAudioNormalizer Logfile v(\\d).(\\d\\d)-(\\d)$");
	QRegExp channelCount("^CHANNEL_COUNT:(\\d+)$");

	QString headerLine = textStream.readLine();
	if(header.indexIn(headerLine) < 0)
	{
		return false;
	}
	if(header.cap(1).toUInt() != 2)
	{
		return false;
	}

	QString channelLine = textStream.readLine();
	if(channelCount.indexIn(channelLine) < 0)
	{
		return false;
	}

	bool ok = false;
	channels = channelCount.cap(1).toUInt(&ok);
	
	return ok && (channels > 0);
}

static void parseData(QTextStream &textStream, unsigned int &channels, LogFileData *data, double &minValue, double &maxValue)
{
	while((!textStream.atEnd()) && (textStream.status() == QTextStream::Ok))
	{
		const QString line = textStream.readLine().simplified();
		if(!line.isEmpty())
		{
			const QStringList tokens = line.split(' ', QString::SkipEmptyParts);
			if(((unsigned int) tokens.count()) == (channels * 3))
			{
				QStringList::ConstIterator iter = tokens.constBegin();
				for(unsigned int c = 0; c < channels; c++)
				{
					data[c].original.append((iter++)->toDouble());
					data[c].minimal .append((iter++)->toDouble());
					data[c].smoothed.append((iter++)->toDouble());

					minValue = qMin(minValue, data[c].original.back());
					minValue = qMin(minValue, data[c].minimal .back());
					minValue = qMin(minValue, data[c].smoothed.back());

					maxValue = qMax(maxValue, data[c].original.back());
					maxValue = qMax(maxValue, data[c].minimal .back());
					maxValue = qMax(maxValue, data[c].smoothed.back());
				}
			}
		}
	}
}

static void createGraph(QCustomPlot *plot, QVector<double> &steps, QVector<double> &data, const Qt::GlobalColor &color, const Qt::PenStyle &style, const int width)
{
	QCPGraph *graph = plot->addGraph();
	QPen pen(color);
	pen.setStyle(style);
	pen.setWidth(width);
	graph->setPen(pen);
	graph->setData(steps, data);
}

static int dynamicNormalizerGuiMain(int argc, char* argv[])
{
	QApplication app(argc, argv);
	app.setWindowIcon(QIcon(":/res/chart_curve.png"));
	app.setStyle(new QPlastiqueStyle());

	QTabWidget window;
	window.setWindowTitle(QString("Dynamic Audio Normalizer - Log Viewer"));
	window.setMinimumSize(640, 480);
	window.show();

	const QString logFileName = QFileDialog::getOpenFileName(&window, "Open Log File", QString(), QString("Log File (*.log)"));
	if(logFileName.isEmpty())
	{
		return EXIT_FAILURE;
	}

	QFile logFile(logFileName);
	if(!logFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::critical(&window, QString("Dynamic Audio Normalizer"), QString("Error: The selected log file could not opened for reading!"));
		return EXIT_FAILURE;
	}

	QTextStream textStream(&logFile);
	textStream.setCodec("UTF-8");

	unsigned int channels = 0;

	if(!parseHeader(textStream, channels))
	{
		QMessageBox::critical(&window, QString("Dynamic Audio Normalizer"), QString("Error: Failed to parse the header of the log file!\nProbably the file is of an unsupported type."));
		return EXIT_FAILURE;
	}

	double minValue = DBL_MAX;
	double maxValue = 0.0;

	LogFileData *data = new LogFileData[channels];
	QCustomPlot *plot = new QCustomPlot[channels];

	parseData(textStream, channels, data, minValue, maxValue);

	unsigned int length = 0;
	for(unsigned int c = 0; c < channels; c++)
	{
		unsigned int temp = UINT_MAX;
		temp = qMin(temp, (unsigned int) data[c].original.count());
		temp = qMin(temp, (unsigned int) data[c].minimal .count());
		temp = qMin(temp, (unsigned int) data[c].smoothed.count());
		length =  qMax(length, temp);
	}

	QVector<double> steps(length);
	for(unsigned int i = 0; i < length; i++)
	{
		steps[i] = double(i);
	}

	for(unsigned int c = 0; c < channels; c++)
	{
		// create graph and assign data to it:
		createGraph(&plot[c], steps, data[c].original, Qt::blue,  Qt::SolidLine, 1);
		createGraph(&plot[c], steps, data[c].minimal,  Qt::green, Qt::SolidLine, 1);
		createGraph(&plot[c], steps, data[c].smoothed, Qt::red,   Qt::DashLine,  2);

		// give the axes some labels:
		plot[c].xAxis->setLabel("Frame #");
		plot[c].yAxis->setLabel("Gain Factor");
		makeAxisBold(plot[c].xAxis);
		makeAxisBold(plot[c].yAxis);

		// set axes ranges, so we see all data:
		plot[c].xAxis->setRange(0.0, length);
		plot[c].yAxis->setRange(minValue - 0.25, maxValue + 0.25);
		plot[c].yAxis->setScaleType(QCPAxis::stLinear);
		plot[c].yAxis->setTickStep(1.0);
		plot[c].yAxis->setAutoTickStep(false);

		// add title
		plot[c].plotLayout()->insertRow(0);
		plot[c].plotLayout()->addElement(0, 0, new QCPPlotTitle(&plot[c], QString("%1 (Channel %2/%3)").arg(QFileInfo(logFile).fileName(), QString::number(c+1), QString::number(channels))));

		// show the plot
		plot[c].replot();
		window.addTab(&plot[c], QString().sprintf("Channel #%u", c + 1));
	}

	// run application
	window.showMaximized();
	app.exec();

	// clean up
	delete [] data;
	delete [] plot;

	return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
	int exitCode = EXIT_SUCCESS;

	try
	{
		exitCode = dynamicNormalizerGuiMain(argc, argv);
	}
	catch(std::exception &e)
	{
		fprintf(stderr, "\n\nGURU MEDITATION: Unhandeled C++ exception error: %s\n\n", e.what());
		exitCode = EXIT_FAILURE;
	}
	catch(...)
	{
		fprintf(stderr, "\n\nGURU MEDITATION: Unhandeled unknown C++ exception error!\n\n");
		exitCode = EXIT_FAILURE;
	}

	return exitCode;
}
