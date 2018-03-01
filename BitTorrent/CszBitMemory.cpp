#include "CszBitTorrent.h"

namespace Csz
{
    BitMemory::~BitMemory()
    {
        Clear();
    }

    void BitMemory::Clear()
    {
        auto resource_pool= butil::ResourcePool<BT>::singleton();
        //1.get index
        for (auto& val : memory_pool)
        {
            //2.get id(index have vetor id)
            for (auto& id: val->second->second)
            {
                resource_pool.return_resource(id.first);
            }    
        }
        resource_pool->clear_resource();
        return ;
    }    

    void BitMemory::Init(int32_t T_index_end,int32_t T_length_end)
    {
        if (T_index_end< 0)
        {
            Csz::ErrQuit("Bit Memory init failed index end < 0");
            return ;
        }
        if (T_length_end< 0)
        {
            Csz::ErrQuit("Bit Memory init failed leng end < 0");
            return ;
        }
        index_end= T_index_end;
        length_end= T_length_end;
        return ;
    }


#define MEMORYPOOL_DEAL(T_LEN)                                                              \
            /*1 new memory*/                                                                \
            if (memory_pool.find(T_index)== memory_pool.end())                              \
            {                                                                               \
                TypeP data= std::make_shared<Type>(std::make_pair(T_LEN,DataType()));       \
                int32_t cur_memory= T_LEN;                                                  \
                while (cur_memory> 0)                                                       \
                {                                                                           \
                    butil::ResourceId<BT> id;                                               \
                    auto buf= resource_pool->get_resource(&id);                             \
                    if (buf== nullptr)                                                      \
                    {                                                                       \
                        Clear();                                                            \
                        /*TODO wait*/                                                       \
                        Csz::ErrQuit("Bit Memory write failed,new return null");            \
                        return ;                                                            \
                    }                                                                       \
                    (data->second).emplace_back(std::make_pair(id,buf));                    \
                    cur_memory-= sizeof(BT);                                                \
                }                                                                           \
                memory_pool.emplace(T_index,std::move(data));                               \
            }                                                                               \
            /*2 real write*/                                                                \
            auto block= memory_pool[T_index];                                               \
            if (block->first< T_length)                                                     \
            {                                                                               \
                Csz::ErrMsg("Bit Memory write failed,left< write length");                  \
                return ;                                                                    \
            }                                                                               \
            auto block_index= T_begin/ sizeof(BT);                                          \
            T_begin%= sizeof(BT);                                                           \
            auto& write_buf= block->second;                                                 \
            memcpy(write_buf[block_index].second+ T_begin,T_buf,T_length);                  \
            block->first= block->first- T_length;                                           \
            if (0== block->first)                                                           \
            {                                                                               \
                /*write file*/                                                              \
                _Write(T_index);                                                            \
                /*TODO update bitfield,send have*/                                          \
                ..                                                                          \
            }                                                                               


    void BitMemory::Write(int32_t T_index,int32_t T_begin,const char* T_buf,int32_t T_length)
    {
        if (T_index< 0)
        {
            Csz::ErrMsg("Bit Memory write failed,index < 0");
            return  ;
        }
        if (nullptr== T_buf || T_length<= 0)
        {
            Csz::ErrMsg("Bit Memory write failed,T_buf== nullptr or length < 0");
            return ;
        }
        auto resource_pool= butil::ResourcePool<BT>::singleton();
        //1.deal end index
        if (index_end== T_index)
        { 
            MEMORYPOOL_DEAL(length_end);
            return ;
        }
        //2.deal normal index
        MEMORYPOOL_DEAL(sizeof(BT));
        return ;
    }

#undef  MEMORYPOOL_DEAL

}
