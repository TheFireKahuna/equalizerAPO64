/*
    This file is part of Equalizer APO, a system-wide equalizer.
    Copyright (C) 2017  Jonas Thedering

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "filters/loudnessCorrection/LoudnessCorrectionFilter.h"
#include "LoudnessCorrectionFilterGUI.h"
#include "LoudnessCorrectionFilterGUIFactory.h"

LoudnessCorrectionFilterGUIFactory::LoudnessCorrectionFilterGUIFactory()
{
}

LoudnessCorrectionFilterGUIFactory::~LoudnessCorrectionFilterGUIFactory()
{
	if (timer != NULL)
		timer->stop();

	if (volumeController != NULL)
	{
		delete volumeController;
		volumeController = NULL;
	}
}

void LoudnessCorrectionFilterGUIFactory::initialize(FilterTable* filterTable)
{
	this->filterTable = filterTable;
}

QList<FilterTemplate> LoudnessCorrectionFilterGUIFactory::createFilterTemplates()
{
	QList<FilterTemplate> list;
	list.append(FilterTemplate(tr("Loudness correction"), "LoudnessCorrection: State 1 ReferenceLevel 0 ReferenceOffset 0 Attenuation 1.0", QStringList(tr("Advanced filters"))));
	return list;
}

IFilterGUI* LoudnessCorrectionFilterGUIFactory::createFilterGUI(QString& command, QString& parameters)
{
	LoudnessCorrectionFilterGUI* result = NULL;

	if (command == "LoudnessCorrection")
	{
		LoudnessCorrectionFilter::FilterParameters params;
		if (!params.deSerialize(parameters.toStdWString()))
		{
			result = new LoudnessCorrectionFilterGUI(params.referenceLevel, params.referenceOffset, params.attenuation);

			if (timer == NULL)
			{
				timer = new QTimer(this);
				connect(timer, SIGNAL(timeout()), this, SLOT(checkVolume()));
				timer->start(10);
			}
		}
	}

	return result;
}

void LoudnessCorrectionFilterGUIFactory::checkVolume()
{
	if (volumeController == NULL)
	{
		volumeController = new VolumeController();
		double lVolD;
		volumeController->getVolume(lVolD);
		lastVolume = lVolD;
	}
	else
	{
		double volume;
		HRESULT res = volumeController->getVolume(volume);
		float volume2 = (float)volume;

		if (SUCCEEDED(res) && volume2 != lastVolume)
		{
			filterTable->updateAnalysis();
			lastVolume = volume2;
		}
	}
}
