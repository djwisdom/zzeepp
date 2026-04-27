#include "zep/filesystem.h"
#include "zep/editor.h" // FOR ZEP_UNUSED

#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

#include "zep/mcommon/logger.h"
#include "zep/mcommon/string/stringutils.h"

#undef ERROR

#if defined(ZEP_FEATURE_CPP_FILE_SYSTEM)

#include <filesystem>

#ifdef __APPLE__
namespace cpp_fs = std::__fs::filesystem;
#else
namespace cpp_fs = std::filesystem;
#endif

namespace Zep
{
ZepFileSystemCPP::ZepFileSystemCPP(const fs::path& configPath)
{
    // Use the config path
    m_configPath = configPath;

    m_workingDirectory = fs::path(cpp_fs::current_path().string());

    // Didn't find the config path, try the working directory
    if (!Exists(m_configPath))
    {
        m_configPath = m_workingDirectory;
    }

    ZLOG(INFO, "Config Dir: " << m_configPath.string());
    ZLOG(INFO, "Working Dir: " << m_workingDirectory.string());
}

ZepFileSystemCPP::~ZepFileSystemCPP()
{
}

void ZepFileSystemCPP::SetWorkingDirectory(const fs::path& path)
{
    if (!IsDirectory(path))
    {
        return;
    }
    // Set the file system's current working directory too.
    cpp_fs::current_path(path.string());
    m_workingDirectory = path;
}

const fs::path& ZepFileSystemCPP::GetWorkingDirectory() const
{
    return m_workingDirectory;
}

fs::path ZepFileSystemCPP::GetConfigPath() const
{
    return m_configPath;
}

bool ZepFileSystemCPP::MakeDirectories(const fs::path& path)
{
    return cpp_fs::create_directories(path.c_str());
}

bool ZepFileSystemCPP::IsDirectory(const fs::path& path) const
{
    if (!Exists(path))
    {
        return false;
    }
    return cpp_fs::is_directory(path.string());
}

bool ZepFileSystemCPP::IsReadOnly(const fs::path& path) const
{
    auto perms = cpp_fs::status(path.string()).permissions();
    if ((perms & cpp_fs::perms::owner_write) == cpp_fs::perms::owner_write)
    {
        return false;
    }
    return true;
}

std::string ZepFileSystemCPP::Read(const fs::path& fileName)
{
    // Security enhancement: Check file size limit before reading
    if (m_maxFileSize > 0)
    {
        try
        {
            auto fileSize = cpp_fs::file_size(fileName.string());
            if (fileSize > m_maxFileSize)
            {
                ZLOG(WARNING, "File size exceeds limit: " << fileName.string() 
                      << " (" << fileSize << " bytes > " << m_maxFileSize << " bytes)");
                return std::string();
            }
        }
        catch (const cpp_fs::filesystem_error&)
        {
            // If we can't get file size, fall back to standard reading
            // but this is unlikely to happen if Exists() worked
        }
    }
    
    std::ifstream in(fileName, std::ios::in | std::ios::binary);
    if (in)
    {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(size_t(in.tellg()));
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        return (contents);
    }
    else
    {
        ZLOG(ERROR, "File Not Found: " << fileName.string());
    }
    return std::string();
}

bool ZepFileSystemCPP::Write(const fs::path& fileName, const void* pData, size_t size)
{
    FILE* pFile;
    pFile = fopen(fileName.string().c_str(), "wb");
    if (!pFile)
    {
        return false;
    }
    // Valid to open/close with size == 0, which will truncate
    if (size != 0)
    {
        fwrite(pData, sizeof(uint8_t), size, pFile);
    }
    fclose(pFile);
    return true;
}

void ZepFileSystemCPP::ScanDirectory(const fs::path& path, std::function<bool(const fs::path& path, bool& dont_recurse)> fnScan) const
{
    for (auto itr = cpp_fs::recursive_directory_iterator(path.string());
         itr != cpp_fs::recursive_directory_iterator();
         itr++)
    {
        auto p = fs::path(itr->path().string());

        bool recurse = true;
        if (!fnScan(p, recurse))
            return;

        if (!recurse && itr.recursion_pending())
        {
            itr.disable_recursion_pending();
        }
    }
}

bool ZepFileSystemCPP::Exists(const fs::path& path) const
{
    try
    {
        // Security enhancement: Validate path for traversal attempts
        if (!IsPathSafe(path))
        {
            ZLOG(WARNING, "Unsafe path attempt blocked: " << path.string());
            return false;
        }
        return cpp_fs::exists(path.string());
    }
    catch (cpp_fs::filesystem_error& err)
    {
        ZEP_UNUSED(err);
        ZLOG(ERROR, "Exception: " << err.what());
        return false;
    }
}

// Security enhancement: Set maximum file size for reading (0 = unlimited)
void ZepFileSystemCPP::SetMaxFileSize(size_t maxSize)
{
    m_maxFileSize = maxSize;
}

size_t ZepFileSystemCPP::GetMaxFileSize() const
{
    return m_maxFileSize;
}

// Security enhancement: Validate path for traversal attempts
bool ZepFileSystemCPP::IsPathSafe(const fs::path& path) const
{
    // Convert to string for easier manipulation
    std::string pathStr = path.string();
    
    // Check for obvious path traversal attempts
    if (pathStr.find("..") != std::string::npos) 
    {
        // Additional check: make sure it's not just a file named "..something"
        // Simple approach: split by path separators and check each component
        std::vector<std::string> components;
        std::string component;
        std::istringstream iss(pathStr);
        
        // Handle both path separators
        std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
        std::istringstream iss2(pathStr);
        
        while (std::getline(iss2, component, '/'))
        {
            if (component == "..")
            {
                return false; // Found a parent directory reference
            }
            components.push_back(component);
        }
    }
    
    // Check for null bytes (though string shouldn't contain them if from filesystem)
    if (pathStr.find('\0') != std::string::npos)
    {
        return false;
    }
    
    // Additional safety: limit path length to prevent DoS via extremely long paths
    if (pathStr.length() > 4096) // Reasonable limit for path length
    {
        return false;
    }
    
    return true;
} // namespace Zep

#endif // CPP_FILESYSTEM
