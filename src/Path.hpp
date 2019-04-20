#pragma once

#include "Allocator.hpp"
#include "Defines.hpp"
#include "Collections/StringView.hpp"
#include <assert.h>

struct Path
{
    Allocator* allocator;
    char* data;
    size_t cap;
    size_t len;
    
public:
    Path()
        : Path(nullptr)
    {}

    Path(Allocator* allocator)
        : allocator(allocator)
        , data(nullptr)
        , cap(0)
        , len(0)
    {}
    
    Path(const Path& other_path) = delete;
    Path& operator=(const Path& other_path) = delete;
    
    Path(Path&& other_path)
        : allocator(nullptr)
        , data(nullptr)
        , cap(0)
        , len(0)
    {
        *this = std::move(other_path);
    }
    
    Path& operator=(Path&& other_path)
    {
        allocator = other_path.allocator;
        cap = other_path.cap;
        len = other_path.len;
        if (data) {
            allocator->Deallocate(data);
        }
        data = other_path.data;
        other_path.allocator = nullptr;
        other_path.data = nullptr;
        other_path.len = 0;
        other_path.cap = 0;
        return *this;
    }
    
    void Destroy()
    {
        if (data) {
            allocator->Deallocate(data);
            data = nullptr;
        }
        allocator = nullptr;
    }

    void Push(const Path& other_path)
    {
        Push(other_path.data, other_path.len);
    }

    void Push(const StringView& str)
    {
        Push(str.data, str.len);
    }

    void Push(const char* str)
    {
        Push(str, strlen(str));
    }
    
    void Push(const char* str_data, size_t str_len)
    {
        assert(allocator);
        if (str_len == 0) {
            return;
        }
        
        // +2 here is necessary:
        //  - +1 for the null terminated string
        //  - +1 for the extra '/' we might add
        size_t needed_cap = len + str_len + 2;
        if (needed_cap > cap) {
            Resize(needed_cap);
        }
        
        if (len == 0) {
            memcpy(data, str_data, str_len);
            len = str_len;
            data[len] = 0;
            return;
        }
        
        if (data[len-1] != '/') {
            data[len++] = '/';
        }
        
        if (str_data[0] == '/') {
            memcpy(data + len, str_data + 1, str_len - 1);
            len += str_len - 1;
        } else {
            memcpy(data + len, str_data, str_len);
            len += str_len;
        }
        
        assert(len < cap);
        data[len] = 0;
    }

    void Resize(size_t desired_cap)
    {
        const float factor = 1.5f;
        size_t new_cap = MAX(cap * factor, desired_cap);
        char* buf = (char*)allocator->Allocate(new_cap);
        assert(buf);

        // copy contents
        memcpy(buf, data, len);
        buf[len] = 0;

        allocator->Deallocate(data);
        data = buf;
        cap = new_cap;
    }
};
