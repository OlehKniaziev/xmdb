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

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdarg>
#include <cerrno>
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

#if defined(__unix__) || defined(__unix) || defined(__APPLE__)

#define OK_UNIX 1
#define OK_WINDOWS 0

#include <sys/mman.h>
#include <unistd.h>

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

#ifdef __GNUC__
#define ATTRIBUTE_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
#else
#define ATTRIBUTE_PRINTF(fmt, args)
#endif // __GNUC__

namespace ok {
struct Allocator {
    // required methods
    virtual void* raw_alloc(size_t size) = 0;
    virtual void raw_dealloc(void* ptr, size_t size) = 0;

    // optional methods
    virtual void* raw_resize(void* ptr, size_t old_size, size_t new_size) {
        void* new_ptr = raw_alloc(new_size);
        memcpy(new_ptr, ptr, old_size);
        raw_dealloc(ptr, old_size);
        return new_ptr;
    }

    // provided methods
    template <typename T>
    inline T* alloc(size_t size = 1) {
        return (T*)raw_alloc(sizeof(T) * size);
    }

    template <typename T>
    inline void dealloc(T* ptr, size_t count) {
        return raw_dealloc((void*)ptr, count * sizeof(T));
    }

    template <typename T>
    inline T* resize(T* ptr, size_t old_size, size_t new_size) {
        T* new_ptr = alloc<T>(new_size);
        memcpy((void*)new_ptr, ptr, old_size * sizeof(T));
        dealloc<T>(ptr, old_size);
        return new_ptr;
    }
};

extern Allocator* temp_allocator;
extern Allocator* static_allocator;

struct FixedBufferAllocator : public Allocator {
    void* raw_alloc(size_t size) override;
    void raw_dealloc(void* ptr, size_t size) override;

    static constexpr size_t DEFAULT_PAGE_COUNT = 5;

    void* buffer;
    size_t buffer_size;
    size_t buffer_off;
};

static inline uintptr_t align_to(uintptr_t size, uintptr_t align) {
    return size + ((align - (size & (align - 1))) & (align - 1));
}

struct ArenaAllocator : public Allocator {
    struct Region {
        size_t avail() const {
            OK_ASSERT(off <= size);
            return size - off;
        }

        Region* next;
        void* data;
        size_t size;
        size_t off;
    };

    void* raw_alloc(size_t size) override;
    void raw_dealloc(void* ptr, size_t size) override;
    void* raw_resize(void* ptr, size_t old_size, size_t new_size) override;

    Region* alloc_region(size_t size);

    inline size_t avail() const {
        size_t result = 0;
        for (Region* r = head; r != nullptr; r = r->next) result += r->avail();
        return result;
    }

    inline void reserve(size_t bytes) {
        size_t available_bytes = avail();

        if (available_bytes < bytes) {
            size_t bytes_needed = bytes - available_bytes;
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
#define OK_LIST_GROW_FACTOR(x) ((((x) + 1) * 3) >> 1)

template <typename T>
struct Slice;

template <typename T>
struct List {
    static List<T> alloc(Allocator* a, size_t cap = List::DEFAULT_CAP);

    static constexpr size_t DEFAULT_CAP = 7;

    void push(const T& item);
    void extend(List<T> other);
    void remove_at(size_t idx);

    List<T> copy(Allocator* a, size_t start, size_t end) const;

    inline List<T> copy(Allocator* a, size_t start) const {
        return copy(a, start, count);
    }

    inline List<T> copy(Allocator* a) const {
        return copy(a, 0, count);
    }

    void reserve(size_t new_cap);

    size_t find_index(const T& elem);
    template <typename F>
    size_t find_index(F pred);

    Slice<T> slice(size_t start, size_t end) const;
    Slice<T> slice(size_t start) const;
    Slice<T> slice() const;

    template <typename Dest>
    inline List<Dest> cast() {
        List<Dest> list;
        list.allocator = allocator;
        list.items = (Dest*)items;
        list.count = count;
        list.capacity = capacity;
        return list;
    }

    inline T& operator [](size_t idx) {
        OK_ASSERT(idx < count);

        return items[idx];
    }

    inline const T& operator [](size_t idx) const {
        OK_ASSERT(idx < count);

        return items[idx];
    }

    T* items;
    size_t count;
    size_t capacity;
    Allocator* allocator;
};

template <typename T>
struct Slice {
    inline Slice<T> slice(size_t start, size_t end) const {
        OK_ASSERT(end >= start);
        return Slice<T>{items + start, end - start};
    }

    inline Slice<T> slice(size_t start) const {
        return slice(start, count);
    }

    inline Slice<T> slice() const {
        return slice(0, count);
    }

    inline const T& operator [](size_t idx) const {
        OK_ASSERT(idx < count);

        return items[idx];
    }

    const T* items;
    size_t count;
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

    constexpr StringView(const char* data, size_t count) : data{data}, count{count} {}
    explicit constexpr StringView(const char* cstr) : StringView{cstr, strlen(cstr)} {}

    String to_string(Allocator* a) const;

    const char* data;
    size_t count;
};

inline bool operator ==(const StringView lhs, const StringView rhs) {
    if (lhs.count != rhs.count) return false;

    for (size_t i = 0; i < lhs.count; i++) {
        if (lhs.data[i] != rhs.data[i]) return false;
    }

    return true;
}

inline namespace literals {
constexpr StringView operator ""_sv(const char* cstr, size_t len) {
    return StringView{cstr, len};
}
};

struct String {
    static constexpr char NULL_CHAR = '\0';
    static constexpr size_t DEFAULT_CAPACITY = 7;

    static String alloc(Allocator* a, size_t capacity = DEFAULT_CAPACITY);

    static String alloc(Allocator* a, const char* data, size_t data_len);
    static String alloc(Allocator* a, const char* data);

    static String from(List<uint8_t>);
    static String from(List<char>);

    static String format(Allocator* a, const char* fmt, ...) ATTRIBUTE_PRINTF(2, 3);

    void append(StringView);
    void append(String);

    void format_append(const char*, ...) ATTRIBUTE_PRINTF(2, 3);

    bool starts_with(StringView);

    inline const char* cstr() const {
        return reinterpret_cast<const char*>(data.items);
    }

    inline StringView view(size_t start, size_t end) const {
        OK_ASSERT(start < count());
        OK_ASSERT(end >= start);

        return StringView{data.items + start, end - start};
    }

    inline StringView view(size_t start) const {
        return view(start, count());
    }

    inline StringView view() const {
        return view(0, count());
    }

    inline String copy(Allocator* a) {
        String s;
        s.data = data.copy(a);
        return s;
    }

    inline void push(char character) {
        data.push(NULL_CHAR);
        data[data.count - 2] = character;
    }

    inline void reserve(size_t chars) {
        size_t available_size = count();

        if (available_size >= chars) return;

        data.reserve(chars + 1);
    }

    inline size_t count() const {
        OK_ASSERT(data.count != 0);
        return data.count - 1;
    }

    inline bool operator ==(const String& other) {
        if (other.count() != count()) return false;

        for (size_t i = 0; i < count(); ++i) {
            if (data.items[i] != other.data.items[i]) return false;
        }

        return true;
    }

    inline char& operator [](size_t idx) {
        OK_ASSERT(idx < count());

        return data.items[idx];
    }

    inline const char& operator [](size_t idx) const {
        OK_ASSERT(idx < count());

        return data.items[idx];
    }

    List<char> data;
};

template <template <typename> class Self, typename T>
struct OptionalBase {
    const T& or_else(const T& other) const {
        auto* self = cast();

        if (self->has_value()) return self->get_unchecked();
        return other;
    }

    T& or_else(T& other) {
        auto* self = cast();

        if (self->has_value(this)) return self->get_unchecked();
        return other;
    }

    Self<T>* cast() {
        return static_cast<Self<T>*>(this);
    }

    const Self<T>* cast() const {
        return static_cast<const Self<T>*>(this);
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
struct Optional<T*> : public OptionalBase<Optional, T> {
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
uint64_t fnv1(StringView);
};

template <typename T>
struct Hash {};

template <typename T>
struct HashPtr {
    HashPtr(const T* v) : value{v} {}
    HashPtr(T* v) : value{v} {}
    const T* value;
};

// Default hash implementations
template <typename T>
struct Hash<HashPtr<T>> {
    static uint64_t hash(const HashPtr<T>& ptr) {
        return Hash<T>::hash(*ptr.value);
    }
};

template <>
struct Hash<uint32_t> {
    static uint64_t hash(const uint32_t& val) {
        return val;
    }
};

template <>
struct Hash<size_t> {
    static uint64_t hash(const size_t& val) {
        return val;
    }
};

template <>
struct Hash<StringView> {
    static uint64_t hash(StringView sv) {
        return ::ok::hash::fnv1(sv);
    }
};

template <>
struct Hash<String> {
    static uint64_t hash(String string) {
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
    for (size_t _tab_i = 0; _tab_i < (tab).capacity; _tab_i++) {\
    if (OK_TAB_IS_FREE((tab).meta[_tab_i])) continue; \
    auto key = (tab).keys[_tab_i]; \
    auto value = (tab).values[_tab_i]; \
    code; \
    }\
    } while (0)

template <typename K, typename V>
struct Table {
    using Meta = uint8_t;

    static Table<K, V> alloc(Allocator* a, size_t capacity = Table::DEFAULT_CAPACITY);

    void put(const K& key, const V& value);
    Optional<V> get(const K& key);
    bool has(const K& key);

    static constexpr size_t DEFAULT_CAPACITY = 47;

    inline uint8_t load_percentage() const {
        return (uint8_t)((double)(count * 100) / (double)capacity);
    }

    K* keys;
    V* values;
    uint8_t* meta;
    size_t count;
    size_t capacity;
    Allocator* allocator;
};

#define OK_SET_GROWTH_FACTOR OK_TABLE_GROWTH_FACTOR

template <typename T>
struct Set {
    using Meta = uint8_t;

    static constexpr size_t DEFAULT_CAPACITY = 47;

    static Set<T> alloc(Allocator* a, size_t capacity = DEFAULT_CAPACITY);

    void put(const T& elem);
    bool has(const T& elem) const;

    inline uint8_t load_percentage() const {
        return (uint8_t)(100.0 * (double)count / (double)capacity);
    }

    Allocator* allocator;
    size_t capacity;
    size_t count;
    T* values;
    uint8_t* meta;
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

// LIST IMPLEMENTATION
template <typename T>
List<T> List<T>::alloc(Allocator* a, size_t cap) {
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
        size_t new_capacity = OK_LIST_GROW_FACTOR(capacity);
        items = allocator->resize<T>(items, capacity, new_capacity);
        capacity = new_capacity;
    }

    items[count++] = item;
}

template <typename T>
inline void List<T>::remove_at(size_t idx) {
    OK_ASSERT(idx < count);

    for (size_t i = idx; i < count - 1; i++) {
        items[i] = items[i + 1];
    }

    count--;
}

template <typename T>
inline List<T> List<T>::copy(Allocator* a, size_t start, size_t end) const {
    OK_ASSERT(end >= start);

    auto res = List<T>::alloc(a, end - start);
    for (size_t i = start; i < end; i++) {
        res.push(items[i]);
    }
    return res;
}

template <typename T>
inline void List<T>::extend(List<T> other) {
    reserve(capacity + other.capacity);

    for (size_t i = 0; i < other.count; i++) {
        push(other.items[i]);
    }
}

template <typename T>
inline void List<T>::reserve(size_t new_cap) {
    if (new_cap <= capacity) {
        return;
    }

    items = allocator->resize<T>(items, capacity, new_cap);
    capacity = new_cap;
}

template <typename T>
inline size_t List<T>::find_index(const T& elem) {
    for (size_t i = 0; i < count; i++) {
        if (items[i] == elem) {
            return i;
        }
    }

    return (size_t)-1;
}

template <typename T>
template <typename F>
inline size_t List<T>::find_index(F pred) {
    for (size_t i = 0; i < count; i++) {
        if (pred(items[i])) {
            return i;
        }
    }

    return (size_t)-1;
}

template <typename T>
Slice<T> List<T>::slice(size_t start, size_t end) const {
    OK_ASSERT(end >= start);
    return Slice<T>{items + start, end - start};
}

template <typename T>
Slice<T> List<T>::slice(size_t start) const {
    return slice(start, count);
}

template <typename T>
Slice<T> List<T>::slice() const {
    return slice(0, count);
}

// TABLE IMPLEMENTATION
template <typename K, typename V>
Table<K, V> Table<K, V>::alloc(Allocator* a, size_t capacity) {
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

        for (size_t i = 0; i < capacity; i++) {
            if (OK_TAB_IS_OCCUPIED(meta[i])) {
                new_table.put(keys[i], values[i]);
            }
        }

        *this = new_table;
    }

    uint64_t idx = Hash<K>::hash(key) % capacity;

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
    uint64_t idx = Hash<K>::hash(key) % capacity;
    uint64_t initial_idx = idx;

    do {
        if (OK_TAB_IS_OCCUPIED(meta[idx]) && keys[idx] == key) {
            return values[idx];
        }

        idx = (idx + 1) % capacity;
    } while (idx != initial_idx);

    return {};
}

template <typename K, typename V>
bool Table<K, V>::has(const K& key) {
    uint64_t idx = Hash<K>::hash(key) % capacity;
    uint64_t initial_idx = idx;

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
Set<T> Set<T>::alloc(Allocator* a, size_t capacity) {
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

        for (size_t i = 0; i < capacity; i++) {
            if (OK_TAB_IS_FREE(meta[i])) {
                continue;
            }

            new_set.put(values[i]);
        }

        *this = new_set;
    }

    uint64_t hash = Hash<T>::hash(elem);

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
    uint64_t hash = Hash<T>::hash(elem);
    uint64_t idx = hash;

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

    Optional<ReadError> read(uint8_t* buf, size_t count);

    Optional<ReadError> read_full(Allocator* a, List<uint8_t>* out);

    size_t size() const;

    int fd;
    const char* path;
};

// procedures
template <typename T>
const T& max(const T& a) {
    return a;
}

template <typename T, typename... Rest>
const T& max(const T& x, const Rest&... xs) {
    const T& xs_max = max(xs...);
    return x > xs_max ? x : xs_max;
}

template <typename T>
const T& min(const T& a) {
    return a;
}

template <typename T, typename... Rest>
const T& min(const T& x, const Rest&... xs) {
    const T& xs_min = min(xs...);
    return x < xs_min ? x : xs_min;
}

void println(const char*);
void println(StringView);
void println(String);

void eprintln(const char*);
void eprintln(String);
void eprintln(StringView);

String to_string(Allocator*, int32_t);
String to_string(Allocator*, uint32_t);
String to_string(Allocator*, int64_t);
String to_string(Allocator*, uint64_t);

#ifdef OK_IMPLEMENTATION

FixedBufferAllocator _temp_allocator_impl{};
Allocator* temp_allocator = &_temp_allocator_impl;

ArenaAllocator _static_allocator_impl{};
Allocator* static_allocator = &_static_allocator_impl;

static void _init_region(ArenaAllocator::Region* region, size_t size) {
    region->data = (uint8_t*)OK_ALLOC_PAGE(size);
    OK_ASSERT(region->data != (void*)-1);
    region->size = size;
    region->off = 0;
    region->next = nullptr;
}

// ALLOCATORS IMPLEMENTATION
void* FixedBufferAllocator::raw_alloc(size_t size) {
    size = align_to(size, sizeof(void*));

    if (buffer == nullptr) {
        buffer_size = max(size, OK_PAGE_SIZE * FixedBufferAllocator::DEFAULT_PAGE_COUNT);
        buffer_size = align_to(buffer_size, OK_PAGE_ALIGN);
        buffer = OK_ALLOC_PAGE(buffer_size);
        buffer_off = 0;
    }

    if (size > buffer_size) return nullptr;

    if (buffer_size - buffer_off < size) {
        buffer_off = size;
        return buffer;
    }

    auto* ptr = (uint8_t*)buffer + buffer_off;
    buffer_off += size;
    return (void*)ptr;
}

void FixedBufferAllocator::raw_dealloc(void* ptr, size_t size) {
    if (ptr == (void*)((uint8_t*)buffer + buffer_off)) buffer_off -= size;
}

ArenaAllocator::Region* ArenaAllocator::alloc_region(size_t region_size) {
    using Region = ArenaAllocator::Region;

    region_size = align_to(region_size, OK_PAGE_ALIGN);

    Region* current_region = region_pool;

    while (current_region) {
        if (current_region->size - current_region->off >= sizeof(Region)) {
            auto* region = (Region*)((uint8_t*)current_region->data + current_region->off);

            _init_region(region, region_size);

            current_region->off += align_to(sizeof(Region), sizeof(void*));

            return region;
        }

        current_region = current_region->next;
    }

    current_region = (Region*)OK_ALLOC_SMOL(sizeof(Region));

    current_region->data = OK_ALLOC_PAGE(OK_PAGE_SIZE);
    current_region->size = OK_PAGE_SIZE;
    current_region->off = align_to(sizeof(Region), sizeof(void*));
    current_region->next = region_pool;

    this->region_pool = current_region;

    auto* resulting_region = (Region*)current_region->data;
    _init_region(resulting_region, region_size);
    return resulting_region;
}

void* ArenaAllocator::raw_alloc(size_t size) {
    ArenaAllocator::Region* region_head = this->head;

    size = align_to(size, sizeof(void*));

    while (region_head) {
        if (region_head->size - region_head->off >= size) {
            void* ptr = (void*)((uint8_t*)region_head->data + region_head->off);
            region_head->off += size;
            return ptr;
        }

        region_head = region_head->next;
    }

    size_t region_size = align_to(size, OK_PAGE_ALIGN);
    region_head = alloc_region(region_size);
    region_head->next = this->head;
    this->head = region_head;

    void* ptr = (void*)((uint8_t*)region_head->data + region_head->off);
    region_head->off += size;
    return ptr;
}

void ArenaAllocator::raw_dealloc(void* ptr, size_t size) {
    OK_UNUSED(ptr);
    OK_UNUSED(size);
}

void* ArenaAllocator::raw_resize(void* old_ptr, size_t old_size, size_t new_size) {
    auto* new_ptr = raw_alloc(new_size);
    memcpy(new_ptr, old_ptr, old_size);
    return new_ptr;
}

// STRING IMPLEMENTATION

String String::alloc(Allocator* a, size_t capacity) {
    String s;
    s.data = List<char>::alloc(a, capacity + 1);
    s.data.push(NULL_CHAR);

    return s;
}

String String::alloc(Allocator* a, const char* data, size_t data_len) {
    auto s = String::alloc(a, data_len);

    for (size_t i = 0; i < data_len; i++) s.push((char)data[i]);

    return s;
}

String String::alloc(Allocator* a, const char* data) {
    size_t data_len = strlen((const char*)data);
    return String::alloc(a, data, data_len);
}

String String::from(List<char> chars) {
    String s;
    s.data = chars;
    s.data.push('\0');
    return s;
}

String String::from(List<uint8_t> bytes) {
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
    for (size_t i = 0; i < sv.count; ++i) push(sv.data[i]);
}

void String::append(String str) {
    size_t str_count = str.count();
    for (size_t i = 0; i < str_count; ++i) push(str.data.items[i]);
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

    size_t bytes_needed = count() + required_buf_size;
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

    for (size_t i = 0; i < prefix.count; i++) {
        if (data.items[i] != prefix.data[i]) return false;
    }

    return true;
}

// STRING VIEW IMPLEMENTATION
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
        case EFAULT:       OK_PANIC_FMT("Parameter 'path' (%p) is not mapped to the current process", path);
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
    off_t seek_res = _lseek(fd, 0, SEEK_END);
    OK_ASSERT(seek_res != (off_t)-1);
}

off_t File::seek_end() const {
    off_t seek_res = _lseek(fd, 0, SEEK_SET);
    OK_ASSERT(seek_res != (off_t)-1);

    return seek_res;
}

size_t File::size() const {
    seek_start();
    return seek_end();
}

static inline int64_t _read(int fd, void* buffer, size_t count) {
#if OK_UNIX
    return ::read(fd, buffer, count);
#elif OK_WINDOWS
    return ::_read(fd, buffer, (unsigned int)count);
#endif // OK_UNIX
}

Optional<File::ReadError> File::read(uint8_t* buf, size_t count) {
    int64_t r = ok::_read(fd, buf, count);

    if (r < 0) {
        switch (errno) {
        case EIO: return ReadError::IO_ERROR;
        case EFAULT: OK_PANIC_FMT("The buffer (%p) is mapped outside the current process", buf);
        default: OK_UNREACHABLE();
        }
    }

    return {};
}

Optional<File::ReadError> File::read_full(Allocator* a, List<uint8_t>* out) {
    size_t sz = size();
    *out = List<uint8_t>::alloc(a, sz);
    out->count = sz;
    return read(out->items, sz);
}

// PROCEDURES IMPLEMENTATION
String to_string(Allocator* allocator, uint32_t value) {
    String s = String::alloc(allocator, 10);

    uint32_t value_copy = value;
    int idx = 0;

    while (value_copy /= 10) idx++;

    size_t string_count = idx + 1;

    do {
        char digit = value % 10;
        value /= 10;
        s.data.items[idx--] = digit + '0';
    } while (value != 0);

    s.data.items[string_count] = String::NULL_CHAR;
    s.data.count = string_count + 1;

    return s;
}

String to_string(Allocator* allocator, int32_t input_value) {
    String s = String::alloc(allocator, 10);

    uint32_t value = input_value;
    int idx = 0;

    if (input_value < 0) {
        s.push('-');
        value = (uint32_t)-input_value;
        idx = 1;
    }

    uint32_t value_copy = value;

    while (value_copy /= 10) idx++;

    size_t string_count = idx + 1;

    do {
        char digit = value % 10;
        value /= 10;
        s.data.items[idx--] = digit + '0';
    } while (value != 0);

    s.data.items[string_count] = String::NULL_CHAR;
    s.data.count = string_count + 1;

    return s;
}

String to_string(Allocator* allocator, uint64_t value) {
    String s = String::alloc(allocator, 10);

    uint64_t value_copy = value;
    int idx = 0;

    while (value_copy /= 10) idx++;

    size_t string_count = idx + 1;

    do {
        char digit = value % 10;
        value /= 10;
        s.data.items[idx--] = digit + '0';
    } while (value != 0);

    s.data.items[string_count] = String::NULL_CHAR;
    s.data.count = string_count + 1;

    return s;
}

String to_string(Allocator* allocator, int64_t input_value) {
    String s = String::alloc(allocator, 10);

    uint64_t value = input_value;
    int idx = 0;

    if (input_value < 0) {
        s.push('-');
        value = -input_value;
        idx = 1;
    }

    uint64_t value_copy = value;

    while (value_copy /= 10) idx++;

    size_t string_count = idx + 1;

    do {
        char digit = value % 10;
        value /= 10;
        s.data.items[idx--] = digit + '0';
    } while (value != 0);

    s.data.items[string_count] = String::NULL_CHAR;
    s.data.count = string_count + 1;

    return s;
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
uint64_t fnv1(StringView sv) {
    constexpr const uint64_t fnv_offset_basis = 0xCBF29CE484222325;
    constexpr const uint64_t fnv_prime = 0x100000001B3;

    uint64_t hash = fnv_offset_basis;

    for (size_t i = 0; i < sv.count; ++i) {
        hash *= fnv_prime;

        uint8_t byte = sv.data[i];
        hash ^= (uint64_t)byte;
    }

    return hash;
}
};

#endif
};

#endif // OK_H_
