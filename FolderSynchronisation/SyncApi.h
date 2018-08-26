#pragma once
#include <filesystem>
#include <vector>
#include <mutex>
#include <atomic>

namespace folder_sync
{
	using namespace std::experimental::filesystem;

	enum type_of_copy { only_in_source, only_in_target, different_source_target };

	//ask diff means interactive console asking
	enum type_of_sync {
		bad_type,
		both_way_ask_diff,
		both_way_diffs_to_file,
		both_way_diffs_to_target,
		both_way_diffs_to_source,
		both_way_no_diffs,
		source_to_target_with_diffs,
		source_to_target_ask_diffs,
		source_to_target_no_diffs,
		source_to_target_diffs_to_file,
		target_to_source_with_diffs,
		target_to_source_ask_diffs,
		target_to_source_no_diffs,
		target_to_source_diffs_to_file,
		not_set
	};
	
	inline type_of_sync parse_type_of_sync(const char* chr)
	{ 
		if (strncmp(chr, "bwad", 4) == 0)
		{
			return both_way_ask_diff;
		}
		if (strncmp(chr, "bwdtf", 4) == 0)
		{
			return both_way_diffs_to_file;
		}
		if (strncmp(chr, "bwdtt", 4) == 0)
		{
			return both_way_diffs_to_target;
		}
		if (strncmp(chr, "bwdts", 4) == 0)
		{
			return type_of_sync::both_way_diffs_to_source;
		}
		if (strncmp(chr, "bwnd", 4) == 0)
		{
			return type_of_sync::both_way_no_diffs;
		}
		if (strncmp(chr, "sttwd", 4) == 0)
		{
			return type_of_sync::source_to_target_with_diffs;
		}
		if (strncmp(chr, "sttad", 4) == 0)
		{
			return type_of_sync::source_to_target_ask_diffs;
		}
		if (strncmp(chr, "sttnd", 4) == 0)
		{
			return type_of_sync::source_to_target_no_diffs;
		}
		if (strncmp(chr, "sttdtf", 4) == 0)
		{
			return type_of_sync::source_to_target_diffs_to_file;
		}
		if (strncmp(chr, "ttswd", 4) == 0)
		{
			return type_of_sync::target_to_source_with_diffs;
		}
		if (strncmp(chr, "ttsad", 4) == 0)
		{
			return type_of_sync::target_to_source_ask_diffs;
		}
		if (strncmp(chr, "ttsnd", 4) == 0)
		{
			return target_to_source_no_diffs;
		}
		if (strncmp(chr, "ttsdtf", 4) == 0)
		{
			return target_to_source_diffs_to_file;
		}
		return type_of_sync::bad_type;
	}
	
	enum type_of_backup { everything, only_changed, backup_not_set  };

	enum read_diff_type { source, target, nothing};

	class diff_token
	{
	public:
		path source_dest;
		path target_dest;
		type_of_copy type;
		void diff_token::copy_this(std::error_code & ec);
		void swap_source_target();
	};

	struct read_diffs_token
	{
		diff_token dt;
		read_diff_type type;
	};
	struct synchronisation_communication
	{
	public:
		std::vector<std::pair<diff_token, std::error_code>> errs;
		std::mutex mtx_to_errs;

	public:
		std::atomic<bool> running = true;
		std::atomic<bool> dont_do_next = false;
	};

	struct folder_differences
	{
	public:
		std::vector<diff_token> in_both_folders;
		std::vector<diff_token> only_in_target_or_source;
	};



	void differentiate_folders(const path & source_folder, const path & target_folder, size_t recursive_depth, folder_differences & result);

	//write diffs to file which can user edit
	void write_diffs_to_file(const folder_differences & fd, std::ostream & ofs);

	//read user input from diff file, return as folder_differences
	std::vector<read_diffs_token> read_diffs_from_file(std::istream & ifs);

	std::vector<std::pair<diff_token,std::error_code>> copy_folder_differences(const folder_differences & fd, type_of_sync type);

	std::vector<std::pair<diff_token, std::error_code>> copy_folder_differences(const folder_differences & fd, type_of_sync type, std::ostream & diffs_output);






	void back_up(const path & source_folder,const path & target_folder, size_t time_in_seconds, size_t recursive_depth, type_of_backup type, synchronisation_communication & sc);

	//call synchronise every x seconds
	std::thread real_time_sync(const path & source_folder, const path & target_folder, size_t time_in_seconds, size_t recursive_depth, type_of_sync type, synchronisation_communication & sc);

	//first call differentiate_folders to each target then call copy_folder_differences
	std::vector<std::pair<diff_token, std::error_code>> synchronise(const path & source, std::vector<path> & targets, size_t recursive_depth, type_of_sync type);

	std::vector<std::pair<diff_token, std::error_code>> synchronise(const path & source, std::vector<path> & targets, size_t recursive_depth, type_of_sync type, std::ostream & output);
}
