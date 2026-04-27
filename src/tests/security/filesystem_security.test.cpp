#include "zep/filesystem.h"
#include <gtest/gtest.h>
#include <fstream>

using namespace Zep;

TEST(FileSystemSecurity, PathTraversalDetection)
{
    // Create a temporary directory for testing
    fs::path tempDir = fs::temp_directory_path() / "zep_security_test";
    fs::create_directories(tempDir);
    
    // Create a test filesystem
    ZepFileSystemCPP fs(tempDir);
    
    // Test normal path - should exist (the directory itself)
    EXPECT_TRUE(fs.Exists("."));
    
    // Test path traversal attempts - should be blocked
    EXPECT_FALSE(fs.Exists(".."));
    EXPECT_FALSE(fs.Exists("../"));
    EXPECT_FALSE(fs.Exists("..\\"));
    EXPECT_FALSE(fs.Exists("..\\.."));
    EXPECT_FALSE(fs.Exists("../../"));
    EXPECT_FALSE(fs.Exists("subdir/.."));
    EXPECT_FALSE(fs.Exists("subdir/../"));
    EXPECT_FALSE(fs.Exists("a/b/../../../etc/passwd"));
    
    // Test with mixed separators
    EXPECT_FALSE(fs.Exists("a\\b/../../../etc/passwd"));
    EXPECT_FALSE(fs.Exists("a/b\\..\\..\\..\\etc\\passwd"));
    
    // Test null byte injection (though harder to test with std::string)
    // We can at least verify the function handles it
    
    // Test extremely long path
    std::string longPath(4097, 'a');
    EXPECT_FALSE(fs.Exists(longPath));
    
    // Clean up
    fs::remove_all(tempDir);
}

TEST(FileSystemSecurity, FileSizeLimits)
{
    // Create a temporary directory for testing
    fs::path tempDir = fs::temp_directory_path() / "zep_size_test";
    fs::create_directories(tempDir);
    
    // Create a test file
    fs::path testFile = tempDir / "test.txt";
    std::ofstream(testFile) << "Hello, World!";
    
    // Create a test filesystem with size limit
    ZepFileSystemCPP fs(tempDir);
    fs.SetMaxFileSize(5); // 5 bytes limit
    
    // Reading the file should fail due to size limit (13 bytes > 5 bytes)
    std::string contents = fs.Read(testFile);
    EXPECT_TRUE(contents.empty());
    
    // Increase limit above file size - should succeed
    fs.SetMaxFileSize(20); // 20 bytes limit
    contents = fs.Read(testFile);
    EXPECT_EQ(contents, "Hello, World!");
    
    // Test with zero limit (unlimited)
    fs.SetMaxFileSize(0);
    contents = fs.Read(testFile);
    EXPECT_EQ(contents, "Hello, World!");
    
    // Test with non-existent file and size limit
    fs::path nonexistent = tempDir / "nonexistent.txt";
    contents = fs.Read(nonexistent);
    EXPECT_TRUE(contents.empty());
    
    // Clean up
    fs::remove_all(tempDir);
}

TEST(FileSystemSecurity, IsPathSafeFunction)
{
    // Create a temporary directory for testing
    fs::path tempDir = fs::temp_directory_path() / "zep_pathsafe_test";
    fs::create_directories(tempDir);
    
    // Create a test filesystem
    ZepFileSystemCPP fs(tempDir);
    
    // Test safe paths
    EXPECT_TRUE(fs.IsPathSafe("."));
    EXPECT_TRUE(fs.IsPathSafe("..")); // Note: IsPathSafe doesn't check if path exists, just syntax
    EXPECT_TRUE(fs.IsPathSafe("subdir"));
    EXPECT_TRUE(fs.IsPathSafe("subdir/"));
    EXPECT_TRUE(fs.IsPathSafe("subdir\\file.txt"));
    EXPECT_TRUE(fs.IsPathSafe("subdir/file.txt"));
    
    // Test unsafe paths
    EXPECT_FALSE(fs.IsPathSafe(".."));
    EXPECT_FALSE(fs.IsPathSafe("../"));
    EXPECT_FALSE(fs.IsPathSafe("..\\"));
    EXPECT_FALSE(fs.IsPathSafe("..\\.."));
    EXPECT_FALSE(fs.IsPathSafe("../../"));
    EXPECT_FALSE(fs.IsPathSafe("subdir/.."));
    EXPECT_FALSE(fs.IsPathSafe("subdir/../"));
    EXPECT_FALSE(fs.IsPathSafe("a/b/../../../etc/passwd"));
    
    // Test with mixed separators
    EXPECT_FALSE(fs.IsPathSafe("a\\b/../../../etc/passwd"));
    EXPECT_FALSE(fs.IsPathSafe("a/b\\..\\..\\..\\etc\\passwd"));
    
    // Test null bytes
    // Note: Hard to test directly since std::string constructor stops at null
    // But we can verify the check is in place
    
    // Test extremely long path
    std::string longPath(4097, 'a');
    EXPECT_FALSE(fs.IsPathSafe(longPath));
    
    // Clean up
    fs::remove_all(tempDir);
}