#pragma once

#include <windows.h>

#include <utility>

namespace ps3translator::hid
{
    // Wrapper RAII para HANDLE do Windows: fecha o handle automaticamente.
    class UniqueHandle
    {
    public:
        UniqueHandle() = default;
        explicit UniqueHandle(HANDLE handle) : handle_(handle) {}
        ~UniqueHandle() { reset(); }

        UniqueHandle(const UniqueHandle&) = delete;
        UniqueHandle& operator=(const UniqueHandle&) = delete;

        UniqueHandle(UniqueHandle&& other) noexcept : handle_(std::exchange(other.handle_, INVALID_HANDLE_VALUE)) {}
        UniqueHandle& operator=(UniqueHandle&& other) noexcept
        {
            if (this != &other)
            {
                reset();
                handle_ = std::exchange(other.handle_, INVALID_HANDLE_VALUE);
            }
            return *this;
        }

        [[nodiscard]] HANDLE get() const { return handle_; }
        [[nodiscard]] bool valid() const { return handle_ != INVALID_HANDLE_VALUE && handle_ != nullptr; }

        void reset(HANDLE handle = INVALID_HANDLE_VALUE)
        {
            if (valid())
            {
                CloseHandle(handle_);
            }
            handle_ = handle;
        }

    private:
        HANDLE handle_ = INVALID_HANDLE_VALUE;
    };
}
