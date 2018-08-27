#pragma once
#include <filesystem>
#include "SyncApi.h"
#include <vector>
#include "syncApi_support.cpp"
#include <iomanip>
#include <time.h>
#include <fstream>
#include <thread>

namespace folder_sync
{
	namespace fs = std::experimental::filesystem;

	void diff_token::copy_this(std::error_code & ec)
	{
		fs::create_directory(this->target_dest.parent_path());
		if (this->type==only_in_target)
		{
			fs::copy_file(this->target_dest, this->source_dest, fs::copy_options::overwrite_existing, ec);
			return;
		}
		fs::copy_file(this->source_dest, this->target_dest,fs::copy_options::overwrite_existing, ec);
	}
	void diff_token::swap_source_target()
	{
		auto const s = this->source_dest;
		this->source_dest = this->target_dest;
		this->target_dest = s;
	}

	void differentiate_folders(const fs::path & source_folder,const fs::path & target_folder, size_t recursive_depth, folder_differences & result)
	{
		std::vector<fs::path> source_directories;
		std::vector<fs::path> source_files;
		std::vector<fs::path> target_directories;
		std::vector<fs::path> target_files;
		for (auto & p : fs::directory_iterator(source_folder))
		{
			if (fs::is_directory(p))
			{
				source_directories.push_back(p.path());
			}
			else
			{
				source_files.push_back(p.path());
			}
		}
		for (auto & p : fs::directory_iterator(target_folder))
		{
			if (fs::is_directory(p))
			{
				target_directories.push_back(p.path());
			}
			else
			{
				target_files.push_back(p.path());
			}
		}
		std::sort(source_directories.begin(), source_directories.end(), [](const fs::path& lhs, const fs::path& rhs)
		{
			if (path_compare()(lhs, rhs)>0)
			{
				return 0;
			}
			return 1;
		});
		std::sort(source_files.begin(), source_files.end(), [](const fs::path& lhs, const fs::path& rhs)
		{
			if (path_compare()(lhs, rhs)>0)
			{
				return 0;
			}
			return 1;
		});
		std::sort(target_directories.begin(), target_directories.end(), [](const fs::path& lhs, const fs::path& rhs)
		{
			if (path_compare()(lhs, rhs)>0)
			{
				return 0;
			}
			return 1;
		});
		std::sort(target_files.begin(), target_files.end(), [](const fs::path& lhs, const fs::path& rhs)
		{
			if (path_compare()(lhs, rhs)>0)
			{
				return 0;
			}
			return 1;
		});
		
		auto source_it = source_files.begin();
		auto target_it = target_files.begin();
		while (source_it!=source_files.end() && target_it!= target_files.end())
		{
			
			const auto compare = source_it->filename().compare(target_it->filename());
			
			if (compare<0)
			{
				diff_token dt;
				dt.source_dest = *source_it;
				dt.target_dest = target_folder/ source_it->filename();
				dt.type = only_in_source;
				result.only_in_target_or_source.push_back(dt);
				++source_it;
			}
			if (compare>0)
			{
				diff_token dt;
				dt.source_dest = source_folder / target_it->filename();
				dt.target_dest = *target_it;
				dt.type = only_in_target;
				result.only_in_target_or_source.push_back(dt);
				++target_it;
			}
			if (compare==0)
			{
				const auto compare1 = path_compare()(*source_it, *target_it);
				if (compare1 != 0)
				{
					diff_token dt;
					dt.source_dest = *source_it;
					dt.target_dest = *target_it;
					dt.type = different_source_target;
					result.in_both_folders.push_back(dt);
				}
				++target_it;
				++source_it;
			}
		}
		while (source_it != source_files.end())
		{
			diff_token dt;
			dt.source_dest = *source_it;
			dt.target_dest = target_folder / source_it->filename();
			dt.type = only_in_source;
			result.only_in_target_or_source.push_back(dt);
			++source_it;
		}
		while (target_it != target_files.end())
		{
			diff_token dt;
			dt.source_dest = source_folder / target_it->filename();
			dt.target_dest = *target_it;
			dt.type = only_in_target;
			result.only_in_target_or_source.push_back(dt);
			++target_it;
		}

		if (recursive_depth>0)
		{
			source_it = source_directories.begin();
			target_it = target_directories.begin();
			while (source_it != source_directories.end() && target_it != target_directories.end())
			{
				const auto compare = source_it->filename().compare(target_it->filename());
				if (compare < 0)
				{
					differentiate_folders(source_folder / source_it->filename(), target_folder / source_it->filename(), recursive_depth-1,result);
					++source_it;
				}
				if (compare > 0)
				{
					differentiate_folders(source_folder / target_it->filename(), target_folder / target_it->filename(), recursive_depth - 1, result);
					++target_it;
				}
				if (compare == 0)
				{
					differentiate_folders(source_folder / target_it->filename(), target_folder / target_it->filename(), recursive_depth - 1, result);
					++target_it;
					++source_it;
				}
			}
			while (target_it != target_directories.end())
			{
				differentiate_folders(source_folder / target_it->filename(), target_folder / target_it->filename(), recursive_depth - 1, result);
				++target_it;

			}
			while (source_it != source_directories.end())
			{
				differentiate_folders(source_folder / source_it->filename(), target_folder / source_it->filename(), recursive_depth - 1, result);
				++source_it;
			}
		}
	}


	void write_diffs_to_file(const folder_differences & fd, std::ostream & ofs)
	{
		for (auto && i : fd.in_both_folders)
		{
			ofs << "File in source:" << std::endl;
			ofs << "File: " << i.source_dest << std::endl;
			ofs << "File size:" << fs::file_size(i.source_dest) << std::endl;
			auto ftime = fs::last_write_time(i.source_dest);
			std::time_t cftime = decltype(ftime)::clock::to_time_t(ftime);
			tm timeinfo;
			localtime_s(&timeinfo, &cftime);
			char buffer[256];
			asctime_s(buffer, &timeinfo);
			ofs << "File modified:" << buffer << std::endl << std::endl;

			ofs << "File in target:" << std::endl;
			ofs << "File: " << i.target_dest << std::endl;
			ofs << "File size:" << fs::file_size(i.target_dest) << std::endl;
			ftime = fs::last_write_time(i.target_dest);
			cftime = decltype(ftime)::clock::to_time_t(ftime);
			localtime_s(&timeinfo, &cftime);
			asctime_s(buffer, &timeinfo);
			ofs << "File modified:" << buffer << std::endl;
			ofs << "Which file copy: (Source,Target,Nothing)" << std::endl << std::endl;
		}
	}

	std::vector<read_diffs_token> read_diffs_from_file(std::istream & ifs)
	{
		std::vector<read_diffs_token> result;
		std::string s;
		while (std::getline(ifs,s))
		{
			read_diffs_token rdf;
			rdf.dt.type = different_source_target;
			std::getline(ifs, s);
			rdf.dt.source_dest = fs::path(s.substr(6));
			std::getline(ifs, s);
			std::getline(ifs, s);
			std::getline(ifs, s);
			std::getline(ifs, s);
			std::getline(ifs, s);
			std::getline(ifs, s);
			rdf.dt.target_dest = fs::path(s.substr(6));
			std::getline(ifs, s);
			std::getline(ifs, s);
			std::getline(ifs, s);
			std::getline(ifs, s);
			std::getline(ifs, s);
			auto type = s.front();
			type=toupper(type);
			switch (type)
			{
			case 'N':
				rdf.type = nothing;
				break;
			case 'S':
				rdf.type = source;
				break;
			case 'T':
				rdf.type = target;
				break;
			default:
				rdf.type = nothing;
				break;
			}
			std::getline(ifs, s);
			result.push_back(rdf);
		}
		return result;

	}

	std::vector<std::pair<diff_token, std::error_code>> copy_folder_differences(const folder_differences & fd, type_of_sync type)
	{
		std::ofstream ofs;
		switch (type)
		{
		case both_way_diffs_to_file:
		case source_to_target_diffs_to_file:
		case target_to_source_diffs_to_file:
			static_assert(true, "file not set");
			break;
		}
		return copy_folder_differences(fd, type, ofs);
	}

	std::vector<std::pair<diff_token, std::error_code>> copy_folder_differences(const folder_differences & fd, type_of_sync type, std::ostream & diffs_output)
	{
		switch (type)
		{
		case both_way_diffs_to_file:
		case source_to_target_diffs_to_file:
		case target_to_source_diffs_to_file:
			write_diffs_to_file(fd, diffs_output);
			break;
		}


		std::vector<std::pair<diff_token, std::error_code>> result;
		for (auto element : fd.only_in_target_or_source)
		{
			std::error_code ec;
			switch (type) {
			case bad_type:
				break;
			case both_way_ask_diff:
			case both_way_diffs_to_file:
			case both_way_diffs_to_target:
			case both_way_diffs_to_source:
			case both_way_no_diffs:
				element.copy_this(ec);
				break;

			case source_to_target_with_diffs:
			case source_to_target_ask_diffs:
			case source_to_target_no_diffs:
			case source_to_target_diffs_to_file:
				if (element.type == only_in_source)
				{
					element.copy_this(ec);
				}
				break;
			case target_to_source_with_diffs:
			case target_to_source_ask_diffs:
			case target_to_source_no_diffs:
			case target_to_source_diffs_to_file:
				if (element.type == only_in_target)
				{
					element.copy_this(ec);
				}
				break;
			}
			if (ec)
			{
				result.emplace_back(std::make_pair(element, ec));
			}

		}

		switch (type)
		{
		case both_way_diffs_to_file:
		case source_to_target_diffs_to_file:
		case target_to_source_diffs_to_file:

		case both_way_no_diffs:
		case source_to_target_no_diffs:
		case target_to_source_no_diffs:
			break;
		default:
			for (auto element : fd.in_both_folders)
			{
				std::error_code ec;

				switch (type) {
				case bad_type:
					break;
				case both_way_ask_diff:
				case source_to_target_ask_diffs:
				case target_to_source_ask_diffs:
					console_diff_solve(element, ec);
					break;

				case both_way_diffs_to_target:
				case source_to_target_with_diffs:
					element.swap_source_target();
					element.copy_this(ec);
					element.swap_source_target();
					break;

				case both_way_diffs_to_source:
				case target_to_source_with_diffs:
					element.copy_this(ec);
					break;
				}

				if (ec)
				{
					result.emplace_back(std::make_pair(element, ec));
				}
			}
			break;
		}
		return result;
	}

	std::vector<std::pair<diff_token, std::error_code>> synchronise(const path & source, std::vector<path> & targets, size_t recursive_depth, type_of_sync type)
	{
		std::ofstream ofs;
		switch (type)
		{
		case both_way_diffs_to_file:
		case source_to_target_diffs_to_file:
		case target_to_source_diffs_to_file:
			static_assert(true, "file not set");
			break;
		}
		return synchronise(source, targets, recursive_depth, type, ofs);
	}
	std::vector<std::pair<diff_token, std::error_code>> synchronise(const path & source, std::vector<path> & targets, size_t recursive_depth, type_of_sync type, std::ostream & output)
	{
		folder_differences fd;
		std::vector<std::pair<diff_token, std::error_code>> result;
		for (auto && element : targets)
		{
			differentiate_folders(source, element, recursive_depth, fd);
			std::vector < std::pair<diff_token, std::error_code>> a = copy_folder_differences(fd, type, output);
			result.insert(result.end(),a.begin(),a.end());
		}
		return result;
	}

	std::thread back_up(const path & source_folder, const path & target_folder, size_t time_in_seconds, size_t recursive_depth, type_of_backup type, synchronisation_communication & sc)
	{
		back_up_function(source_folder, target_folder, time_in_seconds, recursive_depth, type, sc);
		std::thread t1(back_up_function, std::ref(source_folder), std::ref(target_folder), time_in_seconds, recursive_depth, type, std::ref(sc));
		return t1;
	}

	std::thread real_time_sync(const path & source_folder, const path & target_folder, size_t time_in_seconds, size_t recursive_depth, type_of_sync type, synchronisation_communication & sc)
	{
		switch (type)
		{
		case both_way_diffs_to_file:
		case source_to_target_diffs_to_file:
		case target_to_source_diffs_to_file:
		case both_way_ask_diff:
		case source_to_target_ask_diffs:
		case target_to_source_ask_diffs:
			static_assert(true, "Wrong type");
			break;
		}
		std::thread thrd = std::thread(real_time_sync_function, std::ref(source_folder), std::ref(target_folder), time_in_seconds, recursive_depth, type, std::ref(sc));
		//real_time_sync_function(source_folder, target_folder, time_in_seconds, recursive_depth,type, sc);
		return thrd;
	}


}
