//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Utility
// Copyright (c) 2014-2017 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// http://www.gnu.org/licenses/gpl-3.0.txt
//////////////////////////////////////////////////////////////////////////////////

//Std
#include <cstdlib>
#include <cfloat>

//Qt
#include <QApplication>
#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QTabWidget>
#include <QLabel>
#include <QStatusBar>
#include <QPlastiqueStyle>
#include <QRegExp>

//MDynamicAudioNormalizer API
#include "DynamicAudioNormalizer.h"

//Internal
#include "3rd_party/qcustomplot.h"

//Const
static const int PRE_ALLOC_SIZE = 12000;
static const QLatin1String g_title("Dynamic Audio Normalizer GUI");

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

static QString readNextLine(QTextStream &textStream)
{
	while ((!textStream.atEnd()) && (textStream.status() == QTextStream::Ok))
	{
		const QString line = textStream.readLine().simplified();
		if (!line.isEmpty())
		{
			return line;
		}
	}
	return QString();
}

static bool parseHeader(QWidget &parent, QTextStream &textStream, quint32 &channels, const uint32_t versionMajor, const uint32_t versionMinor)
{
	static const QLatin1String REGEX_HEADER("^\\s*DynamicAudioNormalizer\\s+Logfile\\s+v(\\d).(\\d\\d)-(\\d)\\s*$");
	static const QLatin1String REGEX_CHNNLS("^\\s*CHANNEL_COUNT\\s*:\\s*(\\d+)\\s*$");

	QRegExp header(REGEX_HEADER, Qt::CaseInsensitive);
	QRegExp channelCount(REGEX_CHNNLS, Qt::CaseInsensitive);

	const QString headerLine = readNextLine(textStream);
	if(headerLine.isEmpty() || (header.indexIn(headerLine) < 0))
	{
		QMessageBox::warning(&parent, g_title, QLatin1String("Invalid file: Header line could not be found!"));
		return false;
	}

	bool okay[2] = { false, false };
	uint32_t fileVersion[2];
	fileVersion[0] = header.cap(1).toUInt(&okay[0]);
	fileVersion[1] = header.cap(2).toUInt(&okay[1]);
	if ((!(okay[0] && okay[1])) || (fileVersion[0] != versionMajor) || (fileVersion[1] > versionMinor))
	{
		QMessageBox::warning(&parent, g_title, QLatin1String("Invalid file: Unsupported file format version!"));
		return false;
	}

	const QString channelLine = readNextLine(textStream);
	if(channelLine.isEmpty() || (channelCount.indexIn(channelLine) < 0))
	{
		QMessageBox::warning(&parent, g_title, QLatin1String("Invalid file: Number of channels could not be found!"));
		return false;
	}

	bool haveChannels = false;
	channels = channelCount.cap(1).toUInt(&haveChannels);
	if ((!haveChannels) || (channels < 1))
	{
		QMessageBox::warning(&parent, g_title, QLatin1String("Invalid file: Number of channels is invalid!"));
		return false;
	}

	return true; /*success*/
}

static bool parseLine(const quint32 channels, const QStringList &input, QVector<double> &output)
{
	const int minLength = qMin(input.count(), output.count());
	QVector<double>::Iterator outputIter = output.begin();
	QStringList::ConstIterator inputIter = input.constBegin();
	for (int i = 0; i < minLength; ++i)
	{
		bool okay = false;
		*(outputIter++) = (inputIter++)->toDouble(&okay);
		if (!okay)
		{
			return false;
		}
	}
	return true; /*success*/
}

static void parseData(QTextStream &textStream, const quint32 channels, LogFileData *data, double &minValue, double &maxValue)
{
	const QRegExp spaces(QLatin1String("\\s+"));
	QVector<double> temp(channels * 3U);
	forever
	{
		const QString line = readNextLine(textStream);
		if(!line.isEmpty())
		{
			const QStringList tokens = line.split(spaces, QString::SkipEmptyParts);
			if ((quint32)tokens.count() >= (channels * 3U))
			{
				if (parseLine(channels, tokens, temp))
				{
					QVector<double>::ConstIterator tempIter = temp.constBegin();
					for (quint32 c = 0; c < channels; c++)
					{
						data[c].original.append(*(tempIter++));
						data[c].minimal .append(*(tempIter++));
						data[c].smoothed.append(*(tempIter++));
						minValue = qMin(qMin(minValue, data[c].original.back()), qMin(data[c].minimal.back(), data[c].smoothed.back()));
						maxValue = qMax(qMax(maxValue, data[c].original.back()), qMax(data[c].minimal.back(), data[c].smoothed.back()));
					}
				}
			}
			continue;
		}
		break; /*end of file*/
	}
}

static void createGraph(QCustomPlot *const plot, QVector<double> &steps, QVector<double> &data, const Qt::GlobalColor &color, const Qt::PenStyle &style, const int width)
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

	//initialize application
	QApplication app(argc, argv);
	app.setWindowIcon(QIcon(QLatin1String(":/res/chart_curve.png")));
	app.setStyle(new QPlastiqueStyle());
	app.setStyleSheet(QLatin1String("QStatusBar::item{border:none}"));

	//create basic tab widget
	QMainWindow window;
	window.setWindowTitle(QString::fromLatin1("Dynamic Audio Normalizer - Log Viewer [%1]").arg(QString().sprintf("v%u.%02u-%u", versionMajor, versionMinor, versionPatch)));
	window.setMinimumSize(640, 480);
	window.setContentsMargins(5, 5, 5, 5);
	
	//create statusbar
	QStatusBar statusbar;
	QLabel info(QLatin1String("Copyright (c) 2014-2017 LoRd_MuldeR <mulder2@gmx.de> | License: GNU General Public License v3"));
	statusbar.addPermanentWidget(&info);
	window.setStatusBar(&statusbar);

	//Show the window
	window.show();

	//get arguments
	const QStringList args = app.arguments();
	
	//choose input file
	const QString logFileName = (args.size() < 2) ? QFileDialog::getOpenFileName(&window, QLatin1String("Open Log File"), QString(), QLatin1String("Log File (*.log)")) : args.at(1);
	if(logFileName.isEmpty())
	{
		return EXIT_FAILURE;
	}

	//check for existence
	if(!(QFileInfo(logFileName).exists() && QFileInfo(logFileName).isFile()))
	{
		QMessageBox::critical(&window, g_title, QLatin1String("Error: The specified log file could not be found!"));
		return EXIT_FAILURE;
	}

	//open the input file
	QFile logFile(logFileName);
	if(!logFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::critical(&window, g_title, QLatin1String("Error: The selected log file could not opened for reading!"));
		return EXIT_FAILURE;
	}

	//wrap into text stream
	QTextStream textStream(&logFile);
	textStream.setCodec("UTF-8");

	quint32 channels = 0;
	double minValue = DBL_MAX;
	double maxValue = 0.0;

	//parse header
	if(!parseHeader(window, textStream, channels, versionMajor, versionMinor))
	{
		QMessageBox::critical(&window, g_title, QLatin1String("Error: Failed to parse the header of the selected log file!\nProbably this file is corrupted or an unsupported type."));
		return EXIT_FAILURE;
	}

	//allocate buffers
	QVector<LogFileData> data(channels);
	for (quint32 c = 0; c < channels; c++)
	{
		data[c].original.reserve(PRE_ALLOC_SIZE);
		data[c].minimal .reserve(PRE_ALLOC_SIZE);
		data[c].smoothed.reserve(PRE_ALLOC_SIZE);
	}

	//load data
	parseData(textStream, channels, data.data(), minValue, maxValue);
	logFile.close();

	//check data
	if((data[0].original.size() < 1) || (data[0].minimal.size() < 1) || (data[0].smoothed.size() < 1))
	{
		QMessageBox::critical(&window, g_title, QLatin1String("Error: Failed to load data from log file.\nFile countains no valid data points!"));
		return EXIT_FAILURE;
	}

	//determine length of data
	quint32 length = UINT_MAX;
	for(quint32 c = 0; c < channels; c++)
	{
		length = qMin(length, (quint32)data[c].original.count());
		length = qMin(length, (quint32)data[c].minimal .count());
		length = qMin(length, (quint32)data[c].smoothed.count());
	}

	//create x-axis steps
	QVector<double> steps(length);
	for(quint32 i = 0; i < length; i++)
	{
		steps[i] = double(i);
	}

	//Create tab widget
	QTabWidget tabWidget(&window);
	window.setCentralWidget(&tabWidget);

	//now create the plots
	QVector<QCustomPlot*> plot(channels, NULL);
	for(quint32 c = 0; c < channels; c++)
	{
		//create new instance
		plot[c] = new QCustomPlot(&window);

		//init the legend
		plot[c]->legend->setVisible(true);
		plot[c]->legend->setFont(QFont(QLatin1String("Helvetica"), 9));

		//create graph and assign data to it:
		createGraph(plot[c], steps, data[c].original, Qt::blue,  Qt::SolidLine, 1);
		createGraph(plot[c], steps, data[c].minimal,  Qt::green, Qt::SolidLine, 1);
		createGraph(plot[c], steps, data[c].smoothed, Qt::red,   Qt::DashLine,  2);

		//set plot names
		plot[c]->graph(0)->setName(QLatin1String("Local Max. Gain"));
		plot[c]->graph(1)->setName(QLatin1String("Minimum Filtered"));
		plot[c]->graph(2)->setName(QLatin1String("Final Smoothed"));

		//set axes labels:
		plot[c]->xAxis->setLabel(QLatin1String("Frame #"));
		plot[c]->yAxis->setLabel(QLatin1String("Gain Factor"));
		makeAxisBold(plot[c]->xAxis);
		makeAxisBold(plot[c]->yAxis);

		//set axes ranges
		plot[c]->xAxis->setRange(0.0, length);
		plot[c]->yAxis->setRange(minValue - 0.25, maxValue + 0.25);
		plot[c]->yAxis->setScaleType(QCPAxis::stLinear);
		plot[c]->yAxis->setTickStep(1.0);
		plot[c]->yAxis->setAutoTickStep(false);

		//add title
		plot[c]->plotLayout()->insertRow(0);
		plot[c]->plotLayout()->addElement(0, 0, new QCPPlotTitle(plot[c], QString::fromLatin1("%1 (Channel %2/%3)").arg(QFileInfo(logFile).fileName(), QString::number(c+1), QString::number(channels))));

		//show the plot
		plot[c]->replot();
		tabWidget.addTab(plot[c], QString().sprintf("Channel #%u", c + 1));
	}

	//run application
	window.showMaximized();
	app.exec();

	//clean-up
	for (QVector<QCustomPlot*>::Iterator iter = plot.begin(); iter != plot.end(); ++iter)
	{
		if (*iter)
		{
			delete *iter;
			*iter = NULL;
		}
	}

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
