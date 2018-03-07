#include <butil/resource_pool.h> //butil::ResourcePool
#include <butil/files/file_path.h> //butil::file_path
#include <butil/files/file.h> //butil::file
#include <string.h> //memcpy
#include "CszBitTorrent.h"
#include "sha1.h"

namespace Csz
{
    BitMemory::~BitMemory()
    {
#ifdef CszTest
        Csz::LI("destructor Bit Memory");
#endif
        _Clear();
    }

    void BitMemory::_Clear()
    {
        auto resource_pool= butil::ResourcePool<BT>::singleton();
        //1.get index
        for (auto& val : memory_pool)
        {
            //2.get id(index have vetor id)
            for (auto& id: (val.second)->second)
            {
                resource_pool->return_resource(id.first);
            }    
        }
        resource_pool->clear_resources();
        return ;
    }    

    void BitMemory::Init(int32_t T_index_end,int32_t T_length_end,int32_t T_length_normal)
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
		length_normal= T_length_normal;
        return ;
    }


#define MEMORYPOOL_DEAL(T_LEN)																	                        \
            /*1.new memory*/																	                        \
            if (memory_pool.find(T_index)== memory_pool.end())									                        \
            {																				                        	\
                TypeP data= std::make_shared<Type>(std::make_pair(T_LEN,DataType()));			                        \
                int32_t cur_memory= T_LEN;														                        \
                while (cur_memory> 0)															                        \
                {																				                        \
                    butil::ResourceId<BT> id;													                        \
                    auto buf= resource_pool->get_resource(&id);									                        \
                    if (buf== nullptr)															                        \
                    {																			                        \
                        _Clear();																                        \
                        /*TODO wait*/															                        \
                        Csz::ErrQuit("Bit Memory write failed,new return null");				                        \
                        return false;															                        \
                    }																			                        \
                    (data->second).emplace_back(std::make_pair(id,reinterpret_cast<char*>(buf)));						\
                    cur_memory-= sizeof(BT);													                        \
                }																				                        \
                memory_pool.emplace(T_index,std::move(data));									                        \
            }																					                        \
            /*2.write*/																	                                \
            auto block= memory_pool[T_index];													                        \
            if (block->first< T_length)															                        \
            {																					                        \
                Csz::ErrMsg("Bit Memory write failed,left< write length");						                        \
                return false;																	                        \
            }																					                        \
            auto block_index= T_begin/ sizeof(BT);												                        \
            T_begin%= sizeof(BT);																                        \
            auto& write_buf= block->second;														                        \
			/*2.1judge the slice are between  blocks*/											                        \
			if (T_begin+ T_length> (int)sizeof(BT))													                    \
			{																					                        \
				auto lhs_len= sizeof(BT)- T_begin;												                        \
				memcpy(write_buf[block_index].second+ T_begin,T_buf,lhs_len);					                        \
				memcpy(write_buf[block_index+ 1].second,T_buf+ lhs_len,T_length- lhs_len);		                        \
			}																					                        \
			else																				                        \
			{																					                        \
				memcpy(write_buf[block_index].second+ T_begin,T_buf,T_length);                                          \
			}																					                        \
            block->first= block->first- T_length;												                        \
			/*2.2 real write*/																	                        \
            if (0== block->first)																                        \
            {																					                        \
                /*md5*/																			                        \
				std::string sha1_data;															                        \
				sha1_data.reserve(T_LEN);														                        \
				if (T_LEN<= (int)sizeof(BT))															                \
				{																				                        \
					sha1_data.append(write_buf[0].second);										                        \
				}																				                        \
				else																			                        \
				{																				                        \
					int32_t sha1_len= T_LEN;													                        \
					/*if debug judge sha1_len< 0*/												                        \
					for (auto start= write_buf.cbegin(),stop= write_buf.cend();start< stop && sha1_len> 0; ++start)     \
					{																			                        \
						if (sha1_len> (int)sizeof(BT))												                    \
							sha1_data.append(start->second,sizeof(BT));							                        \
						else																	                        \
							sha1_data.append(start->second,sha1_len);								                    \
						sha1_len-= sizeof(BT);													                        \
					}																			                        \
				}																				                        \
				std::string hash_data;															                        \
				hash_data.resize(20);															                        \
				Sha1(sha1_data.c_str(),sha1_data.size(),reinterpret_cast<unsigned char*>(&hash_data[0]));               \
				/*error hash val*/																                        \
				if (hash_data!= TorrentFile::GetInstance()->GetHash(T_index))									        \
				{																				                        \
					Csz::ErrMsg("Bit Memory write failed,piece hash info is error");			                        \
                    ClearIndex(T_index);                                                                                \
                    return false;                                                                                       \
				}																				                        \
				else																			                        \
				{																				                        \
					/*write file*/                                                                                      \
					if (_Write(T_index)== T_LEN)														                \
					{																			                        \
						/*TODO update bitfield,send have*/                                                              \
						LocalBitField::GetInstance()->FillBitField(T_index);    			                            \
                        ClearIndex(T_index);                                                                            \
                        return true;                                                                                    \
					}																			                        \
                    ClearIndex(T_index);                                                                                \
                    return false;                                                                                       \
				}																				                        \
            }																					


    bool BitMemory::Write(int32_t T_index,int32_t T_begin,const char* T_buf,int32_t T_length)
    {
        if (T_index< 0)
        {
            Csz::ErrMsg("Bit Memory write failed,index < 0");
            return  false;
        }
        if (nullptr== T_buf || T_length<= 0)
        {
            Csz::ErrMsg("Bit Memory write failed,T_buf== nullptr or length < 0");
            return false;
        }
        auto resource_pool= butil::ResourcePool<BT>::singleton();
        //1.deal end index
        if (index_end== T_index)
        { 
            MEMORYPOOL_DEAL(length_end);
        }
        //2.deal normal index
        MEMORYPOOL_DEAL(length_normal);
    }

#undef  MEMORYPOOL_DEAL
    
    void BitMemory::ClearIndex(int32_t T_index)
    {
        auto flag= memory_pool.find(T_index);
        if (flag== memory_pool.end())
        {
            return ;
        }
        auto resource_pool= butil::ResourcePool<BT>::singleton();
        for (auto& val : flag->second->second)
        {
            resource_pool->return_resource(val.first);
        }
        return ;
    }
	
	int32_t BitMemory::_Write(int32_t T_index)
	{
		//bug,check point is nullptr
		const auto& read_buf= memory_pool[T_index]->second;
		if (read_buf.empty())
		{
			Csz::ErrMsg("Bit Memory write failed,read buf is empty");
			return -1;
		}
		auto file_name= TorrentFile::GetInstance()->GetFileName(T_index);
		if (file_name.empty())
		{
			Csz::ErrMsg("Bit Memory write in local file failed,return file is empty");
			return -1;
		}
		auto start= read_buf.cbegin();
		auto stop= read_buf.cend();
		int cur_read= 0;
		int left_read= sizeof(BT);
		int32_t write_byte= 0;
		for (const auto& val : file_name)
		{
			if (start>= stop)
			{
				Csz::ErrMsg("Bit Memory write failed,read buf goto end,but have need write local file");
				return -1;
			}
			int32_t code;
			butil::FilePath file_path(val.first);
			butil::File file(file_path,butil::File::FLAG_OPEN_ALWAYS | butil::File::FLAG_WRITE);
			if (!file.IsValid())
			{
				Csz::ErrMsg("Bit Memory write failed,%s",butil::File::ErrorToString(file.error_details()).c_str());
				return -1;
			}
			if (left_read>= val.second.second)
			{
				code= file.Write(val.second.first,start->second+ cur_read,val.second.second);
				cur_read+= val.second.second;
				left_read-= val.second.second;
			}
			else
			{
				code= file.Write(val.second.first,start->second+ cur_read,left_read);
				++start;
				if (start>= stop)
				{
					Csz::ErrMsg("Bit Memory write failed,read buf goto end,but have need write local file");
					return -1;
				}
				code= code+ file.Write(val.second.first+ left_read,start->second,val.second.second- left_read);
				cur_read= val.second.second- left_read;
				left_read= sizeof(BT)- left_read;
			}
			if (code!= val.second.second)
			{
				Csz::ErrMsg("Bit Memory write failed,write byte != file need num,file name %s",val.first.c_str());
				return -1;
			}
			write_byte+= code;
		}
		return  write_byte;
	}

	void BitMemory::COutInfo()
	{
		Csz::LI("Bit Memory index end=%d,length end=%d,length normal=%d",index_end,length_end,length_normal);
		return ;
	}
}
