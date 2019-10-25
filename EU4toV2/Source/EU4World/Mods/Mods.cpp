/*Copyright (c) 2019 The Paradox Game Converters Project

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.*/



#include "Mods.h"
#include "Mod.h"
#include "../../Configuration.h"
#include "Log.h"
#include "OSCompatibilityLayer.h"
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>



EU4::Mods::Mods(std::istream& theStream, Configuration& theConfiguration)
{
	std::set<std::string> usedMods;
	registerKeyword(std::regex("\".+\""), [&usedMods](const std::string& modName, std::istream& theStream) {
		if (modName.substr(0, 1) == "\"")
		{
			usedMods.insert(modName.substr(1, modName.size() - 2));
		}
		else
		{
			usedMods.insert(modName);
		}
	});

	parseStream(theStream);

	loadEU4ModDirectory(theConfiguration);
	loadSteamWorkshopDirectory(theConfiguration);
	loadCK2ExportDirectory(theConfiguration);

	Log(LogLevel::Debug) << "Finding Used Mods";
	for (auto usedMod: usedMods)
	{
		auto possibleModPath = getModPath(usedMod);
		if (possibleModPath)
		{
			if (!Utils::doesFolderExist(*possibleModPath) && !Utils::DoesFileExist(*possibleModPath))
			{
				LOG(LogLevel::Error) << usedMod << " could not be found in the specified mod directory " \
					"- a valid mod directory must be specified. Tried " << *possibleModPath;
				exit(-1);
			}
			else
			{
				LOG(LogLevel::Debug) << "EU4 Mod is at " << *possibleModPath;
				theConfiguration.addEU4Mod(*possibleModPath);
			}
		}
		else
		{
			LOG(LogLevel::Error) << "No path could be found for " << usedMod << ". Check that the mod is present and that the .mod file specifies the path for the mod";
			exit(-1);
		}
	}
}


void EU4::Mods::loadEU4ModDirectory(const Configuration& theConfiguration)
{
	std::string EU4DocumentsLoc = theConfiguration.getEU4DocumentsPath();
	if (!Utils::doesFolderExist(EU4DocumentsLoc))
	{
		std::invalid_argument e(
			"No Europa Universalis 4 documents directory was specified in configuration.txt, or the path was invalid"
		);
		throw e;
	}
	else
	{
		LOG(LogLevel::Debug) << "EU4 Documents directory is " << EU4DocumentsLoc;
		if (theConfiguration.getEU4Version() > EU4::Version("1.29.0.0"))
		{
			loadModDirectory(EU4DocumentsLoc, "");
		}
		else
		{
			loadModDirectory(EU4DocumentsLoc, EU4DocumentsLoc);
		}
	}
}


void EU4::Mods::loadSteamWorkshopDirectory(const Configuration& theConfiguration)
{
	std::string steamWorkshopPath = theConfiguration.getSteamWorkshopPath();
	if (!Utils::doesFolderExist(steamWorkshopPath))
	{
		std::invalid_argument e(
			"No Steam Worksop directory was specified in configuration.txt, or the path was invalid"
		);
		throw e;
	}
	else
	{
		LOG(LogLevel::Debug) << "Steam Workshop directory is " << steamWorkshopPath;
		std::set<std::string> subfolders;
		Utils::GetAllSubfolders(steamWorkshopPath + "/mod", subfolders);
		for (auto subfolder: subfolders)
		{
			std::string descriptorFilename = subfolder + "/descriptor.mod";
			if (Utils::doesFolderExist(subfolder) && Utils::DoesFileExist(descriptorFilename))
			{
				std::ifstream modFile(subfolder);
				Mod theMod(modFile);
				modFile.close();

				if (theMod.isValid())
				{
					possibleMods.insert(std::make_pair(theMod.getName(), subfolder));
					Log(LogLevel::Debug) << "\tFound a mod named " << theMod.getName() << " at " << subfolder;
				}
			}
		}
	}
}


void EU4::Mods::loadCK2ExportDirectory(const Configuration& theConfiguration)
{
	std::string CK2ExportLoc = theConfiguration.getCK2ExportPath();
	if (!Utils::doesFolderExist(CK2ExportLoc))
	{
		LOG(LogLevel::Warning) << "No Crusader Kings 2 mod directory was specified in configuration.txt," \
			" or the path was invalid - this will cause problems with CK2 converted saves";
	}
	else
	{
		LOG(LogLevel::Debug) << "CK2 export directory is " << CK2ExportLoc;
		if (theConfiguration.getEU4Version() > EU4::Version("1.29.0.0"))
		{
			loadModDirectory(CK2ExportLoc, "");
		}
		else
		{
			loadModDirectory(CK2ExportLoc, CK2ExportLoc);
		}
	}
}


void EU4::Mods::loadModDirectory(const std::string& searchDirectory, const std::string& recordDirectory)
{
	std::set<std::string> filenames;
	Utils::GetAllFilesInFolder(searchDirectory + "/mod", filenames);
	for (auto filename: filenames)
	{
		const int pos = filename.find_last_of('.');
		if ((pos != std::string::npos) && (filename.substr(pos, filename.length()) == ".mod"))
		{
			try
			{
				std::ifstream modFile(searchDirectory + "/mod/" + filename);
				Mod theMod(modFile);
				modFile.close();

				if (theMod.isValid())
				{
					std::string trimmedFilename = filename.substr(0, pos);

					possibleMods.insert(std::make_pair(theMod.getName(), recordDirectory + "/" + theMod.getPath()));
					possibleMods.insert(std::make_pair("mod/" + filename, recordDirectory + "/" + theMod.getPath()));
					possibleMods.insert(std::make_pair(trimmedFilename, recordDirectory + "/" + theMod.getPath()));
					Log(LogLevel::Debug) << "\tFound a mod named " << theMod.getName() <<
						" with a mod file at " << searchDirectory << "/mod/" + filename <<
						" claiming to be at " << recordDirectory << "/" << theMod.getPath();
				}
			}
			catch (std::exception e)
			{
				LOG(LogLevel::Warning) << "Error while reading " << searchDirectory << " / mod / " << filename << ". " \
					"Mod will not be useable for conversions.";
			}
		}
	}
}


std::optional<std::string> EU4::Mods::getModPath(const std::string& modName) const
{
	auto mod = possibleMods.find(modName);
	if (mod == possibleMods.end())
	{
		return {};
	}
	else
	{
		return mod->second;
	}
}