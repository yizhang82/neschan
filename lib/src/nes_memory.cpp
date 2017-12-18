#include "stdafx.h"
#include "nes_memory.h"
#include "nes_mapper.h"

void nes_memory::load_mapper(shared_ptr<nes_mapper> &mapper)
{
    // unset previous mapper
    _mapper = nullptr;

    // Give mapper a chance to copy all the bytes needed
    mapper->on_load(*this);

    _mapper = mapper;
}