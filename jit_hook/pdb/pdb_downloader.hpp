#pragma once
#include <sstream>
#include <filesystem>
#include <fstream>
#include "urlmon.h"

namespace jit_hook::pdb
{
	// https://llvm.org/docs/PDB/MsfFile.html#the-superblock
	struct super_block
	{
		char signature[0x20];
		int32_t block_size;
		int32_t free_block_map_block;
		int32_t num_blocks;
	};

	struct cv_info_pdb70
	{
		uint32_t cv_signature;
		GUID signature;
		uint32_t age;
		char pdb_file_name[0];
	};

	inline std::string guid_to_string(const GUID& guid)
	{
		auto result = std::format("{:08X}{:04X}{:04X}", guid.Data1, guid.Data2, guid.Data3);

		for (const int32_t i : guid.Data4)
			result += std::format("{:02X}", i);

		return result;
	}

	inline bool is_correct_size(const std::filesystem::path& pdb_file_path)
	{
		auto pdb = std::ifstream{ pdb_file_path, std::ios::binary };
		auto block = super_block{};
		pdb.read(reinterpret_cast<char*>(&block), sizeof(super_block));
		const auto expected_size = block.num_blocks * block.block_size;
		const auto actual_size = static_cast<int>(std::filesystem::file_size(pdb_file_path));
		return expected_size == actual_size;
	}

	inline std::tuple<std::string, std::string> parse_pdb_info(const std::string_view module_name)
	{
		const auto module_address = reinterpret_cast<uint8_t*>(GetModuleHandleA(module_name.data()));
		if (!module_address)
			throw std::invalid_argument(std::format("Failed to obtain the base address of {}", module_name));

		unsigned long directory_size;
		auto debug_directory = reinterpret_cast<IMAGE_DEBUG_DIRECTORY*>(
			ImageDirectoryEntryToDataEx(module_address, 1, IMAGE_DIRECTORY_ENTRY_DEBUG, &directory_size, nullptr));

		for (auto i = 0u; i < directory_size / sizeof(IMAGE_DEBUG_DIRECTORY); ++i, ++debug_directory)
		{
			if (debug_directory->Type != IMAGE_DEBUG_TYPE_CODEVIEW)
				continue;

			const auto cv_info = reinterpret_cast<cv_info_pdb70*>(module_address + debug_directory->AddressOfRawData);
			if (cv_info->cv_signature != 0x53445352) // SDSR
				continue;

			const auto signature = std::format("{}{}", guid_to_string(cv_info->signature), cv_info->age);
			const auto pdb_name = std::filesystem::path{ cv_info->pdb_file_name }.filename().string();
			return { signature, pdb_name };
		}
		
		throw std::invalid_argument(std::format("Failed to obtain PDB name and signature of {}", module_name));
	}

	inline void download_pdb(const std::string_view module_name)
	{
		const auto [signature, pdb_name] = parse_pdb_info(module_name);

		auto pdb_file_path = std::filesystem::current_path() / "Symbols" / pdb_name / signature;
		std::filesystem::create_directories(pdb_file_path);
		pdb_file_path /= pdb_name;

		if (std::filesystem::exists(pdb_file_path) && is_correct_size(pdb_file_path))
			return;

		const auto pdb_download_url = std::format("http://msdl.microsoft.com/download/symbols/{}/{}/{}",
			pdb_name, signature, pdb_name);

		URLDownloadToFileA(nullptr, pdb_download_url.c_str(), pdb_file_path.string().c_str(), 0, nullptr);

		if (std::filesystem::exists(pdb_file_path) && is_correct_size(pdb_file_path))
			return;

		std::filesystem::remove(pdb_file_path);
		throw std::invalid_argument(std::format("Failed to download the PDB of {}", module_name));
	}
}