#include "SyncApi.h"
#include <cstring>
#include <iostream>
#include <fstream>

int main(int argc, char *argv[])
{

	namespace fs = std::experimental::filesystem;
	bool wrong_arguments = false;
	fs::path source;
	fs::path target;
	fs::path compare_file;
	bool compare_file_set = false;
	bool real_time = false;
	bool back_up = false;
	int time_in_seconds = 60;
	bool diffs_to_file = false;
	bool diffs_from_file = false;
	int recursive_depth = 0;
	folder_sync::type_of_sync type_of_sync = folder_sync::type_of_sync::not_set;
	folder_sync::type_of_backup type_of_backup = folder_sync::type_of_backup::backup_not_set;
	try {
		for (int i = 1; i < argc; i++)
		{
			if (strncmp(argv[i], "-source", 7) == 0)
			{
				source = fs::path(argv[i + 1]);
				i++;
				continue;
			}
			if (strncmp(argv[i], "-target", 7) == 0)
			{
				target = fs::path(argv[i + 1]);
				i++;
				continue;
			}
			if (strncmp(argv[i], "-type", 5) == 0)
			{
				type_of_sync = folder_sync::parse_type_of_sync(argv[i + 1]);
				i++;
				continue;
			}
			if (strncmp(argv[i], "-rts", 4) == 0)
			{
				real_time = true;
				continue;
			}
			if (strncmp(argv[i], "-backup", 6) == 0)
			{
				back_up = true;
				if (strncmp(argv[i + 1], "everything", 10) == 0)
				{
					type_of_backup = folder_sync::type_of_backup::everything;
					i++;
				}
				else if (strncmp(argv[i + 1], "only-changed", 12) == 0)
				{
					type_of_backup = folder_sync::type_of_backup::only_changed;
					i++;
				}
				else
				{
					wrong_arguments = true;
				}
				continue;
			}
			if (strncmp(argv[i], "-compare_file", 13) == 0)
			{
				compare_file_set = true;
				compare_file = fs::path(argv[i + 1]);
				i++;
				continue;
			}
			if (strncmp(argv[i], "-recursive", 10) == 0)
			{
				recursive_depth = std::stoi(argv[i + 1]);
				i++;
				continue;
			}
			if (strncmp(argv[i], "-time", 5) == 0)
			{
				time_in_seconds = std::stoi(argv[i + 1]);
				i++;
				continue;
			}
			if (strncmp(argv[i], "-diffs_from_file", 10) == 0)
			{
				diffs_from_file = true;
				continue;
			}
			if (strncmp(argv[i], "-diffs_to_file", 10) == 0)
			{
				diffs_to_file = true;
			}
		}
	}
	catch (...)
	{
		wrong_arguments = true;
	}

#pragma region checkingarguments
	int boolcounter = 0;
	if (back_up)
	{
		boolcounter++;
	}
	if (diffs_from_file)
	{
		if (!compare_file_set)
		{
			wrong_arguments = true;
		}
		boolcounter++;
	}
	if (diffs_to_file)
	{
		if (!compare_file_set)
		{
			wrong_arguments = true;
		}
		boolcounter++;
	}
	if (real_time)
	{
		boolcounter++;
	}
	if (boolcounter > 1)
	{
		wrong_arguments = true;
		std::cout << "Too many things to do";
	}
	if (type_of_sync == folder_sync::type_of_sync::bad_type)
	{
		wrong_arguments = true;
		std::cout << "Bad type";
	}
	if (wrong_arguments)
	{
		std::cout << "Wrong arguments. Not doing anything";
		return 1;
	}

#pragma endregion checkingarguments
	if (back_up)
	{
		std::string s;
		folder_sync::synchronisation_communication sc;
		auto t = folder_sync::back_up(source, target, time_in_seconds, recursive_depth, type_of_backup, sc);
		std::cout << "To stop backuping write 's'";
		while (s != "s")
		{
			std::cin >> s;
		}
		sc.dont_do_next = true;
		t.join();
		return 0;
	}
	if (real_time)
	{
		std::string s;
		folder_sync::synchronisation_communication sc;
		auto t = folder_sync::real_time_sync(source, target, time_in_seconds, recursive_depth, type_of_sync, sc);
		std::cout << "To stop real tyme sync write 's'";
		while (s != "s")
		{
			std::cin >> s;
		}
		sc.dont_do_next = true;
		t.join();
		return 0;
	}
	if (diffs_to_file)
	{
		folder_sync::folder_differences fd;
		folder_sync::differentiate_folders(source, target, recursive_depth, fd);
		std::ofstream ofs(compare_file.c_str());
		folder_sync::write_diffs_to_file(fd, ofs);
		return 0;
	}
	if (diffs_from_file)
	{
		std::ifstream ifs(compare_file.c_str());
		auto ftd = folder_sync::read_diffs_from_file(ifs);
		for (auto && element : ftd)
		{
			std::error_code ec;
			if (element.type == folder_sync::target)
			{
				element.dt.copy_this(ec);
			}
			else if (element.type == folder_sync::source)
			{
				element.dt.swap_source_target();
				element.dt.copy_this(ec);
			}
		}
		return 0;
	}
	auto a = std::vector<fs::path>{ target };
	if (compare_file_set)
	{
		std::ofstream ofs(compare_file.c_str());
		folder_sync::synchronise(source, a, recursive_depth, type_of_sync, ofs);
	}
	else
		folder_sync::synchronise(source, a, recursive_depth, type_of_sync);
}
