#pragma once
#include <filesystem>
#include "SyncApi.h"
#include <iostream>
#include <fstream>
#include <ctime>

namespace folder_sync
{
	namespace fs = std::experimental::filesystem;
	struct path_compare
	{
		inline int operator() (const fs::path & path1, const fs::path& path2)
		{
			if (fs::is_directory(path1) && fs::is_directory(path2))
			{
				return path1.filename().compare(path2.filename());
				//todo ako vracia directory pathy
			}

			if (fs::is_directory(path1) && !fs::is_directory(path2))
			{
				return 1;
			}

			if (!fs::is_directory(path1) && fs::is_directory(path2))
			{
				return -1;
			}

			if (path1.filename().compare( path2.filename())!=0)
			{
				auto a = path1.filename();
				return path1.filename().compare(path2.filename());
			}
			if (fs::file_size(path1) != fs::file_size(path2))
			{
				return static_cast<int>(fs::file_size(path1) - fs::file_size(path2));
			}
			if (fs::last_write_time(path1) != fs::last_write_time(path2))
			{
				auto time = fs::last_write_time(path1);
				auto t1 = decltype(time)::clock::to_time_t(time);
				auto t2 = decltype(time)::clock::to_time_t(fs::last_write_time(path2));
				return static_cast<int>(t1 - t2);
			}
			return 0;
		}
	};

	inline void console_diff_solve(diff_token & dt,std::error_code & ec)
	{
		std::cout << "File in source:" << std::endl;
		std::cout << "File: " << dt.source_dest << std::endl;
		std::cout << "File size:" << fs::file_size(dt.source_dest) << std::endl;
		auto ftime = fs::last_write_time(dt.source_dest);
		std::time_t cftime = decltype(ftime)::clock::to_time_t(ftime);
		tm timeinfo;
		localtime_s(&timeinfo, &cftime);
		char buffer[256];
		asctime_s(buffer, &timeinfo);
		std::cout << "File modified:" << buffer << std::endl;
		std::cout << "File in target:" << std::endl;
		std::cout << "File: " << dt.target_dest << std::endl;
		std::cout << "File size:" << fs::file_size(dt.target_dest) << std::endl;
		ftime = fs::last_write_time(dt.target_dest);
		cftime = decltype(ftime)::clock::to_time_t(ftime);
		localtime_s(&timeinfo, &cftime);
		asctime_s(buffer, &timeinfo);
		std::cout << "File modified:" << buffer << std::endl;
		std::cout << "Which file copy: (Source,Target,Nothing)" << std::endl << std::endl;
		std::string s;
		std::getline(std::cin,s);
		auto type = s.front();
		type = toupper(type);
		switch (type)
		{
		case 'N':
			break;
		case 'S':
			dt.copy_this(ec);
			break;
		case 'T':
			dt.swap_source_target();
			dt.copy_this(ec);
			dt.swap_source_target();
			break;
		default:
			break;
		}
	}

	inline void back_up_function(const path & source_folder, const path & target_folder, size_t time_in_seconds, size_t recursive_depth, type_of_backup type, synchronisation_communication & sc)
	{
		while (!sc.dont_do_next)
		{
			std::vector<std::pair<diff_token, std::error_code>> results;
			char buff[100];
			time_t now = time(0);
			struct tm buffer;
			localtime_s(&buffer,&now);
			strftime(buff, 100, "%Y-%m-%d-%H-%M-%S", &buffer);
			path new_folder= target_folder/buff;
			if (type==only_changed)
			{
				fs::path original=target_folder / "original";
				folder_differences fd;
				if (fs::exists(original))
				{
					differentiate_folders(source_folder, original, recursive_depth, fd);
				}
				else
				{
					auto a = std::vector<fs::path>{ original };
					results = synchronise(source_folder, a, recursive_depth, source_to_target_with_diffs);
					continue;
				}
				results= copy_folder_differences(fd, source_to_target_with_diffs);
				for (auto && element : fd.in_both_folders)
				{
					auto a = element.target_dest.generic_string();
					a.replace(a.find("original"), 8, buff);
					element.target_dest = fs::path(a);
				}
				for (auto && element : fd.only_in_target_or_source)
				{
					auto a = element.target_dest.generic_string();
					a.replace(a.find("original"), 8, buff);
					element.target_dest = fs::path(a);
				}
				auto a = copy_folder_differences(fd, source_to_target_with_diffs);
				results.insert(results.end(), a.begin(), a.end());
			}
			else //copy everything
			{
				folder_differences fd;
				differentiate_folders(source_folder, new_folder, recursive_depth, fd);
				results = copy_folder_differences(fd, source_to_target_with_diffs);
			}
			if (results.size()>0)
			{
				std::unique_lock<std::mutex> lock(sc.mtx_to_errs);
				sc.errs.insert(sc.errs.end(), results.begin(), results.end());
			}
			std::this_thread::sleep_for(std::chrono::seconds(time_in_seconds));
		}
		sc.running = false;
	}

	inline void real_time_sync_function(const path & source_folder, const path & target_folder, size_t time_in_seconds, size_t recursive_depth, type_of_sync type, synchronisation_communication & sc)
	{
		while (!sc.dont_do_next)
		{
			std::vector<std::pair<diff_token, std::error_code>> results;
			folder_differences fd;
			differentiate_folders(source_folder, target_folder, recursive_depth, fd);
			results = copy_folder_differences(fd,type);
			if (results.size()>0)
			{
				std::unique_lock<std::mutex> lock(sc.mtx_to_errs);
				sc.errs.insert(sc.errs.end(), results.begin(), results.end());
			}
			std::this_thread::sleep_for(std::chrono::seconds(time_in_seconds));
		}
		sc.running = false;
	}
}
