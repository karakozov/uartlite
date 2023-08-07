
#ifndef __MAPPER_H__
#define __MAPPER_H__

#include <stdint.h>
#include <vector>
#include <string>
#include <sstream>
#include <memory>

//-----------------------------------------------------------------------------

struct map_addr_t {
    void *virtualAddress;
    size_t physicalAddress;
    size_t areaSize;
};

//-----------------------------------------------------------------------------

class Mapper {

public:
    Mapper();
    Mapper(int handle);
    virtual ~Mapper();

    void  init(int handle);
    void* map(size_t pa, uint32_t size);
    void* map(void*  pa, uint32_t size);
    void  unmap(void* va);
    void  unmap();

private:
    int fd;
    bool extHandle;

    std::vector<struct map_addr_t> mappedList;

    int openDevMem();
    void closeDevMem();
};

//-----------------------------------------------------------------------------

using mapper_t = std::shared_ptr<Mapper>;

//-----------------------------------------------------------------------------

template <typename block_type, typename... args_type>
auto get_mapper(args_type &&...args) {
  return std::make_shared<block_type>(std::forward<args_type>(args)...);
}

//-----------------------------------------------------------------------------

#endif //__MAPPER_H__
