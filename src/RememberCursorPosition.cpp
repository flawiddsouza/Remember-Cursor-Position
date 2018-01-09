//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <locale>
#include <codecvt>

extern FuncItem funcItem[nbFunc];
extern NppData nppData;

#ifndef UNICODE
typedef std::string String;
#else
typedef std::wstring String;
#endif

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  reasonForCall, LPVOID /*lpReserved*/)
{
    switch (reasonForCall)
    {
      case DLL_PROCESS_ATTACH:
        pluginInit(hModule);
        break;

      case DLL_PROCESS_DETACH:
        pluginCleanUp();
        break;

      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}


extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;
	commandMenuInit();
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
	*nbF = nbFunc;
	return funcItem;
}

void deletePathFromHistoryFileIfPreExisting(String historyFilePath, String tempHistoryFilePath, String currentFilePath)
{
	std::string line;
	std::string lineToDelete;
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

	// find the line that needs to be deleted
	std::ifstream fileToRead(historyFilePath);
	while (std::getline(fileToRead, line))
	{
		std::istringstream iss(line);
		std::vector<std::string> result;
		while (iss.good())
		{
			std::string substr;
			std::getline(iss, substr, ',');
			result.push_back(substr);
		}
		// strip quotes from stored filePath
		if (result[0].front() == '"') {
			result[0].erase(0, 1); // erase the first character
			result[0].erase(result[0].size() - 1); // erase the last character
		}
		String filePathInLine = converter.from_bytes(result[0]);
		if (filePathInLine == currentFilePath) {
			lineToDelete = line;
			//::MessageBox(NULL, filePathInLine.c_str(), TEXT("Saved FilePath"), MB_OK);
		}
	}
	fileToRead.close();

	if (!lineToDelete.empty())
	{
		//::MessageBox(NULL, converter.from_bytes(lineToDelete).c_str(), TEXT("Line to delete"), MB_OK);

		std::ofstream tempHistoryFile(tempHistoryFilePath);

		// create a temp
		fileToRead.open(historyFilePath);
		while (std::getline(fileToRead, line))
		{
			if (line != lineToDelete)
			{
				tempHistoryFile << line << std::endl;
				//::MessageBox(NULL, converter.from_bytes(line).c_str(), TEXT("Line not to delete"), MB_OK);
			}
		}

		tempHistoryFile.close();

		CopyFile(tempHistoryFilePath.c_str(), historyFilePath.c_str(), false);

		DeleteFile(tempHistoryFilePath.c_str());
	}
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	switch (notifyCode->nmhdr.code) 
	{
		case NPPN_FILEOPENED:
		{
			TCHAR filePath[MAX_PATH];
			::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)filePath);

			TCHAR configPath[MAX_PATH];
			::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configPath);

			String historyFilePath = configPath;
			historyFilePath += L"\\cursorHistory.txt";

			std::ifstream fileToRead(historyFilePath);
			std::string line;
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			while (std::getline(fileToRead, line))
			{
				std::istringstream iss(line);
				std::vector<std::string> result;
				while (iss.good())
				{
					std::string substr;
					std::getline(iss, substr, ',');
					result.push_back(substr);
				}
				// strip quotes from stored filePath
				if (result[0].front() == '"') {
					result[0].erase(0, 1); // erase the first character
					result[0].erase(result[0].size() - 1); // erase the last character
				}
				String filePathInLine = converter.from_bytes(result[0]);
				if (filePathInLine == filePath) {
					::SendMessage(nppData._scintillaMainHandle, SCI_GOTOPOS, std::stoi(result[1]), 0); // Sets cursor position
				}
			}
		}
		break;

		case NPPN_FILEBEFORECLOSE:
		{
			TCHAR filePath[MAX_PATH];
			::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, MAX_PATH, (LPARAM)filePath);

			int cursorPosition = ::SendMessage(nppData._scintillaMainHandle, SCI_GETCURRENTPOS, 0, 0); // Gets cursor position

			TCHAR configPath[MAX_PATH];
			::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configPath);

			String historyFilePath = configPath;
			historyFilePath += L"\\cursorHistory.txt";

			String tempHistoryFilePath = configPath;
			tempHistoryFilePath += L"\\tempcursorHistory.txt";

			deletePathFromHistoryFileIfPreExisting(historyFilePath, tempHistoryFilePath, filePath);

			std::wofstream historyFile;
			historyFile.open(historyFilePath, std::ios_base::app); // the second parameter == append to file instead of replacing it
			historyFile << '"' << filePath << '"' << "," << cursorPosition << "\n";
			historyFile.close();
		}
		break;

		case NPPN_SHUTDOWN:
		{
			commandMenuCleanUp();
		}
		break;

		default:
			return;
	}
}

// Here you can process the Npp Messages 
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// http://sourceforge.net/forum/forum.php?forum_id=482781
//
extern "C" __declspec(dllexport) LRESULT messageProc(UINT /*Message*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{/*
	if (Message == WM_MOVE)
	{
		::MessageBox(NULL, "move", "", MB_OK);
	}
*/
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif //UNICODE
