#include <iterator>
#include <cstddef>

// Pass string_ref by value
class string_ref
{
public:
    string_ref() noexcept = default;
    // Don't use explicit.
    constexpr string_ref(decltype(nullptr)) noexcept : ptr(0), len(0) {}
    [[gnu::always_inline]]
        string_ref(const char *s) noexcept : ptr(s), len(__builtin_strlen(s)) {}
    constexpr string_ref(const char *s, std::size_t l) noexcept : ptr(s), len(l) {}
    constexpr string_ref(const char *s, const char *e) noexcept : ptr(s), len(e-s) {}

    string_ref(const string_ref &) noexcept = default;
    string_ref &operator = (const string_ref &) noexcept = default;

    // Don't provide c_str(). No null-terminator is guaranteed.
    constexpr const char *data() const noexcept { return ptr; }
    constexpr std::size_t size() const noexcept { return len; }
    constexpr std::size_t length() const noexcept { return len; }
    constexpr bool empty() const noexcept { return (len == 0); }
    const char &operator [] (std::size_t k) const noexcept { return ptr[k]; }

    constexpr explicit operator bool() const noexcept { return ptr; }
    constexpr bool operator! () const noexcept { return !ptr; }
    constexpr bool operator == (decltype(nullptr)) const noexcept { return !ptr; }
    constexpr bool operator != (decltype(nullptr)) const noexcept { return ptr; }

    void set_data(const char *p) noexcept { ptr = p; }
    void set_length(std::size_t l) noexcept { len = l; }

    using value_type     = const char;
    using reference      = const char &;
    using pointer        = const char *;
    using iterator       = const char *;
    using const_iterator = const char *;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    constexpr iterator begin() const noexcept { return ptr; }
    constexpr iterator cbegin() const noexcept { return ptr; }
    constexpr iterator end() const noexcept { return ptr + len; }
    constexpr iterator cend() const noexcept { return ptr + len; }

    reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
    reverse_iterator crbegin() const noexcept { return reverse_iterator(end()); }
    reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }
    reverse_iterator crend() const noexcept { return reverse_iterator(begin()); }
    reference front() const noexcept { return ptr[0]; }
    reference back() const noexcept { return ptr[len - 1]; }

    bool equal(const string_ref &o) const noexcept {
        return (length() == o.length()) && __builtin_memcmp(data(), o.data(), length()) == 0;
    }

private:
    const char *ptr;
    std::size_t len;
};

inline constexpr string_ref operator ""_ref(const char *s, std::size_t l) noexcept {
    return {s, l};
}
