#-------------------------------------------------
#
# UpdateChecker qmake project for CI builds
# This allows building UpdateChecker with qmake in CI
# while keeping the .vcxproj for local Visual Studio development
#
#-------------------------------------------------

QT += core gui widgets network

TARGET = UpdateChecker
TEMPLATE = app

DEFINES += WIN32
DEFINES += _UNICODE
DEFINES += MUP_USE_WIDE_STRING
QMAKE_CXXFLAGS += /arch:AVX2
QMAKE_CXXFLAGS_RELEASE += /O2

PRECOMPILED_HEADER = stdafx.h

SOURCES += \
	main.cpp \
	UpdateChecker.cpp \
	AutoSizeTextEdit.cpp \
	../helpers/StringHelper.cpp \
	../helpers/TaskSchedulerHelper.cpp \
	stdafx.cpp

HEADERS += \
	UpdateChecker.h \
	AutoSizeTextEdit.h \
	../helpers/StringHelper.h \
	../helpers/TaskSchedulerHelper.h \
	../helpers/RegistryHelper.h \
	resource.h \
	stdafx.h

FORMS += \
	UpdateChecker.ui

RESOURCES += \
	UpdateChecker.qrc

TRANSLATIONS += \
	translations/UpdateChecker_de.ts \
	translations/UpdateChecker_fr.ts \
	translations/UpdateChecker_zh_CN.ts

# Include parent directory for shared headers
INCLUDEPATH += $$PWD/..

# Link against Common library and Windows libraries
LIBS += -lSecur32 -ltaskschd Kernel32.lib version.lib oleaut32.lib Shlwapi.lib user32.lib advapi32.lib crypt32.lib uuid.lib authz.lib

# Include Common.lib
LIBS += Common.lib
contains(QT_ARCH, arm64) {
	build_pass:CONFIG(debug, debug|release) {
		QMAKE_LIBDIR += "../ARM64/Debug"

	} else {
		QMAKE_LIBDIR += "../ARM64/Release"
	}
} else:contains(QT_ARCH, x86_64) {
	build_pass:CONFIG(debug, debug|release) {
		QMAKE_LIBDIR += "../x64/Debug"

	} else {
		QMAKE_LIBDIR += "../x64/Release"
	}
} else {
	build_pass:CONFIG(debug, debug|release) {
		QMAKE_LIBDIR += "../x32/Debug"

	} else {
		QMAKE_LIBDIR += "../x32/Release"
	}
}

RC_FILE = UpdateChecker.rc
