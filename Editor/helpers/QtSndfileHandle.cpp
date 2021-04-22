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

#include "QtSndfileHandle.h"

sf_count_t getFileLenDevice(void* user_data)
{
	QIODevice* device = (QIODevice*)user_data;
	return device->size();
}

sf_count_t seekDevice(sf_count_t offset, int whence, void* user_data)
{
	QIODevice* device = (QIODevice*)user_data;
	qint64 pos;
	if (whence == SF_SEEK_SET)
		pos = 0;
	else if (whence == SF_SEEK_CUR)
		pos = device->pos();
	else // whence == SF_SEEK_END
		pos = device->size();
	if (!device->seek(pos + offset))
		return -1;
	return device->pos();
}

sf_count_t readDevice(void* ptr, sf_count_t count, void* user_data)
{
	QIODevice* device = (QIODevice*)user_data;
	return device->read((char*)ptr, count);
}

sf_count_t writeDevice(const void* ptr, sf_count_t count, void* user_data)
{
	QIODevice* device = (QIODevice*)user_data;
	return device->write((const char*)ptr, count);
}

sf_count_t tellDevice(void* user_data)
{
	QIODevice* device = (QIODevice*)user_data;
	return device->pos();
}

SF_VIRTUAL_IO g_deviceIo = {getFileLenDevice, seekDevice, readDevice, writeDevice, tellDevice};

QtSndfileHandle::QtSndfileHandle(QIODevice& device, int mode, int format, int channels, int samplerate)
	: SndfileHandle(g_deviceIo, &device, mode, format, channels, samplerate)
{
}
