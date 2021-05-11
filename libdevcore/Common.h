#pragma once
fdef _M_IX86
#pragma warning(disable:4244)
#endif

#if _MSC_VER && _MSC_VER < 1900
#define _ALLOW_KEYWORD_MACROS
#define noexcept throw()
#endif

#ifdef __INTEL_COMPILER
#pragma warning(disable:3682) //call through incomplete class
#endif

#include <map>
#include <queue>
#include <unordered_map>
#include <vector>
#include <set>
#include <unordered_set>
#include <functional>
#include <string>
#include <chrono>
#include <sys/time.h>
#pragma warning(push)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <boost/multiprecision/cpp_int.hpp>
#pragma warning(pop)
#pragma GCC diagnostic pop
#include "vector_ref.h"

// CryptoPP defines byte in the global namespace, so must we.
using byte = uint8_t;

// Quote a given token stream to turn it into a string.
#define DEV_QUOTED_HELPER(s) #s
#define DEV_QUOTED(s) DEV_QUOTED_HELPER(s)

#define DEV_IGNORE_EXCEPTIONS(X) try { X; } catch (...) {}

#define DEV_IF_THROWS(X) try{X;}catch(...)

namespace dev
{

extern char const* Version;

static const std::string EmptyString;

// Binary data types.
using bytes = std::vector<byte>;
using bytesRef = vector_ref<byte>;
using bytesConstRef = vector_ref<byte const>;

template <class T>
class secure_vector
{
public:
	secure_vector() {}
	secure_vector(secure_vector<T> const& /*_c*/) = default;  // See https://github.com/ethereum/libweb3core/pull/44
	explicit secure_vector(size_t _size): m_data(_size) {}
	explicit secure_vector(size_t _size, T _item): m_data(_size, _item) {}
	explicit secure_vector(std::vector<T> const& _c): m_data(_c) {}
	explicit secure_vector(vector_ref<T> _c): m_data(_c.data(), _c.data() + _c.size()) {}
	explicit secure_vector(vector_ref<const T> _c): m_data(_c.data(), _c.data() + _c.size()) {}
	~secure_vector() { ref().cleanse(); }

	secure_vector<T>& operator=(secure_vector<T> const& _c)
	{
		if (&_c == this)
			return *this;

		ref().cleanse();
		m_data = _c.m_data;
		return *this;
	}
	std::vector<T>& writable() { clear(); return m_data; }
	std::vector<T> const& makeInsecure() const { return m_data; }

	void clear() { ref().cleanse(); }

	vector_ref<T> ref() { return vector_ref<T>(&m_data); }
	vector_ref<T const> ref() const { return vector_ref<T const>(&m_data); }

	size_t size() const { return m_data.size(); }
	bool empty() const { return m_data.empty(); }

	void swap(secure_vector<T>& io_other) { m_data.swap(io_other.m_data); }

private:
	std::vector<T> m_data;
};

}
