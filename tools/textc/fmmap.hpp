#ifndef FMMAP_HPP
#define FMMAP_HPP

#include <filesystem>

class fmmap {
private:
	void* addr;
	size_t length;
public:
	fmmap(const std::filesystem::path& filename);
	~fmmap(void);

	const char* begin() const;
	const char* end() const;
	std::reverse_iterator<const char*> rbegin() const;
	std::reverse_iterator<const char*> rend() const;
};

#endif        //  #ifndef FMMAP_HPP
