/** The code in this file is distributed under the following license:

    Copyright 2025 Oleh Kniaziev

    Redistribution and use in source and binary forms, with or without modification, are permitted
    provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this list of
    conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice, this list of
    conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

    3. Neither the name of the copyright holder nor the names of its contributors may be used to
    endorse or promote products derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
    FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
    BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
    OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
    EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef OK_H_
#define OK_H_

#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>

#include <fcntl.h>
#include <sys/types.h>

#ifndef OK_ASSERT
#define OK_ASSERT(x) do { \
    if (!(x)) { \
        fprintf(stderr, "%s:%d: Assertion failed: %s\n", __FILE__, __LINE__, #x); \
        abort(); \
    } \
} while(0)
#endif // OK_ASSERT

#define OK_TODO() do { \
    fprintf(stderr, "%s:%d: TODO: Not implemented\n", __FILE__, __LINE__); \
    exit(1); \
} while (0)

#define OK_UNUSED(arg) (void)(arg);

#define OK_UNREACHABLE() do { \
    fprintf(stderr, "%s:%d: Encountered unreachable code\n", __FILE__, __LINE__); \
    abort(); \
} while (0)

#define OK_PANIC(msg) do { \
    fprintf(stderr, "%s:%d: PROGRAM PANICKED: %s\n", __FILE__, __LINE__, (msg)); \
    abort(); \
} while (0)

#define OK_PANIC_FMT(fmt, ...) do { \
    fprintf(stderr, "%s:%d: PROGRAM PANICKED: " fmt "\n", __FILE__, __LINE__, __VA_ARGS__); \
    abort(); \
} while (0)

#define OK_ARR_LEN(xs) (sizeof((xs)) / sizeof((xs)[0]))

#if defined(__unix__) || defined(__unix) || defined(__APPLE__)

#define OK_UNIX 1
#define OK_WINDOWS 0

#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <spawn.h>

#define OK_ALLOC_PAGE(sz) (mmap(NULL, (sz), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0))
#define OK_DEALLOC_PAGE(page, size) (munmap((page), (size)))
#define OK_ALLOC_SMOL(sz) (sbrk((sz)))

#define OK_PAGE_SIZE 4096
#define OK_PAGE_ALIGN OK_PAGE_SIZE

#elif defined(_WIN32)

#define OK_UNIX 0
#define OK_WINDOWS 1

#include <windows.h>
#include <memoryapi.h>
#include <heapapi.h>
#include <io.h>
#include <share.h>

#undef max
#undef min

#define OK_PAGE_SIZE 4096
#define OK_PAGE_ALIGN (64 * 1024)

#define OK_ALLOC_PAGE(sz) (VirtualAlloc(nullptr, (sz), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))
#define OK_DEALLOC_PAGE(page, size) (VirtualFree((page), 0, MEM_RELEASE))
#define OK_ALLOC_SMOL(sz) (HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (sz)))

#else
#error "Only UNIX-like systems and Windows are supported"
#endif // platform check

#if SIZE_MAX == UINT32_MAX
#  define OK_BITS_32
#elif SIZE_MAX == UINT64_MAX
#  define OK_BITS_64
#else
#  error "Could not determine architecture bit size"
#endif // word size check

using U8 = uint8_t;
using U16 = uint16_t;
using U32 = uint32_t;
using U64 = uint64_t;

using S8 = int8_t;
using S16 = int16_t;
using S32 = int32_t;
using S64 = int64_t;

using F32 = float;
using F64 = double;

#if defined(OK_BITS_32)
    using UZ = uint32_t;
    using SZ = int32_t;
#elif defined(OK_BITS_64)
    using UZ = uint64_t;
    using SZ = int64_t;
#else
#error "Could not determine architecture bit size"
#endif

#ifdef __GNUC__
#define OK_ATTRIBUTE_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
#else
#define OK_ATTRIBUTE_PRINTF(fmt, args)
#endif // __GNUC__

namespace ok {
// min and max
template <typename T>
constexpr const T& max(const T& a) {
    return a;
}

template <typename T, typename... Rest>
constexpr const T& max(const T& x, const Rest&... xs) {
    const T& xs_max = max(xs...);
    return x > xs_max ? x : xs_max;
}

template <typename T>
constexpr const T& min(const T& a) {
    return a;
}

template <typename T, typename... Rest>
constexpr const T& min(const T& x, const Rest&... xs) {
    const T& xs_min = min(xs...);
    return x < xs_min ? x : xs_min;
}

struct Allocator {
    // required methods
    virtual void* raw_alloc(UZ size) = 0;
    virtual void raw_dealloc(void* ptr, UZ size) = 0;

    // optional methods
    virtual void* raw_resize(void* ptr, UZ old_size, UZ new_size) {
        void* new_ptr = raw_alloc(new_size);
        memcpy(new_ptr, ptr, old_size);
        raw_dealloc(ptr, old_size);
        return new_ptr;
    }

    // provided methods
    template <typename T>
    inline T* alloc(UZ size = 1) {
        return (T*)raw_alloc(sizeof(T) * size);
    }

    template <typename T>
    inline void dealloc(T* ptr, UZ count) {
        return raw_dealloc((void*)ptr, count * sizeof(T));
    }

    template <typename T>
    inline T* resize(T* ptr, UZ old_size, UZ new_size) {
        T* new_ptr = alloc<T>(new_size);
        memcpy((void*)new_ptr, ptr, old_size * sizeof(T));
        dealloc<T>(ptr, old_size);
        return new_ptr;
    }

    inline char* strdup(const char* cstr) {
        UZ len = strlen(cstr);
        char* result = alloc<char>(len + 1);
        memcpy((void*)result, cstr, len * sizeof(char));
        result[len] = '\0';
        return result;
    }
};

extern Allocator* temp_allocator;
extern Allocator* static_allocator;

struct FixedBufferAllocator : public Allocator {
    void* raw_alloc(UZ size) override;
    void raw_dealloc(void* ptr, UZ size) override;

    static constexpr UZ DEFAULT_PAGE_COUNT = 5;

    void* buffer;
    UZ buffer_size;
    UZ buffer_off;
};

static inline uintptr_t align_up(uintptr_t size, uintptr_t align) {
    return size + ((align - (size & (align - 1))) & (align - 1));
}

static inline uintptr_t align_down(uintptr_t size, uintptr_t align) {
    return size - (size & (align - 1));
}

struct ArenaAllocator : public Allocator {
    struct Region {
        UZ avail() const {
            OK_ASSERT(off <= size);
            return size - off;
        }

        Region* next;
        void* data;
        UZ size;
        UZ off;
    };

    void* raw_alloc(UZ size) override;
    void raw_dealloc(void* ptr, UZ size) override;
    void* raw_resize(void* ptr, UZ old_size, UZ new_size) override;

    Region* alloc_region(UZ size);

    inline UZ avail() const {
        UZ result = 0;
        for (Region* r = head; r != nullptr; r = r->next) result += r->avail();
        return result;
    }

    inline void reserve(UZ bytes) {
        UZ available_bytes = avail();

        if (available_bytes < bytes) {
            UZ bytes_needed = bytes - available_bytes;
            Region* r = alloc_region(bytes_needed);
            r->next = head;
            head = r;
        }
    }

    inline void reset() {
        for (Region* r = head; r != nullptr; r = r->next) r->off = 0;
    }

    inline void free() {
        for (Region* r = head; r != nullptr; r = r->next) OK_DEALLOC_PAGE(head->data, head->size);
    }

    Region* head;
    Region* region_pool;
};

// templates
template <typename T>
struct Slice;

template <typename Self, typename T>
struct ArrayBase {
    Slice<T> slice(UZ start, UZ end);
    Slice<T> slice(UZ start);
    Slice<T> slice();
    Slice<const T> slice(UZ start, UZ end) const;
    Slice<const T> slice(UZ start) const;
    Slice<const T> slice() const;

    UZ find_index(const T& elem) const {
        auto* self = self_cast();
        UZ count = self->get_count();
        const T* items = self->get_items();

        for (UZ i = 0; i < count; i++) {
            if (items[i] == elem) {
                return i;
            }
        }

        return (UZ)-1;
    }

    template <typename F>
    UZ find_index(F pred) const {
        auto* self = self_cast();
        UZ count = self->get_count();
        const T* items = self->get_items();

        for (UZ i = 0; i < count; i++) {
            if (pred(items[i])) {
                return i;
            }
        }

        return (UZ)-1;
    }

    inline T& operator [](UZ idx) {
        auto* self = self_cast();
        OK_ASSERT(idx < self->get_count());
        return self->get_items()[idx];
    }

    inline const T& operator [](UZ idx) const {
        auto* self = self_cast();
        OK_ASSERT(idx < self->get_count());
        return self->get_items()[idx];
    }

    Self* self_cast() {
        return static_cast<Self*>(this);
    }

    const Self* self_cast() const {
        return static_cast<const Self*>(this);
    }
};

template <typename T, UZ N>
struct Array : public ArrayBase<Array<T, N>, T> {
    T items[N];

    UZ get_count() const {
        return N;
    }

    T* get_items() {
        return items;
    }

    const T* get_items() const {
        return items;
    }
};

#define OK_LIST_GROW_FACTOR(x) ((((x) + 1) * 3) >> 1)

template <typename T>
struct List : public ArrayBase<List<T>, T> {
    static List<T> alloc(Allocator* a, UZ cap = List::DEFAULT_CAP);

    static constexpr UZ DEFAULT_CAP = 7;

    void push(const T& item);
    void extend(List<T> other);
    void remove_at(UZ idx);

    List<T> copy(Allocator* a, UZ start, UZ end) const;

    inline List<T> copy(Allocator* a, UZ start) const {
        return copy(a, start, count);
    }

    inline List<T> copy(Allocator* a) const {
        return copy(a, 0, count);
    }

    void reserve(UZ new_cap);

    UZ find_index(const T& elem);
    template <typename F>
    UZ find_index(F pred);

    UZ get_count() const {
        return count;
    }

    T* get_items() {
        return items;
    }

    const T* get_items() const {
        return items;
    }

    template <typename Dest>
    inline List<Dest> cast() {
        List<Dest> list;
        list.allocator = allocator;
        list.items = (Dest*)items;
        list.count = count;
        list.capacity = capacity;
        return list;
    }

    inline T& pop() {
        OK_ASSERT(count > 0);
        return items[--count];
    }

    T* items;
    UZ count;
    UZ capacity;
    Allocator* allocator;
};

template <typename T>
struct Slice : public ArrayBase<Slice<T>, T> {
    Slice() = default;
    Slice(T* items, UZ count) : items{items}, count{count} {}

    UZ get_count() const {
        return count;
    }

    T* get_items() {
        return items;
    }

    const T* get_items() const {
        return items;
    }

    inline Slice<T> copy(Allocator* allocator) const {
        T* ptr = allocator->alloc<T>(count);
        memcpy((void*)ptr, items, count * sizeof(T));
        return Slice<T>{ptr, count};
    }

    template <typename Dest>
    inline Slice<const Dest> cast() const {
        Slice<Dest> slice;
        slice.items = (const Dest*)items;
        slice.count = count;
        return slice;
    }

    template <typename Dest>
    inline Slice<Dest> cast() {
        Slice<Dest> slice;
        slice.items = (Dest*)items;
        slice.count = count;
        return slice;
    }

    T* items;
    UZ count;
};

// char predicates
bool is_whitespace(char);
bool is_digit(char);
bool is_alpha(char);

struct String;

#define OK_SV_FMT "%.*s"
#define OK_SV_ARG(sv) (int)(sv).count, reinterpret_cast<const char*>((sv).data)

struct StringView {
    StringView() = default;

    constexpr StringView(const char* data, UZ count) : data{data}, count{count} {}
    explicit StringView(const char* cstr) : StringView{cstr, strlen(cstr)} {}

    String to_string(Allocator* a) const;

    inline StringView view(UZ start, UZ end) const {
        OK_ASSERT(end >= start);
        return StringView{data + start, end - start};
    }

    inline StringView view(UZ start) const {
        return view(start, count);
    }

    inline constexpr auto operator <=>(const StringView& other) const {
        UZ min_length = min(count, other.count);
        for (UZ i = 0; i < min_length; ++i) {
            if (data[i] < other[i]) return -1;
            if (data[i] > other[i]) return 1;
        }
        return 0;
    }

    inline bool operator ==(const StringView rhs) const {
        if (count != rhs.count) return false;

        for (UZ i = 0; i < count; i++) {
            if (data[i] != rhs.data[i]) return false;
        }

        return true;
    }

    bool operator ==(const String& string) const;

    inline bool operator!=(const String& string) const {
        return !(*this == string);
    }

    inline const char& operator [](UZ idx) const {
        OK_ASSERT(idx < count);
        return data[idx];
    }

    const char* data;
    UZ count;
};

inline namespace literals {
constexpr StringView operator ""_sv(const char* cstr, UZ len) {
    return StringView{cstr, len};
}
};

struct String {
    static constexpr char NULL_CHAR = '\0';
    static constexpr UZ DEFAULT_CAPACITY = 7;

    static String alloc(Allocator* a, UZ capacity = DEFAULT_CAPACITY);

    static String alloc(Allocator* a, const char* data, UZ data_len);
    static String alloc(Allocator* a, const char* data);

    static String from(List<U8>);
    static String from(List<char>);

    static String format(Allocator* a, const char* fmt, ...) OK_ATTRIBUTE_PRINTF(2, 3);

    void append(StringView);
    void append(String);

    void format_append(const char*, ...) OK_ATTRIBUTE_PRINTF(2, 3);

    bool starts_with(StringView);

    inline const char* cstr() const {
        return reinterpret_cast<const char*>(data.items);
    }

    inline StringView view(UZ start, UZ end) const {
        OK_ASSERT(start < count());
        OK_ASSERT(end >= start);

        return StringView{data.items + start, end - start};
    }

    inline StringView view(UZ start) const {
        return view(start, count());
    }

    inline StringView view() const {
        return view(0, count());
    }

    inline String copy(Allocator* a) const {
        String s;
        s.data = data.copy(a);
        return s;
    }

    inline void push(char character) {
        data.push(NULL_CHAR);
        data[data.count - 2] = character;
    }

    inline void reserve(UZ chars) {
        UZ available_size = count();

        if (available_size >= chars) return;

        data.reserve(chars + 1);
    }

    inline UZ count() const {
        OK_ASSERT(data.count != 0);
        return data.count - 1;
    }

    inline bool operator ==(const String& other) const {
        return view() == other.view();
    }

    inline bool operator ==(const StringView& other) const {
        return view() == other;
    }

    inline bool operator !=(const String& other) const {
        return !(*this == other);
    }

    inline bool operator !=(const StringView& other) const {
        return !(*this == other);
    }

    inline char& operator [](UZ idx) {
        OK_ASSERT(idx < count());

        return data.items[idx];
    }

    inline const char& operator [](UZ idx) const {
        OK_ASSERT(idx < count());

        return data.items[idx];
    }

    List<char> data;
};

template <template <typename> class Self, typename T>
struct OptionalBase {
    const T& or_else(const T& other) const {
        auto* self = self_cast();

        if (self->has_value()) return self->get_unchecked();
        return other;
    }

    T& or_else(T& other) {
        auto* self = self_cast();

        if (self->has_value(this)) return self->get_unchecked();
        return other;
    }

    Self<T>* self_cast() {
        return static_cast<Self<T>*>(this);
    }

    const Self<T>* self_cast() const {
        return static_cast<const Self<T>*>(this);
    }

    explicit operator bool() const {
        return self_cast()->has_value();
    }
};

template <typename T>
struct Optional : public OptionalBase<Optional, T> {
    Optional() : _has_value{false} {}

    Optional(T value) : _has_value{true}, value{value} {}

    inline bool has_value() const {
        return _has_value;
    }

    inline T& get_unchecked() {
        return value;
    }

    inline T& get() {
        OK_ASSERT(has_value());
        return get_unchecked();
    }

    inline const T& get_unchecked() const {
        return value;
    }

    inline const T& get() const {
        OK_ASSERT(has_value());
        return get_unchecked();
    }


    bool _has_value;
    T value;

    static const Optional<T> NONE;
};

template <typename T>
struct Optional<T*> : public OptionalBase<Optional, T*> {
    Optional() : value{nullptr} {}

    Optional(T* value) : value{value} {}

    inline bool has_value() const {
        return value != nullptr;
    }

    inline T& get_unchecked() {
        return *value;
    }

    inline T& get() {
        OK_ASSERT(has_value());
        return get_unchecked();
    }

    inline const T& get_unchecked() const {
        return *value;
    }

    inline const T& get() const {
        OK_ASSERT(has_value());
        return get_unchecked();
    }

    template <typename Base>
    inline Optional<Base*> upcast() {
        return Optional<Base*>{static_cast<Base*>(value)};
    }

    template <typename Base>
    inline Optional<const Base*> upcast() const {
        return Optional<const Base*>{static_cast<const Base*>(value)};
    }

    T* value;

    static const Optional<T> NONE;
};

static_assert(sizeof(Optional<int*>) == sizeof(int*));

template <typename A, typename B>
struct Pair {
    A a;
    B b;
};

namespace hash {
U64 fnv1(StringView);
};

template <typename T>
struct Hash {
    static U64 hash(const T& value) {
        return value.ok_hash_value();
    }
};

template <typename T>
struct HashPtr {
    HashPtr(const T* v) : value{v} {}
    HashPtr(T* v) : value{v} {}

    inline bool operator ==(const T* ptr) const {
        return *value == *ptr;
    }

    const T* value;
};

// Default hash implementations
template <typename T>
struct Hash<HashPtr<T>> {
    static U64 hash(const HashPtr<T>& ptr) {
        return Hash<T>::hash(*ptr.value);
    }
};

template <>
struct Hash<U32> {
    static U64 hash(const U32& val) {
        return val;
    }
};

template <>
struct Hash<UZ> {
    static U64 hash(const UZ& val) {
        return val;
    }
};

template <>
struct Hash<StringView> {
    static U64 hash(StringView sv) {
        return ::ok::hash::fnv1(sv);
    }
};

template <>
struct Hash<String> {
    static U64 hash(String string) {
        return ::ok::hash::fnv1(string.view());
    }
};

template <typename T>
bool operator ==(const HashPtr<T>& lhs, const HashPtr<T>& rhs);

#define OK_TAB_META_OCCUPIED 0x01
#define OK_TAB_IS_OCCUPIED(meta) ((meta) & OK_TAB_META_OCCUPIED)
#define OK_TAB_IS_FREE(meta) (!OK_TAB_IS_OCCUPIED((meta)))

#define OK_TABLE_GROWTH_FACTOR(x) (((x) + 1) * 3)

#define OK_TABLE_FOREACH(tab, key, value, code) do { \
    for (UZ _tab_i = 0; _tab_i < (tab).capacity; _tab_i++) {\
    if (OK_TAB_IS_FREE((tab).meta[_tab_i])) continue; \
    auto key = (tab).keys[_tab_i]; \
    auto value = (tab).values[_tab_i]; \
    code; \
    }\
    } while (0)

template <typename TKey, typename TValue>
struct Table {
    using Meta = U8;

    static Table<TKey, TValue> alloc(Allocator* a, UZ capacity = Table::DEFAULT_CAPACITY);

    void put(const TKey& key, const TValue& value);

    Optional<TValue> get(const TKey& key);
    template <typename K>
    Optional<TValue> get(const K& key);

    bool has(const TKey& key) const;
    template <typename K>
    bool has(const K& key) const;


    static constexpr UZ DEFAULT_CAPACITY = 47;

    inline U8 load_percentage() const {
        return (U8)((double)(count * 100) / (double)capacity);
    }

    TKey* keys;
    TValue* values;
    U8* meta;
    UZ count;
    UZ capacity;
    Allocator* allocator;
};

#define OK_SET_GROWTH_FACTOR OK_TABLE_GROWTH_FACTOR

template <typename T>
struct Set {
    using Meta = U8;

    static constexpr UZ DEFAULT_CAPACITY = 47;

    static Set<T> alloc(Allocator* a, UZ capacity = DEFAULT_CAPACITY);

    void put(const T& elem);
    bool has(const T& elem) const;

    inline U8 load_percentage() const {
        return (U8)(100.0 * (double)count / (double)capacity);
    }

    Allocator* allocator;
    UZ capacity;
    UZ count;
    T* values;
    U8* meta;
};

// SUBPROCESS API
struct Command {
    enum class ExecError {
        TOO_BIG,
        ACCESS_DENIED,
        PROCESS_LIMIT_EXCEEDED,
        INVALID_EXECUTABLE,
        IO,
        LOOP,
        TOO_MANY_FILES,
        EXECUTABLE_NOT_FOUND,
        KERNEL_OUT_OF_MEMORY,
        INVALID_PATH,
        BUSY,
        TERMINATED_BY_SIGNAL,
        STOPPED,
        PROCESS_OPEN_FILE_LIMIT_REACHED,
        SYSTEM_OPEN_FILE_LIMIT_REACHED,
        OUT_OF_SPACE,
    };

    static Command alloc(Allocator* allocator, const char* name) {
        Command cmd;
        cmd.allocator = allocator;
        cmd.name = name;
        cmd.args = List<char*>::alloc(allocator);
        cmd.envs = List<char*>::alloc(allocator);

        char* name_copy = allocator->strdup(name);
        cmd.args.push(name_copy);

        return cmd;
    }

    Command& arg(const char* arg) {
        if (arg != nullptr) {
            char* arg_copy = allocator->strdup(arg);
            args.push(arg_copy);
        } else {
            args.push(nullptr);
        }

        return *this;
    }

    Command& env(const char* env) {
        if (env != nullptr) {
            char* env_copy = allocator->strdup(env);
            envs.push(env_copy);
        } else {
            envs.push(nullptr);
        }

        return *this;
    }

    Command& set_stdin(StringView data) {
        Slice<U8> data_as_slice{(U8*)data.data, data.count};
        stdin_data = data_as_slice.copy(allocator);
        return *this;
    }


    Command& set_stdin(Slice<U8> data) {
        stdin_data = data.copy(allocator);
        return *this;
    }

    Optional<ExecError> exec() {
#if OK_UNIX
        Optional<ExecError> exec_err{};
        pid_t child_pid;

        posix_spawn_file_actions_t actions{};
        posix_spawnattr_t attributes{};

        int stdin_fds[2];
        UZ stdin_write_count;

        int spawn_ret;
        int child_status;
        pid_t wait_ret;

        arg(nullptr);
        env(nullptr);

        if (pipe(stdin_fds) < -1) {
            switch (errno) {
            case EMFILE: exec_err = ExecError::PROCESS_OPEN_FILE_LIMIT_REACHED; goto cleanup;
            case ENFILE: exec_err = ExecError::SYSTEM_OPEN_FILE_LIMIT_REACHED; goto cleanup;
            case EFAULT: OK_UNREACHABLE();
            default: OK_PANIC_FMT("unhandled error %d", errno);
            }
        }

        OK_ASSERT(posix_spawn_file_actions_init(&actions) == 0);

        // Close the write end
        OK_ASSERT(posix_spawn_file_actions_addclose(&actions, stdin_fds[1]) == 0);
        // Map the read end to stdin
        OK_ASSERT(posix_spawn_file_actions_adddup2(&actions, stdin_fds[0], STDIN_FILENO) == 0);

        spawn_ret = posix_spawnp(&child_pid, name, &actions, &attributes, args.items, envs.items);

        if (spawn_ret != 0) {
            switch (spawn_ret) {
            case E2BIG:    exec_err = ExecError::TOO_BIG; goto cleanup;

            case EPERM:
            case EACCES:   exec_err = ExecError::ACCESS_DENIED; goto cleanup;

            case EAGAIN:   exec_err = ExecError::PROCESS_LIMIT_EXCEEDED; goto cleanup;

            case ENOEXEC:
            case ELIBBAD:
            case EISDIR:
            case EINVAL:   exec_err = ExecError::INVALID_EXECUTABLE; goto cleanup;

            case EIO:      exec_err = ExecError::IO; goto cleanup;
            case ELOOP:    exec_err = ExecError::LOOP; goto cleanup;
            case ENFILE:   exec_err = ExecError::TOO_MANY_FILES; goto cleanup;
            case ENOENT:   exec_err = ExecError::EXECUTABLE_NOT_FOUND; goto cleanup;
            case ENOMEM:   exec_err = ExecError::KERNEL_OUT_OF_MEMORY; goto cleanup;
            case ENOTDIR:  exec_err = ExecError::INVALID_PATH; goto cleanup;
            case ETXTBSY:  exec_err = ExecError::BUSY; goto cleanup;
            case EFAULT:   OK_UNREACHABLE();
            default:       OK_PANIC_FMT("unhandled error %d", spawn_ret);
            }
        }

        stdin_write_count = 0;
        while (stdin_write_count < stdin_data.count) {
            U8* data = stdin_data.items + stdin_write_count;
            UZ data_count = stdin_data.count - stdin_write_count;

            SZ write_count = write(stdin_fds[1], data, data_count);
            if (write_count < 0) {
                switch (errno) {
                case EFBIG:  exec_err = ExecError::TOO_BIG; goto cleanup;
                case EIO:    exec_err = ExecError::IO; goto cleanup;
                case ENOSPC: exec_err = ExecError::OUT_OF_SPACE; goto cleanup;
                case EPERM:  exec_err = ExecError::ACCESS_DENIED; goto cleanup;
                case EPIPE:
                case EBADFD:
                case EDESTADDRREQ:
                case EAGAIN:
                case EDQUOT:
                case EINVAL:
                case EINTR:
                    OK_UNREACHABLE();
                default: OK_PANIC_FMT("unhandled error %d", errno);
                }
            }

            stdin_write_count += write_count;
        }

        OK_ASSERT(close(stdin_fds[1]) == 0);

        wait_ret = waitpid(child_pid, &child_status, 0);
        if (wait_ret == (pid_t)-1) OK_UNREACHABLE();

        if (WIFEXITED(child_status)) {
            exit_code = WEXITSTATUS(child_status);
            goto cleanup;
        }

        if (WIFSIGNALED(child_status)) {
            term_signal_num = WTERMSIG(child_status);
            exec_err = ExecError::TERMINATED_BY_SIGNAL;
            goto cleanup;
        }

        if (WIFSTOPPED(child_status)) {
            stop_signal_num = WSTOPSIG(child_status);
            exec_err = ExecError::STOPPED;
            goto cleanup;
        }

cleanup:
        posix_spawn_file_actions_destroy(&actions);

        return exec_err;
#else
        OK_TODO();
#endif // OS guard
    }

    Allocator* allocator;
    const char* name;
    List<char*> args;
    List<char*> envs;

    int exit_code;
    int term_signal_num;
    int stop_signal_num;

    Slice<U8> stdin_data{};
};

// HASH IMPLEMENTATION
template <typename T>
bool operator ==(const HashPtr<T>& lhs, const HashPtr<T>& rhs) {
    return *lhs.value == *rhs.value;
}

// OPTIONAL IMPLEMENTATION
template <typename T>
const Optional<T> Optional<T>::NONE = Optional<T>{};

template <typename T>
const Optional<T> Optional<T*>::NONE = Optional<T*>{};

// ARRAY BASE IMPLEMENTATION
template <typename Self, typename T>
Slice<T> ArrayBase<Self, T>::slice(UZ start, UZ end) {
    auto* self = self_cast();
    UZ count = self->get_count();

    OK_ASSERT(end >= start);
    OK_ASSERT(end <= count);

    return Slice<T>{self->get_items(), end - start};
}

template <typename Self, typename T>
Slice<T> ArrayBase<Self, T>::slice(UZ start) {
    auto* self = self_cast();
    return slice(start, self->get_count());
}

template <typename Self, typename T>
Slice<T> ArrayBase<Self, T>::slice() {
    auto* self = self_cast();
    return slice(0, self->get_count());
}

template <typename Self, typename T>
Slice<const T> ArrayBase<Self, T>::slice(UZ start, UZ end) const {
    auto* self = self_cast();
    UZ count = self->get_count();

    OK_ASSERT(end >= start);
    OK_ASSERT(end <= count);

    return Slice<const T>{self->get_items(), end - start};
}

template <typename Self, typename T>
Slice<const T> ArrayBase<Self, T>::slice(UZ start) const {
    auto* self = self_cast();
    return slice(start, self->get_count());
}

template <typename Self, typename T>
Slice<const T> ArrayBase<Self, T>::slice() const {
    auto* self = self_cast();
    return slice(0, self->get_count());
}

// LIST IMPLEMENTATION
template <typename T>
List<T> List<T>::alloc(Allocator* a, UZ cap) {
    List<T> list{};
    list.items = a->alloc<T>(cap);
    list.count = 0;
    list.capacity = cap;
    list.allocator = a;

    return list;
}

template <typename T>
void List<T>::push(const T& item) {
    if (count >= capacity) {
        UZ new_capacity = OK_LIST_GROW_FACTOR(capacity);
        items = allocator->resize<T>(items, capacity, new_capacity);
        capacity = new_capacity;
    }

    items[count++] = item;
}

template <typename T>
inline void List<T>::remove_at(UZ idx) {
    OK_ASSERT(idx < count);

    for (UZ i = idx; i < count - 1; i++) {
        items[i] = items[i + 1];
    }

    count--;
}

template <typename T>
inline List<T> List<T>::copy(Allocator* a, UZ start, UZ end) const {
    OK_ASSERT(end >= start);

    auto res = List<T>::alloc(a, end - start);
    for (UZ i = start; i < end; i++) {
        res.push(items[i]);
    }
    return res;
}

template <typename T>
inline void List<T>::extend(List<T> other) {
    reserve(capacity + other.capacity);

    for (UZ i = 0; i < other.count; i++) {
        push(other.items[i]);
    }
}

template <typename T>
inline void List<T>::reserve(UZ new_cap) {
    if (new_cap <= capacity) {
        return;
    }

    items = allocator->resize<T>(items, capacity, new_cap);
    capacity = new_cap;
}

// TABLE IMPLEMENTATION
template <typename K, typename V>
Table<K, V> Table<K, V>::alloc(Allocator* a, UZ capacity) {
    Table<K, V> tab{};

    tab.keys = a->alloc<K>(capacity);
    tab.values = a->alloc<V>(capacity);
    tab.meta = a->alloc<Meta>(capacity);
    tab.count = 0;
    tab.capacity = capacity;
    tab.allocator = a;

    return tab;
}

template <typename K, typename V>
void Table<K, V>::put(const K& key, const V& value) {
    if (load_percentage() >= 70) {
        auto new_table = Table<K, V>::alloc(allocator, OK_TABLE_GROWTH_FACTOR(capacity));

        for (UZ i = 0; i < capacity; i++) {
            if (OK_TAB_IS_OCCUPIED(meta[i])) {
                new_table.put(keys[i], values[i]);
            }
        }

        *this = new_table;
    }

    U64 idx = Hash<K>::hash(key) % capacity;

    while (true) {
        if (OK_TAB_IS_FREE(meta[idx])) {
            meta[idx] |= OK_TAB_META_OCCUPIED;
            values[idx] = value;
            keys[idx] = key;
            count++;
            return;
        }

        if (keys[idx] == key) {
            values[idx] = value;
            keys[idx] = key;
            return;
        }

        idx = (idx + 1) % capacity;
    }
}

template <typename K, typename V>
Optional<V> Table<K, V>::get(const K& key) {
    U64 idx = Hash<K>::hash(key) % capacity;
    U64 initial_idx = idx;

    do {
        if (OK_TAB_IS_OCCUPIED(meta[idx]) && keys[idx] == key) {
            return values[idx];
        }

        idx = (idx + 1) % capacity;
    } while (idx != initial_idx);

    return {};
}

template <typename TKey, typename TValue>
template <typename K>
Optional<TValue> Table<TKey, TValue>::get(const K& key) {
    U64 idx = Hash<K>::hash(key) % capacity;
    U64 initial_idx = idx;

    do {
        if (OK_TAB_IS_OCCUPIED(meta[idx]) && keys[idx] == key) {
            return values[idx];
        }

        idx = (idx + 1) % capacity;
    } while (idx != initial_idx);

    return {};
}

template <typename K, typename V>
bool Table<K, V>::has(const K& key) const {
    U64 idx = Hash<K>::hash(key) % capacity;
    U64 initial_idx = idx;

    do {
        if (OK_TAB_IS_OCCUPIED(meta[idx]) && keys[idx] == key) {
            return true;
        }

        idx = (idx + 1) % capacity;
    } while (idx != initial_idx);

    return false;
}


template <typename TKey, typename TValue>
template <typename K>
bool Table<TKey, TValue>::has(const K& key) const {
    U64 idx = Hash<K>::hash(key) % capacity;
    U64 initial_idx = idx;

    do {
        if (OK_TAB_IS_OCCUPIED(meta[idx]) && keys[idx] == key) {
            return true;
        }

        idx = (idx + 1) % capacity;
    } while (idx != initial_idx);

    return false;
}

// SET IMPLEMENTATION
template <typename T>
Set<T> Set<T>::alloc(Allocator* a, UZ capacity) {
    Set<T> set;
    set.allocator = a;
    set.count = 0;
    set.capacity = capacity;
    set.values = a->alloc<T>(capacity);
    set.meta = a->alloc<Meta>(capacity);
    return set;
}

template <typename T>
void Set<T>::put(const T& elem) {
    if (load_percentage() >= 70) {
        auto new_set = Set<T>::alloc(allocator, OK_SET_GROWTH_FACTOR(capacity));

        for (UZ i = 0; i < capacity; i++) {
            if (OK_TAB_IS_FREE(meta[i])) {
                continue;
            }

            new_set.put(values[i]);
        }

        *this = new_set;
    }

    U64 hash = Hash<T>::hash(elem);

    while (true) {
        if (OK_TAB_IS_FREE(meta[hash])) {
            meta[hash] |= OK_TAB_META_OCCUPIED;
            values[hash] = elem;
            count++;
            return;
        }

        if (values[hash] == elem) {
            values[hash] = elem;
            return;
        }

        hash = (hash + 1) % capacity;
    }
}

template <typename T>
bool Set<T>::has(const T& elem) const {
    U64 hash = Hash<T>::hash(elem);
    U64 idx = hash;

    do {
        if (OK_TAB_IS_OCCUPIED(meta[idx]) && values[idx] == elem) {
            return true;
        }

        idx = (idx + 1) % capacity;
    } while (idx != hash);

    return false;
}

// filesystem API
struct File {
    enum class OpenError {
        ACCESS_DENIED,
        INVALID_PATH,
        IS_DIRECTORY,
        TOO_MANY_SYMLINKS,
        PROCESS_OPEN_FILES_LIMIT_REACHED,
        SYSTEM_OPEN_FILES_LIMIT_REACHED,
        PATH_TOO_LONG,
        KERNEL_OUT_OF_MEMORY,
        OUT_OF_SPACE,
        IS_SOCKET,
        FILE_TOO_BIG,
        READONLY_FILE,
    };

    enum class ReadError {
        IO_ERROR,
    };

    static Optional<OpenError> open(File* out, const char* path);

    void seek_start() const;
    off_t seek_end() const;

    Optional<ReadError> read(U8* buf, UZ count, UZ* n_read);

    Optional<ReadError> read_full(Allocator* a, List<U8>* out);

    UZ size() const;

    int fd;
    const char* path;
};

// procedures
void println(const char*);
void println(StringView);
void println(String);

void eprintln(const char*);
void eprintln(String);
void eprintln(StringView);

String to_string(Allocator*, S32);
String to_string(Allocator*, U32);
String to_string(Allocator*, S64);
String to_string(Allocator*, U64);

bool parse_int64(StringView, S64*);

#ifdef OK_IMPLEMENTATION

FixedBufferAllocator _temp_allocator_impl{};
Allocator* temp_allocator = &_temp_allocator_impl;

ArenaAllocator _static_allocator_impl{};
Allocator* static_allocator = &_static_allocator_impl;

static void _init_region(ArenaAllocator::Region* region, UZ size) {
    region->data = (U8*)OK_ALLOC_PAGE(size);
    OK_ASSERT(region->data != (void*)-1);
    region->size = size;
    region->off = 0;
    region->next = nullptr;
}

// ALLOCATORS IMPLEMENTATION
void* FixedBufferAllocator::raw_alloc(UZ size) {
    size = align_up(size, sizeof(void*));

    if (buffer == nullptr) {
        buffer_size = max(size, OK_PAGE_SIZE * FixedBufferAllocator::DEFAULT_PAGE_COUNT);
        buffer_size = align_up(buffer_size, OK_PAGE_ALIGN);
        buffer = OK_ALLOC_PAGE(buffer_size);
        buffer_off = 0;
    }

    if (size > buffer_size) return nullptr;

    if (buffer_size - buffer_off < size) {
        buffer_off = size;
        return buffer;
    }

    auto* ptr = (U8*)buffer + buffer_off;
    buffer_off += size;
    return (void*)ptr;
}

void FixedBufferAllocator::raw_dealloc(void* ptr, UZ size) {
    if (ptr == (void*)((U8*)buffer + buffer_off)) buffer_off -= size;
}

ArenaAllocator::Region* ArenaAllocator::alloc_region(UZ region_size) {
    using Region = ArenaAllocator::Region;

    region_size = align_up(region_size, OK_PAGE_ALIGN);

    Region* current_region = region_pool;

    while (current_region) {
        if (current_region->size - current_region->off >= sizeof(Region)) {
            auto* region = (Region*)((U8*)current_region->data + current_region->off);

            _init_region(region, region_size);

            current_region->off += align_up(sizeof(Region), sizeof(void*));

            return region;
        }

        current_region = current_region->next;
    }

    current_region = (Region*)OK_ALLOC_SMOL(sizeof(Region));

    current_region->data = OK_ALLOC_PAGE(OK_PAGE_SIZE);
    current_region->size = OK_PAGE_SIZE;
    current_region->off = align_up(sizeof(Region), sizeof(void*));
    current_region->next = region_pool;

    this->region_pool = current_region;

    auto* resulting_region = (Region*)current_region->data;
    _init_region(resulting_region, region_size);
    return resulting_region;
}

void* ArenaAllocator::raw_alloc(UZ size) {
    ArenaAllocator::Region* region_head = this->head;

    size = align_up(size, sizeof(void*));

    while (region_head) {
        if (region_head->size - region_head->off >= size) {
            void* ptr = (void*)((U8*)region_head->data + region_head->off);
            region_head->off += size;
            return ptr;
        }

        region_head = region_head->next;
    }

    UZ region_size = align_up(size, OK_PAGE_ALIGN);
    region_head = alloc_region(region_size);
    region_head->next = this->head;
    this->head = region_head;

    void* ptr = (void*)((U8*)region_head->data + region_head->off);
    region_head->off += size;
    return ptr;
}

void ArenaAllocator::raw_dealloc(void* ptr, UZ size) {
    OK_UNUSED(ptr);
    OK_UNUSED(size);
}

void* ArenaAllocator::raw_resize(void* old_ptr, UZ old_size, UZ new_size) {
    auto* new_ptr = raw_alloc(new_size);
    memcpy(new_ptr, old_ptr, old_size);
    return new_ptr;
}

// STRING IMPLEMENTATION

String String::alloc(Allocator* a, UZ capacity) {
    String s;
    s.data = List<char>::alloc(a, capacity + 1);
    s.data.push(NULL_CHAR);

    return s;
}

String String::alloc(Allocator* a, const char* data, UZ data_len) {
    auto s = String::alloc(a, data_len);

    for (UZ i = 0; i < data_len; i++) s.push((char)data[i]);

    return s;
}

String String::alloc(Allocator* a, const char* data) {
    UZ data_len = strlen((const char*)data);
    return String::alloc(a, data, data_len);
}

String String::from(List<char> chars) {
    String s;
    s.data = chars;
    s.data.push('\0');
    return s;
}

String String::from(List<U8> bytes) {
    List<char> chars = bytes.cast<char>();
    return String::from(chars);
}

String String::format(Allocator* a, const char* fmt, ...) {
    va_list sprintf_args;

    int buf_size;
    va_start(sprintf_args, fmt);
    {
        char* sprintf_buf = nullptr;
        buf_size = vsnprintf(sprintf_buf, 0, fmt, sprintf_args);
        OK_ASSERT(buf_size != -1);

        buf_size += 1;
    }
    va_end(sprintf_args);

    auto buf = String::alloc(a, buf_size);

    va_start(sprintf_args, fmt);
    {
        int bytes_written = vsnprintf((char*)buf.data.items, buf_size, fmt, sprintf_args);
        OK_ASSERT(bytes_written != -1);
    }
    va_end(sprintf_args);

    buf.data.count = buf_size;

    return buf;
}

void String::append(StringView sv) {
    for (UZ i = 0; i < sv.count; ++i) push(sv.data[i]);
}

void String::append(String str) {
    UZ str_count = str.count();
    for (UZ i = 0; i < str_count; ++i) push(str.data.items[i]);
}

void String::format_append(const char* fmt, ...) {
    va_list sprintf_args;

    int required_buf_size;
    va_start(sprintf_args, fmt);
    {
        char* sprintf_buf = nullptr;
        required_buf_size = vsnprintf(sprintf_buf, 0, fmt, sprintf_args);
        OK_ASSERT(required_buf_size != -1);
    }
    va_end(sprintf_args);

    UZ bytes_needed = count() + required_buf_size;
    reserve(bytes_needed);

    va_start(sprintf_args, fmt);
    {
        char* buf = (char*)data.items + count();
        int bytes_written = vsnprintf(buf, required_buf_size + 1, fmt, sprintf_args);
        OK_ASSERT(bytes_written != -1);
    }
    va_end(sprintf_args);

    data.count += required_buf_size;
}

bool String::starts_with(StringView prefix) {
    if (count() < prefix.count) return false;

    for (UZ i = 0; i < prefix.count; i++) {
        if (data.items[i] != prefix.data[i]) return false;
    }

    return true;
}

// STRING VIEW IMPLEMENTATION
bool StringView::operator ==(const String& string) const {
    return *this == string.view();
}

String StringView::to_string(Allocator* a) const {
    return String::alloc(a, data, count);
}

// FILESYSTEM API IMPLEMENTATION
Optional<File::OpenError> File::open(File* out, const char* path) {
#if OK_UNIX
    int fd = ::open(path, O_RDWR);
    int error = errno;
#elif OK_WINDOWS
    int fd;
    errno_t error = ::_sopen_s(&fd, path, _O_RDWR, _SH_DENYNO, 0);
#endif // OK_UNIX

    if (fd < 0) {
        switch (error) {
        case EACCES:       return OpenError::ACCESS_DENIED;
        case EINVAL:       return OpenError::INVALID_PATH;
        case EISDIR:       return OpenError::IS_DIRECTORY;
        case ELOOP:        return OpenError::TOO_MANY_SYMLINKS;
        case EMFILE:       return OpenError::PROCESS_OPEN_FILES_LIMIT_REACHED;
        case ENFILE:       return OpenError::SYSTEM_OPEN_FILES_LIMIT_REACHED;
        case ENAMETOOLONG: return OpenError::PATH_TOO_LONG;
        case ENOMEM:       return OpenError::KERNEL_OUT_OF_MEMORY;
        case ENOSPC:       return OpenError::OUT_OF_SPACE;
        case ENXIO:        return OpenError::IS_SOCKET;
        case EOVERFLOW:    return OpenError::FILE_TOO_BIG;
        case EROFS:        return OpenError::READONLY_FILE;
        case EFAULT:       OK_PANIC_FMT("Parameter 'path' (%p) is not mapped to the current process", (void*)path);
        default:           OK_UNREACHABLE();
        }
    }

    out->fd = fd;
    out->path = path;

    return {};
}

static inline off_t _lseek(int fd, off_t offset, int whence) {
#if OK_UNIX
    return ::lseek(fd, offset, whence);
#elif OK_WINDOWS
    return ::_lseek(fd, offset, whence);
#endif // OK_UNIX
}

void File::seek_start() const {
    off_t seek_res = _lseek(fd, 0, SEEK_SET);
    OK_ASSERT(seek_res != (off_t)-1);
}

off_t File::seek_end() const {
    off_t seek_res = _lseek(fd, 0, SEEK_END);
    OK_ASSERT(seek_res != (off_t)-1);

    return seek_res;
}

UZ File::size() const {
    off_t res = seek_end();
    seek_start();
    return res;
}

static inline S64 _read(int fd, void* buffer, UZ count) {
#if OK_UNIX
    return ::read(fd, buffer, count);
#elif OK_WINDOWS
    return ::_read(fd, buffer, (unsigned int)count);
#endif // OK_UNIX
}

Optional<File::ReadError> File::read(U8* buf, UZ count, UZ* n_read) {
    S64 r = ok::_read(fd, buf, count);

    if (r < 0) {
        switch (errno) {
        case EIO: return ReadError::IO_ERROR;
        case EFAULT: OK_PANIC_FMT("The buffer (%p) is mapped outside the current process", (void*)buf);
        default: OK_UNREACHABLE();
        }
    }

    *n_read = r;

    return {};
}

Optional<File::ReadError> File::read_full(Allocator* a, List<U8>* out) {
    UZ sz = size();
    *out = List<U8>::alloc(a, sz);

    UZ n_read;
    auto err = read(out->items, sz, &n_read);

    if (err.has_value()) return err;

    out->count = n_read;

    return {};
}

// PROCEDURES IMPLEMENTATION
String to_string(Allocator* allocator, U32 value) {
    String s = String::alloc(allocator, 10);

    U32 value_copy = value;
    int idx = 0;

    while (value_copy /= 10) idx++;

    UZ string_count = idx + 1;

    do {
        char digit = value % 10;
        value /= 10;
        s.data.items[idx--] = digit + '0';
    } while (value != 0);

    s.data.items[string_count] = String::NULL_CHAR;
    s.data.count = string_count + 1;

    return s;
}

String to_string(Allocator* allocator, S32 input_value) {
    String s = String::alloc(allocator, 10);

    U32 value = input_value;
    int idx = 0;

    if (input_value < 0) {
        s.push('-');
        value = (U32)-input_value;
        idx = 1;
    }

    U32 value_copy = value;

    while (value_copy /= 10) idx++;

    UZ string_count = idx + 1;

    do {
        char digit = value % 10;
        value /= 10;
        s.data.items[idx--] = digit + '0';
    } while (value != 0);

    s.data.items[string_count] = String::NULL_CHAR;
    s.data.count = string_count + 1;

    return s;
}

String to_string(Allocator* allocator, U64 value) {
    String s = String::alloc(allocator, 10);

    U64 value_copy = value;
    int idx = 0;

    while (value_copy /= 10) idx++;

    UZ string_count = idx + 1;

    do {
        char digit = value % 10;
        value /= 10;
        s.data.items[idx--] = digit + '0';
    } while (value != 0);

    s.data.items[string_count] = String::NULL_CHAR;
    s.data.count = string_count + 1;

    return s;
}

String to_string(Allocator* allocator, S64 input_value) {
    String s = String::alloc(allocator, 10);

    U64 value = input_value;
    int idx = 0;

    if (input_value < 0) {
        s.push('-');
        value = -input_value;
        idx = 1;
    }

    U64 value_copy = value;

    while (value_copy /= 10) idx++;

    UZ string_count = idx + 1;

    do {
        char digit = value % 10;
        value /= 10;
        s.data.items[idx--] = digit + '0';
    } while (value != 0);

    s.data.items[string_count] = String::NULL_CHAR;
    s.data.count = string_count + 1;

    return s;
}

bool parse_int64(StringView source, S64* out) {
    if (source.count == 0) return false;

    S64 result = 0;
    UZ coef = 1;

    for (intptr_t i = source.count - 1; i > 0; --i) {
        auto c = source[i];
        if (!is_digit(c)) return false;

        U8 digit = c - '0';
        result += digit * coef;
        coef *= 10;
    }

    if (is_digit(source[0])) result += (source[0] - '0') * coef;
    else if (source[0] == '-') result = -result;
    else return false;

    *out = result;

    return true;
}

void println(const char* msg) {
    ::printf("%s\n", msg);
}

void println(StringView sv) {
    ::printf(OK_SV_FMT "\n", OK_SV_ARG(sv));
}

void println(String string) {
    ::printf("%s\n", string.cstr());
}

void eprintln(const char* msg) {
    ::fprintf(stderr, "%s\n", msg);
}

void eprintln(StringView sv) {
    ::fprintf(stderr, OK_SV_FMT "\n", OK_SV_ARG(sv));
}

void eprintln(String string) {
    ::fprintf(stderr, "%s\n", string.cstr());
}

bool is_whitespace(char c) {
    return c == '\t' ||
           c == '\n' ||
           c == '\v' ||
           c == '\r' ||
           c == ' ';
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// HASHES IMPLEMENTATION
namespace hash {
U64 fnv1(StringView sv) {
    constexpr const U64 fnv_offset_basis = 0xCBF29CE484222325;
    constexpr const U64 fnv_prime = 0x100000001B3;

    U64 hash = fnv_offset_basis;

    for (UZ i = 0; i < sv.count; ++i) {
        hash *= fnv_prime;

        U8 byte = sv.data[i];
        hash ^= (U64)byte;
    }

    return hash;
}
};

#endif
};

#endif // OK_H_
