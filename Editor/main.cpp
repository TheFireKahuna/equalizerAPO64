/*
	This file is part of EqualizerAPO, a system-wide equalizer.
	Copyright (C) 2015  Jonas Thedering

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

#include <QTranslator>
#include <QApplication>
#include <QDir>
#include <QCommandLineParser>
#include <QSettings>
#include <QStyleHints>
#include "CustomStyle.h"
#include "MainWindow.h"
#include "helpers/RegistryHelper.h"
#include "Editor/helpers/GUIHelper.h"

using namespace std;

int main(int argc, char* argv[])
{
	int result = -1;
#ifdef _DEBUG
	// _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	// _CrtSetBreakAlloc(3318);
#endif

	QCoreApplication::addLibraryPath("qt");
	qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");

	bool restart;
	do
	{
		QApplication application(argc, argv);
		if(GUIHelper::isDarkMode())
			application.setStyle("fusion");
		application.setStyle(new CustomStyle(application.style()));

		QSettings settings(QString::fromWCharArray(EDITOR_REGPATH), QSettings::NativeFormat);

		QVariant languageValue = settings.value("language");
		if (languageValue.isValid())
			QLocale::setDefault(QLocale(languageValue.toString()));
		else
			QLocale::setDefault(QLocale::system());

		QTranslator qtTranslator;
		if (qtTranslator.load(QLocale(), ":/translations/qtbase", "_"))
			application.installTranslator(&qtTranslator);

		QTranslator editorTranslator;
		if (editorTranslator.load(QLocale(), ":/translations/Editor", "_"))
			application.installTranslator(&editorTranslator);

		QString configPath = QDir::currentPath();
		if (RegistryHelper::keyExists(APP_REGPATH) && RegistryHelper::valueExists(APP_REGPATH, L"ConfigPath"))
			configPath = QString::fromStdWString(RegistryHelper::readValue(APP_REGPATH, L"ConfigPath"));
		QDir configDir(configPath);

		if (!RegistryHelper::keyExists(USER_REGPATH))
			RegistryHelper::createKey(USER_REGPATH);

		if (!RegistryHelper::keyExists(EDITOR_REGPATH))
			RegistryHelper::createKey(EDITOR_REGPATH);

		if (!RegistryHelper::keyExists(EDITOR_PER_FILE_REGPATH))
			RegistryHelper::createKey(EDITOR_PER_FILE_REGPATH);

		MainWindow w(configDir);
		w.show();

		QCommandLineParser parser;
		parser.process(application);
		QStringList args = parser.positionalArguments();
		if (args.isEmpty() && w.isEmpty())
			args = QStringList("config.txt");

		for (const QString& arg : args)
			w.load(configDir.absoluteFilePath(arg));

		w.doChecks();

		result = application.exec();

		restart = w.shouldRestart();
	}
	while (restart);

	return result;
}
