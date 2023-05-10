/*
 * Copyright 2020 GoPro Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <stdafx.h>
#include "FileUtil.h"
#include <backends/common/File.h>

#include <chrono>

namespace ngli
{
namespace fs = std::filesystem;
using namespace std::chrono;



bool FileUtil::getmtime(const std::string& filename, fs::file_time_type& mtime)
{
	if(!fs::exists(filename))
	{
		return false;
	}
	mtime = fs::last_write_time(filename);
	return true;
}

bool FileUtil::srcFileNewerThanOutFile(const std::string& srcFileName,
									   const std::string& targetFileName)
{
	fs::file_time_type srcTimeStamp, targetTimeStamp;
	getmtime(srcFileName, srcTimeStamp);
	if(!getmtime(targetFileName, targetTimeStamp))
		return true;
	if(srcTimeStamp > targetTimeStamp)
		return true;
	return false;
}

std::string FileUtil::tempDir()
{
	return fs::canonical(fs::temp_directory_path()).string();
}

#define RETRY_WITH_TIMEOUT(fn, t) \
const uint32_t timeoutMs = t; \
auto t0 = system_clock::now(); \
std::string err = ""; \
while (true) { \
    try { \
        fn; \
        break; \
    } \
    catch (std::exception e) { \
        err = e.what(); \
        std::this_thread::sleep_for(milliseconds(10)); \
    } \
    auto t1 = system_clock::now(); \
    if (duration_cast<milliseconds>(t1 - t0).count() > timeoutMs) { \
        NGLI_ERR("%s: %s timeoutMS: %d", err.c_str(), path.string().c_str(), timeoutMs); \
    } \
}

bool FileUtil::exists(const fs::path& path)
{
	bool r;
	RETRY_WITH_TIMEOUT(r = fs::exists(path), 3000);
	return r;
}

void FileUtil::remove(const fs::path& path)
{
	RETRY_WITH_TIMEOUT(fs::remove(path), 3000);
}


bool FileUtil::open(std::ifstream& in, const std::string& filename)
{
	std::filesystem::path tFilename = FileUtil::getAbsolutePath(filename);
	for(size_t iTry = 3; iTry--;)
	{
		in.open(tFilename);
		if(in.is_open())
		{
			return true;
		}
		LOG(ERROR, "cannot open file: %s", filename.c_str());
		Sleep(500); // Wait before to retry
	}
	return false;
}

std::filesystem::path FileUtil::getAbsolutePath(const std::string& path)
{
	std::filesystem::path tFilename = path;
	if(tFilename.is_relative())
	{
		wchar_t szPath[MAX_PATH];
		GetModuleFileNameW(0, szPath, MAX_PATH);
		std::filesystem::path tExe(szPath);
		tExe = tExe.remove_filename();
		tFilename = std::filesystem::absolute(tExe / tFilename);
	}
	return tFilename;
}

FileUtil::Lock::Lock(const std::string& path, uint32_t timeoutMs)
	: lockPath(path + ".lock"), timeoutMs(timeoutMs)
{
	fs::path fpath(path);
	auto t0 = system_clock::now();
	while(FileUtil::exists(lockPath))
	{
		std::this_thread::sleep_for(milliseconds(10));
		auto t1 = system_clock::now();
		if(duration_cast<milliseconds>(t1 - t0).count() > timeoutMs)
		{
			NGLI_ERR("file locked: %s, timeoutMs: %d", path.c_str(), timeoutMs);
		}
	}
	writeFile(lockPath, "");
}
FileUtil::Lock::~Lock()
{
	FileUtil::remove(lockPath);
}

std::string FileUtil::readFile(const std::string& path)
{
	File file;
	file.read(path);
	return std::string(file.data.get(), file.size);
}

bool FileUtil::writeFile(const std::string& path, const std::string& contents)
{
	for(size_t iTry = 3; iTry--;)
	{
		std::ofstream out(path, std::ofstream::binary, _SH_DENYRW);
		if(out.is_open())
		{
			out.write(contents.data(), contents.size());
			out.close();
			return true;
		}
		Sleep(500); // Wait before to retry
	}
	return false;
}

std::vector<std::string> FileUtil::splitExt(const std::string& filename)
{
	auto it = filename.find_last_of('.');
	return { filename.substr(0, it), filename.substr(it) };
}

std::vector<std::string> FileUtil::findFiles(const std::string& path)
{
	std::vector<std::string> files;
	for(auto& entry : fs::directory_iterator(path))
	{
		fs::path filePath = entry.path();
		files.push_back(filePath.make_preferred().string());
	}
	return files;
}

std::vector<std::string> FileUtil::findFiles(const std::string& path, const std::string& ext)
{
	std::vector<std::string> files;
	for(auto& entry : fs::directory_iterator(path))
	{
		fs::path filePath = entry.path();
		if(filePath.extension() != ext)
			continue;
		files.push_back(filePath.make_preferred().string());
	}
	return files;
}

std::vector<std::string> FileUtil::filterFiles(const std::vector<std::string>& files,
									 const std::string& fileFilter)
{
	std::vector<std::string> filteredFiles;
	for(const std::string& file : files)
	{
		if(strstr(file.c_str(), fileFilter.c_str()))
			filteredFiles.push_back(file);
	}
	return filteredFiles;
}

std::vector<std::string> FileUtil::findFiles(const std::vector<std::string>& paths,
								   const std::vector<std::string>& extensions)
{
	std::vector<std::string> files;
	for(const std::string& path : paths)
	{
		for(const std::string& ext : extensions)
		{
			std::vector<std::string> filteredFiles = findFiles(path, ext);
			files.insert(files.end(), filteredFiles.begin(), filteredFiles.end());
		}
	}
	return files;
}

void FileUtil::copyFiles(const std::vector<std::string>& files,
	const std::string& outDir)
{
	for(const std::string& file : files)
	{
		std::string filename = fs::path(file).filename().string();
		FileUtil::writeFile(fs::path(outDir + "/" + filename).string(), FileUtil::readFile(file));
	}
}
}
