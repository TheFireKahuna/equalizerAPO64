#-------------------------------------------------
#
# DeviceSelector qmake project for CI builds
# This allows building DeviceSelector with qmake in CI
# while keeping the .vcxproj for local Visual Studio development
#
#-------------------------------------------------

QT += core gui widgets

TARGET = DeviceSelector
TEMPLATE = app

DEFINES += WIN32
DEFINES += _UNICODE
DEFINES += MUP_USE_WIDE_STRING
QMAKE_CXXFLAGS_RELEASE += /O2

PRECOMPILED_HEADER = stdafx.h

SOURCES += \
	main.cpp \
	DeviceSelector.cpp \
	DeviceTestDialog.cpp \
	DeviceTestThread.cpp \
	OpacityIconEngine.cpp \
	ReceiveThread.cpp \
	../helpers/ServiceHelper.cpp \
	stdafx.cpp

HEADERS += \
	DeviceSelector.h \
	DeviceTestDialog.h \
	DeviceTestThread.h \
	OpacityIconEngine.h \
	ReceiveThread.h \
	../helpers/ServiceHelper.h \
	resource.h \
	stdafx.h

FORMS += \
	DeviceSelector.ui \
	DeviceTestDialog.ui

RESOURCES += \
	DeviceSelector.qrc

TRANSLATIONS += \
	translations/DeviceSelector_de.ts \
	translations/DeviceSelector_en.ts \
	translations/DeviceSelector_fr.ts \
	translations/DeviceSelector_zh_CN.ts

# Include parent directory for shared headers
INCLUDEPATH += $$PWD/..

# Link against Common library and Windows libraries
LIBS += Kernel32.lib version.lib Shlwapi.lib authz.lib user32.lib advapi32.lib crypt32.lib

# Include Common.lib
LIBS += Common.lib
contains(QT_ARCH, arm64) {
	build_pass:CONFIG(debug, debug|release) {
		QMAKE_LIBDIR += "../ARM64/Debug"

	} else {
		QMAKE_LIBDIR += "../ARM64/Release"
	}
} else:contains(QT_ARCH, x86_64) {
	QMAKE_CXXFLAGS += /arch:AVX2
	build_pass:CONFIG(debug, debug|release) {
		QMAKE_LIBDIR += "../x64/Debug"

	} else {
		QMAKE_LIBDIR += "../x64/Release"
	}
} else {
	QMAKE_CXXFLAGS += /arch:AVX2
	build_pass:CONFIG(debug, debug|release) {
		QMAKE_LIBDIR += "../x32/Debug"

	} else {
		QMAKE_LIBDIR += "../x32/Release"
	}
}

# UAC: Require Administrator
QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"

RC_FILE = DeviceSelector.rc
