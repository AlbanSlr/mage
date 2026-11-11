#include <mage/core/File.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

TEST_CASE("A binary file is read back byte for byte", "[core][file]")
{
    const std::filesystem::path filePath = std::filesystem::temp_directory_path() / "mage_core_file_test.bin";

    // Includes a zero and a newline byte: both must survive untouched.
    const std::vector<char> writtenBytes = {'M', 'A', 'G', 'E', '\0', '\n', '\x7f'};
    {
        std::ofstream file(filePath, std::ios::binary);
        file.write(writtenBytes.data(), static_cast<std::streamsize>(writtenBytes.size()));
    }

    const std::vector<std::byte> readBytes = mage::readBinaryFile(filePath);
    std::filesystem::remove(filePath);

    REQUIRE(readBytes.size() == writtenBytes.size());
    for (std::size_t byteIndex = 0; byteIndex < readBytes.size(); ++byteIndex)
    {
        CHECK(readBytes[byteIndex] == static_cast<std::byte>(writtenBytes[byteIndex]));
    }
}

TEST_CASE("Reading a missing file throws", "[core][file]")
{
    const std::filesystem::path missingFilePath =
        std::filesystem::temp_directory_path() / "mage_core_file_that_does_not_exist.bin";

    CHECK_THROWS_AS(mage::readBinaryFile(missingFilePath), std::runtime_error);
}
