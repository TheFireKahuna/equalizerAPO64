/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2014  Jonas Thedering

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

#include "stdafx.h"
#include <mpParser.h>
#include "helpers/LogHelper.h"
#include "helpers/StringHelper.h"
#include "FilterEngine.h"
#include "StageFilterFactory.h"

using namespace std;

void StageFilterFactory::initialize(FilterEngine* engine)
{
	preMix = engine->isPreMix();
	capture = engine->isCapture();
	postMixInstalled = engine->isPostMixInstalled();

	engine->getParser()->DefineConst(L"stage", engine->isCapture() ? L"capture" : engine->isPreMix() ? L"pre-mix" : L"post-mix");
}

vector<IFilter*> StageFilterFactory::startOfConfiguration()
{
	stageMatches = capture || !preMix || !postMixInstalled;
	while (!stageMatchesStack.empty())
		stageMatchesStack.pop();

	return vector<IFilter*>();
}

std::vector<IFilter*> StageFilterFactory::startOfFile(const std::wstring& configPath)
{
	stageMatchesStack.push(stageMatches);

	return vector<IFilter*>();
}

vector<IFilter*> StageFilterFactory::createFilter(const wstring& configPath, wstring& command, wstring& parameters)
{
	// User-defined Stage commands in config.txt are disabled; only automatic PreMix/PostMix stage selection is active.
	if (!stageMatches)
		// skip line for further factories
		command = L"";

	return vector<IFilter*>();
}

std::vector<IFilter*> StageFilterFactory::endOfFile(const std::wstring& configPath)
{
	stageMatches = stageMatchesStack.top();
	stageMatchesStack.pop();

	return vector<IFilter*>();
}
