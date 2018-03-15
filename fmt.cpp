#include "fmt.h"

#include "array.h"

namespace oak::detail {

	static char toStrBuffer[32];

	size_t to_str_size(char v) {
		return 1;
	}

	size_t to_str_size(int32_t v) {
		return to_str_size(static_cast<int64_t>(v));
	}

	size_t to_str_size(int64_t v) {
		int64_t c = 1;
		size_t t = 1;
		if (v < 0) {
			v = -v;
		}
		while (c <= v) {
			t++;
			c *= 10;
		}
		return t;
	}

	size_t to_str_size(uint32_t v) {
		return to_str_size(static_cast<uint64_t>(v));
	}

	size_t to_str_size(uint64_t v) {
		uint64_t c = 1;
		size_t t = 1;
		while (c <= v) {
			t++;
			c *= 10;
		}
		return t;
	}

	size_t to_str_size(float v) {
		std::memset(toStrBuffer, 0, 32);
		std::sprintf(toStrBuffer, "%f", v);
		return c_str_len(toStrBuffer);
	}

	size_t to_str_size(double v) {
		std::memset(toStrBuffer, 0, 32);
		std::sprintf(toStrBuffer, "%lf", v);
		return c_str_len(toStrBuffer);
	}

	size_t to_str_size(const char *v) {
		return c_str_len(v);
	}

	size_t to_str_size(const unsigned char *v) {
		return c_str_len(reinterpret_cast<const char*>(v));
	}

	size_t to_str_size(String v) {
		return v.size;
	}

	String to_str(char v) {
		toStrBuffer[0] = v;
		return String{ toStrBuffer, 1 };
	}

	String to_str(uint32_t v) {
		std::memset(toStrBuffer, 0, 32);
		std::sprintf(toStrBuffer, "%u", v);
		return toStrBuffer;
	}
	
	String to_str(uint64_t v) {
		std::memset(toStrBuffer, 0, 32);
		std::sprintf(toStrBuffer, "%lu", v);
		return toStrBuffer;
	}

	String to_str(int32_t v) {
		std::memset(toStrBuffer, 0, 32);
		std::sprintf(toStrBuffer, "%i", v);
		return toStrBuffer;
	}
	
	String to_str(int64_t v) {
		std::memset(toStrBuffer, 0, 32);
		std::sprintf(toStrBuffer, "%li", v);
		return toStrBuffer;
	}

	String to_str(float v) {
		std::memset(toStrBuffer, 0, 32);
		std::sprintf(toStrBuffer, "%f", v);
		return toStrBuffer;
	}

	String to_str(double v) {
		std::memset(toStrBuffer, 0, 32);
		std::sprintf(toStrBuffer, "%lf", v);
		return toStrBuffer;
	}

	String to_str(const char *v) {
		return String{ v };
	}

	String to_str(const unsigned char *v) {
		return String{ reinterpret_cast<const char *>(v) };
	}

	String to_str(String str) {
		return str;
	}

}

namespace oak {

	void Stdout::write(const void *data, size_t size) {
		std::fwrite(data, 1, size, stdout);
	}

	void FileBuffer::write(const void *data, size_t size) {
		std::fwrite(data, 1, size, file);
	}

	void CharBuffer::write(const void *data, size_t size) {
		assert(size <= bufferSize - pos);
		std::memcpy(buffer + pos, data, size);
		pos += size;
	}

	void ArrayBuffer::write(const void *data, size_t size) {
		assert(size <= buffer->size - pos);
		std::memcpy(buffer->data + pos, data, size);
		pos += size;
	}

	void ArrayBuffer::resize(size_t size) {
		buffer->resize(pos + size);
	}

}

