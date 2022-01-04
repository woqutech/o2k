/**
 * misc utilities
 */

#ifndef __UTMISC_H
#define __UTMISC_H

#include <functional>
#include <vector>

/**
 * defer execute a closure before exit scope
 */
class MyDefer final {
public:
    explicit MyDefer(std::function<void ()> func) : m_func(func) {}
    ~MyDefer() { m_func(); }
private:
    std::function<void ()> m_func;
};

/**
 * defer execute multi-closures when destruct this object
 */
class MyDeferMulti final {
public:
    ~MyDeferMulti() {
        for (auto it = m_funcs.crbegin(); it != m_funcs.crend(); it++) {
            (*it)();
        }
        m_funcs.clear();
    }
    void defer(const std::function<void ()>& f){
        m_funcs.push_back(f);
    }
private:
    std::vector<std::function<void()>> m_funcs;
};

/**
 * defer free a buffer
 */
template <typename T>
class DeferFree final {
public:
    explicit DeferFree(T *ptr) : m_ptr(ptr) {}
    ~DeferFree() { if (m_ptr != nullptr) delete m_ptr; }
private:
    T *m_ptr = nullptr;
};

using DeferFreeBuf = DeferFree<char>;

/**
 * defer fclose
 */
class DeferFclose final {
public:
    explicit DeferFclose(FILE *fp) : m_fp (fp) {}
    ~DeferFclose() { if (m_fp != nullptr) fclose(m_fp); }
private:
    FILE *m_fp;
};

/**
 *  endian
 */
class HostEndian final {
public:
    static bool isLittleEndian() {
        static const int m_int32 = 1;
        return *((const char *)&m_int32) == 1;
    }
    static bool isBigEndian() {
        static const int m_int32 = 1;
        return *((const char *)&m_int32) == 0;
    }
    static const char * GetEndianName() {
        if (isLittleEndian()) {
            return "Little Endian";
        } else {
            return "Big Endian";
        }
    }
};

#endif // __UTMISC_H
