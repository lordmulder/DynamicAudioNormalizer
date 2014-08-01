//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Utility
// Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
//////////////////////////////////////////////////////////////////////////////////

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

//MDynamicAudioNormalizer API
#include "DynamicAudioNormalizer.h"

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
			const QStringList tokens = line.split(QChar(0x20), QString::SkipEmptyParts);
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
	uint32_t versionMajor, versionMinor, versionPatch;
	MDynamicAudioNormalizer::getVersionInfo(versionMajor, versionMinor, versionPatch);

	QApplication app(argc, argv);
	app.setWindowIcon(QIcon(":/res/chart_curve.png"));
	app.setStyle(new QPlastiqueStyle());

	QTabWidget window;
	window.setWindowTitle(QString("Dynamic Audio Normalizer - Log Viewer [%1]").arg(QString().sprintf("v%u.%02u-%u", versionMajor, versionMinor, versionPatch)));
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
		QMessageBox::critical(&window, QString().sprintf("Dynamic Audio Normalizer"), QString("Error: The selected log file could not opened for reading!"));
		return EXIT_FAILURE;
	}

	QTextStream textStream(&logFile);
	textStream.setCodec("UTF-8");

	double minValue = DBL_MAX;
	double maxValue = 0.0;
	unsigned int channels = 0;

	//parse header
	if(!parseHeader(textStream, channels))
	{
		QMessageBox::critical(&window, QString("Dynamic Audio Normalizer"), QString("Error: Failed to parse the header of the log file!\nProbably the file is of an unsupported type."));
		return EXIT_FAILURE;
	}

	//allocate buffers
	LogFileData *data = new LogFileData[channels];
	QCustomPlot *plot = new QCustomPlot[channels];

	//load data
	parseData(textStream, channels, data, minValue, maxValue);
	logFile.close();

	if((data[0].original.size() < 1) || (data[0].minimal.size() < 1) || (data[0].smoothed.size() < 1))
	{
		QMessageBox::critical(&window, QString("Dynamic Audio Normalizer"), QString("Error: Failed to load data. Log file countains no valid data!"));
		delete [] data;
		delete [] plot;
		return EXIT_FAILURE;
	}

	unsigned int length = UINT_MAX;
	for(unsigned int c = 0; c < channels; c++)
	{
		length = qMin(length, (unsigned int) data[c].original.count());
		length = qMin(length, (unsigned int) data[c].minimal .count());
		length = qMin(length, (unsigned int) data[c].smoothed.count());
	}

	QVector<double> steps(length);
	for(unsigned int i = 0; i < length; i++)
	{
		steps[i] = double(i);
	}

	for(unsigned int c = 0; c < channels; c++)
	{
		//create graph and assign data to it:
		createGraph(&plot[c], steps, data[c].original, Qt::blue,  Qt::SolidLine, 1);
		createGraph(&plot[c], steps, data[c].minimal,  Qt::green, Qt::SolidLine, 1);
		createGraph(&plot[c], steps, data[c].smoothed, Qt::red,   Qt::DashLine,  2);

		//give the axes some labels:
		plot[c].xAxis->setLabel("Frame #");
		plot[c].yAxis->setLabel("Gain Factor");
		makeAxisBold(plot[c].xAxis);
		makeAxisBold(plot[c].yAxis);

		//set axes ranges, so we see all data:
		plot[c].xAxis->setRange(0.0, length);
		plot[c].yAxis->setRange(minValue - 0.25, maxValue + 0.25);
		plot[c].yAxis->setScaleType(QCPAxis::stLinear);
		plot[c].yAxis->setTickStep(1.0);
		plot[c].yAxis->setAutoTickStep(false);

		//add title
		plot[c].plotLayout()->insertRow(0);
		plot[c].plotLayout()->addElement(0, 0, new QCPPlotTitle(&plot[c], QString("%1 (Channel %2/%3)").arg(QFileInfo(logFile).fileName(), QString::number(c+1), QString::number(channels))));

		//show the plot
		plot[c].replot();
		window.addTab(&plot[c], QString().sprintf("Channel #%u", c + 1));
	}

	//run application
	window.showMaximized();
	app.exec();

	//clean up
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
