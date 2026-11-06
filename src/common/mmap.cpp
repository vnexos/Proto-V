/**
 * @file mmap.cpp
 * @brief Cross-platform implementation of RAII MemoryMappedFile for Linux and Windows.
 */

#include <neo/mmap.hpp>

#include <utility>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

static_assert(sizeof(void*) == sizeof(HANDLE), "HANDLE size must match void*");
static_assert(reinterpret_cast<void*>(-1) == INVALID_HANDLE_VALUE, "INVALID_HANDLE_VALUE mismatch");
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace neo {

MemoryMappedFile::~MemoryMappedFile() {
    close();
}

MemoryMappedFile::MemoryMappedFile(MemoryMappedFile&& other) noexcept {
    *this = std::move(other);
}

MemoryMappedFile& MemoryMappedFile::operator=(MemoryMappedFile&& other) noexcept {
    if (this != &other) {
        close();
        data_ = other.data_;
        size_ = other.size_;

#if defined(_WIN32)
        file_handle_ = other.file_handle_;
        mapping_handle_ = other.mapping_handle_;
        other.file_handle_ = reinterpret_cast<void*>(-1);
        other.mapping_handle_ = nullptr;
#else
        fd_ = other.fd_;
        other.fd_ = -1;
#endif

        other.data_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

bool MemoryMappedFile::open(const std::string& path) {
    return open(std::filesystem::path(path));
}

#if !defined(_WIN32)

bool MemoryMappedFile::open(const std::filesystem::path& path) {
    close();

    if (path.empty()) {
        return false;
    }

    std::string path_str = path.string();
    fd_ = ::open(path_str.c_str(), O_RDONLY);
    if (fd_ < 0) {
        return false;
    }

    struct stat st{};
    if (::fstat(fd_, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0) {
        close();
        return false;
    }

    size_ = static_cast<size_t>(st.st_size);
    void* ptr = ::mmap(nullptr, size_, PROT_READ, MAP_SHARED, fd_, 0);
    if (ptr == MAP_FAILED) {
        close();
        return false;
    }

    data_ = static_cast<const uint8_t*>(ptr);
    return true;
}

void MemoryMappedFile::close() noexcept {
    if (data_) {
        ::munmap(const_cast<uint8_t*>(data_), size_);
        data_ = nullptr;
    }

    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }

    size_ = 0;
}

#else

/**
 * @brief Helper creating a read-only file mapping for a Windows file handle.
 * @param file Active Windows file handle.
 * @param out_size Output parameter receiving the validated file size in bytes.
 * @return Mapping handle on success, or nullptr on failure.
 */
static HANDLE create_read_mapping(HANDLE file, size_t& out_size) {
    LARGE_INTEGER li{};
    if (!::GetFileSizeEx(file, &li) || li.QuadPart <= 0) {
        return nullptr;
    }

    out_size = static_cast<size_t>(li.QuadPart);
    return ::CreateFileMappingA(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
}

bool MemoryMappedFile::open(const std::filesystem::path& path) {
    close();

    if (path.empty()) {
        return false;
    }

    std::string path_str = path.string();
    HANDLE file = ::CreateFileA(
        path_str.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    file_handle_ = file;

    HANDLE mapping = create_read_mapping(file, size_);
    if (!mapping) {
        close();
        return false;
    }
    mapping_handle_ = mapping;

    void* view = ::MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    if (!view) {
        close();
        return false;
    }

    data_ = static_cast<const uint8_t*>(view);
    return true;
}

void MemoryMappedFile::close() noexcept {
    // Strict cleanup order: UnmapViewOfFile -> CloseHandle(mapping) -> CloseHandle(file)
    if (data_) {
        ::UnmapViewOfFile(data_);
        data_ = nullptr;
    }

    if (mapping_handle_) {
        ::CloseHandle(static_cast<HANDLE>(mapping_handle_));
        mapping_handle_ = nullptr;
    }

    if (file_handle_ != reinterpret_cast<void*>(-1)) {
        ::CloseHandle(static_cast<HANDLE>(file_handle_));
        file_handle_ = reinterpret_cast<void*>(-1);
    }

    size_ = 0;
}

#endif

} // namespace neo
