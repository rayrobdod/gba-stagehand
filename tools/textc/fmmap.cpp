#include "fmmap.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

static_assert(std::ranges::contiguous_range<fmmap>);

fmmap::fmmap(const std::filesystem::path& filename) {
	this->length = std::filesystem::file_size(filename);
	int fd = open(filename.c_str(), O_RDONLY);
	this->addr = mmap(NULL, this->length, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
}

fmmap::~fmmap(void) {
	munmap(this->addr, this->length);
}

const char* fmmap::begin() const {
	return reinterpret_cast<const char*>(this->addr);
}
const char* fmmap::end() const {
	return this->begin() + this->length;
}
std::reverse_iterator<const char*> fmmap::rbegin() const {
	std::reverse_iterator<const char*> retval(this->end());
	return retval;
}
std::reverse_iterator<const char*> fmmap::rend() const {
	std::reverse_iterator<const char*> retval(this->begin());
	return retval;
}
