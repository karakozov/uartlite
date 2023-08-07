
#include "mapper.h"
#include "exceptinfo.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>

using namespace std;

//-----------------------------------------------------------------------------

Mapper::Mapper()
{
    mappedList.clear();
    extHandle = false;
    try {
        openDevMem();
    } catch(const except_info_t& err) {
        fprintf(stderr, "%s, %d, %s(): %s", __FILE__, __LINE__, __func__, err.info.c_str());
    }
}

//-----------------------------------------------------------------------------

void Mapper::init(int handle)
{
    extHandle = true;
    fd = handle;
}

//-----------------------------------------------------------------------------

Mapper::Mapper(int handle)
{
    mappedList.clear();
    if(handle) {
        extHandle = true;
        fd = handle;
    } else {
        extHandle = false;
        try {
            openDevMem();
        } catch(const except_info_t& err) {
            fprintf(stderr, "%s, %d, %s(): %s", __FILE__, __LINE__, __func__, err.info.c_str());
        }
    }
}

//-----------------------------------------------------------------------------

Mapper::~Mapper()
{
    unmap();
    if(!extHandle)
        closeDevMem();
}

//-----------------------------------------------------------------------------

int Mapper::openDevMem()
{
    fd = open("/dev/mem", O_RDWR, 0666);
    if(fd < 0) {
        throw except_info("%s, %d: %s() - Error open /dev/mem.\n", __FILE__, __LINE__, __FUNCTION__);
    }
    return 0;
}

//-----------------------------------------------------------------------------

void Mapper::closeDevMem()
{
    close(fd);
}

//-----------------------------------------------------------------------------

void* Mapper::map(size_t physicalAddress, uint32_t areaSize)
{
    struct map_addr_t map = {0, physicalAddress, areaSize};

    map.virtualAddress = mmap(0, map.areaSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)map.physicalAddress);
    if(map.virtualAddress == MAP_FAILED) {
        throw except_info("%s, %d: %s() - Error in IPC_mapPhysAddr().\n", __FILE__, __LINE__, __FUNCTION__);
    }

    mappedList.push_back(map);

    return map.virtualAddress;
}

//-----------------------------------------------------------------------------

void* Mapper::map(void* physicalAddress, uint32_t areaSize)
{
    struct map_addr_t map = {0, reinterpret_cast<size_t>(physicalAddress), areaSize};

    map.virtualAddress = mmap(0, map.areaSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)map.physicalAddress);
    if(map.virtualAddress == MAP_FAILED) {
        throw except_info("%s, %d: %s() - Error in IPC_mapPhysAddr().\n", __FILE__, __LINE__, __FUNCTION__);
    }

    mappedList.push_back(map);

    return map.virtualAddress;
}

//-----------------------------------------------------------------------------

void Mapper::unmap(void* va)
{
    for(unsigned i=0; i<mappedList.size(); i++) {
        struct map_addr_t map = mappedList.at(i);
        if(map.virtualAddress == va) {
            munmap(map.virtualAddress, map.areaSize);
            map.virtualAddress = 0;
            map.physicalAddress = 0;
            map.areaSize = 0;
            mappedList.erase(mappedList.begin()+i);
        }
    }
}

//-----------------------------------------------------------------------------

void Mapper::unmap()
{
    for(unsigned i=0; i<mappedList.size(); i++) {
        struct map_addr_t map = mappedList.at(i);
        if(map.virtualAddress) {
            munmap(map.virtualAddress, map.areaSize);
            map.virtualAddress = 0;
            map.physicalAddress = 0;
            map.areaSize = 0;
        }
    }
    mappedList.clear();
}

//-----------------------------------------------------------------------------
