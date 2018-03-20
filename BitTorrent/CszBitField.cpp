#include <bitset>
#include <butil/files/file.h>
#include <butil/files/file_path.h>
#include "sha1.h"
#include "CszBitTorrent.h"

const uint8_t bit_hex[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};

namespace Csz
{
	void BitField::SetParameter(std::string T_bit_field,int32_t T_total)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		_Clear();
		_SetParameter(std::move(T_bit_field));
		total= T_total;
		return ;
	}

	inline void BitField::_SetParameter(std::string T_bit_field)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (T_bit_field.empty())
		{
			Csz::ErrMsg("[Bit Field set parameter]->faile,parameter is empty");
			return ;
		}
		prefix_and_bit_field.append(std::move(T_bit_field));
		_SetPrefixLength();
		return ;
	}

	inline void BitField::_SetPrefixLength()
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (prefix_and_bit_field.size()<= 5)
		{
			Csz::ErrMsg("[Bit Field set prefix length]->faile,too small,can't set prefix length");
			return ;
		}
		//minus prefix length
		//not sure host is small endian
		int32_t temp= htonl(prefix_and_bit_field.size()- 4);
		//host change network byte
		//not use static_cast<int32_t*>,prefix_and_bit_field is string
		char* flag= reinterpret_cast<char*>(&temp);
		prefix_and_bit_field[0]= *(flag+ 0);
		prefix_and_bit_field[1]= *(flag+ 1);
		prefix_and_bit_field[2]= *(flag+ 2);
		prefix_and_bit_field[3]= *(flag+ 3);
		return ;
	}

	inline void BitField::_Clear()
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		//index -> npos
		prefix_and_bit_field.erase(5);
		return ;
	}

	void BitField::FillBitField(int32_t T_index)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (T_index< 0)
		{
			Csz::ErrMsg("[Bit Field fill bit field]->failed,index < 0,can't fill bit field");
			return ;
		}
		if (prefix_and_bit_field.size()<= 5)
		{
			Csz::ErrMsg("[Bit Field fill bit field]->failed, too small,can't fill bit field");
			return ;
		}
		//every bit express one field
		auto index= T_index/ 8+ 5;

		auto& val= prefix_and_bit_field[index];

		T_index %= 8;

		//1.check exist bit
		if (val & bit_hex[T_index])
		{
			return ;
		}
		//2.lack bit,set fill bit
		++cur_sum;
		switch (T_index)
		{
			case 0:
				val|= bit_hex[0];
				break;
			case 1:
				val|= bit_hex[1];
				break;
			case 2:
				val|= bit_hex[2];
				break;
			case 3:
				val|= bit_hex[3];
				break;
			case 4:
				val|= bit_hex[4];
				break;
			case 5:
				val|= bit_hex[5];
				break;
			case 6:
				val|= bit_hex[6];
				break;
			case 7:
				val|= bit_hex[7];
				break;
		}
		return ;
	}

	bool BitField::CheckPiece(int32_t& T_index)
	{
#ifdef CszTest
        if (T_index+1>= total)
        {   
            Csz::LI("[%s->%s->%d->T_index=%d>= total=%d]",__FILE__,__func__,__LINE__,T_index,total);
        }
        else
        {
            Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
        }
#endif
		if (T_index< 0)
		{
			Csz::ErrMsg("[Bit Field check piece]->failed,index < 0");
            T_index= -1;
			return true;
		}
		if (prefix_and_bit_field.size()<= 5)
		{
			Csz::ErrMsg("[Bit Field check piece]->failed,too small,not found index");
            T_index= -1;
			return true;
		}
		//every bit express one field
		auto index= T_index/ 8+ 5;
		auto& val= prefix_and_bit_field[index];
		bool ret= false;
		switch (T_index% 8)
		{
			case 0:
				ret= val& bit_hex[0];
				break;
			case 1:
				ret= val& bit_hex[1];
				break;
			case 2:
				ret= val& bit_hex[2];
				break;
			case 3:
				ret= val& bit_hex[3];
				break;
			case 4:
				ret= val& bit_hex[4];
				break;
			case 5:
				ret= val& bit_hex[5];
				break;
			case 6:
				ret= val& bit_hex[6];
				break;
			case 7:
				ret= val& bit_hex[7];
				break;
		}
		return ret;
	}

	std::vector<int32_t> BitField::LackNeedPiece(const char* T_bit_field,const int T_len)
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		std::vector<int32_t> ret;
		if (prefix_and_bit_field.size()<= 5)
		{
			Csz::ErrMsg("[Bit Field lack need piece]->failed,too small,found lack need piece failed");
			return ret;
		}
		if (nullptr== T_bit_field || T_len+ 5!= (int)prefix_and_bit_field.size() )
		{
			Csz::ErrMsg("[Bit Field lack need piece]->failed,not found bit field,bit field is nullptr or len != bit field len");
			return ret;
		}
		int cur_len= 0;
		int32_t index= 0;
        //fix bug
        int32_t check_total= total;
		while (cur_len< T_len)
		{
			for (int i= 0; i< 8 && check_total> 0; ++i,++index,--check_total)
			{
				//1.have piece
				if(T_bit_field[cur_len] & bit_hex[i])
				{
					//2.check local bit
					if (prefix_and_bit_field[cur_len] & bit_hex[i])
					{
						//2.1 loca have piece
						continue ;
					}
					//2.2local lack piece
					ret.emplace_back(index);
				}
			}
			++cur_len;
		}
		return std::move(ret);
	}

	bool BitField::GameOver()const
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
		if (total== cur_sum)
			return true;
		return false;
/*
		if (prefix_and_bit_field.size()<= 5)
		{
			return false;
		}
		for (auto start= prefix_and_bit_field.cbegin()+ 5,stop= prefix_and_bit_field.cend(); start< stop; ++start)
		{
			const char temp= 0xff;
			if (*start!= temp)
			{
				if (start== (stop- 1) && T_end_bit!= 0x00 && *start== T_end_bit)
					return true;
				return false;
			}
		}
		return true;
*/
	}
    
    void BitField::LoadLocalFile()
    {
        auto torrent_file= TorrentFile::GetInstance();
        std::string sha1_str;
        std::string sha1_data;
        sha1_data.resize(20);
        sha1_str.reserve(torrent_file->GetIndexNormalLength());
        for (int index= 0,max_index= torrent_file->GetIndexTotal(); index< max_index; ++index)
        {
            auto file_name= torrent_file->GetFileName(index);
            if (file_name.empty())
            {
                Csz::ErrQuit("[Bit Field load local file]->failed,file name is empty");
                return ;
            }
            sha1_str.resize(torrent_file->GetPieceLength(index),'\0');
            int32_t cur_read= 0;
            for (const auto& val : file_name)
            {
                butil::FilePath file_path(val.first);
                butil::File file(file_path,butil::File::FLAG_OPEN | butil::File::FLAG_READ);
                if (!file.IsValid())
                {
                    Csz::ErrMsg("[Bit Field load local file]->failed,name=%s,%s",val.first.c_str(),butil::File::ErrorToString(file.error_details()).c_str());
                    file.Close();
                    continue;
                }

                auto read_byte= file.Read(val.second.first,(&sha1_str[0])+ cur_read,val.second.second);
                if (val.second.second!= read_byte && read_byte!= 0)
                {
                    Csz::ErrMsg("[Bit Field load local file]->failed,length!= read byte,file name=%s,index=%d,max_index=%d,need=%d,real=%d",val.first.c_str(),index,max_index,val.second.second,read_byte);
                    file.Close();
                    exit(0);
                    return ;
                }
                file.Close();
            }

            Sha1(sha1_str.c_str(),sha1_str.size(),reinterpret_cast<unsigned char*>(&sha1_data[0]));
            if (sha1_data== torrent_file->GetHash(index))
            {
                FillBitField(index);
            }   
        }
        return ;
    }    

    uint32_t BitField::DownLoad() const
    {
        return cur_sum* TorrentFile::GetInstance()->GetIndexNormalLength();
    }

    uint32_t BitField::LeftSize() const
    {
        return (total- cur_sum)* TorrentFile::GetInstance()->GetIndexNormalLength();
    }

	void BitField::ProgressBar()
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //bug!!
		char* data= const_cast<char*>(&prefix_and_bit_field[0]);
		std::vector<std::bitset<8>> bit_field;
		for (int i= 5,len= prefix_and_bit_field.size();i< len; ++i)
		{
			bit_field.emplace_back(prefix_and_bit_field[i]);
		}
		std::string bit_info;
		bit_info.reserve(bit_field.size()* 8);
		for (auto& val : bit_field)
		{
			bit_info.append(std::move(val.to_string()));
		}
		if (!bit_info.empty())
		{
			Csz::LI("%s,cur_sum=%d -> total=%d,= %d",bit_info.c_str(),cur_sum,total,cur_sum/ total);
		}
		return ;
	}

    

	void BitField::COutInfo() const
	{
#ifdef CszTest
        Csz::LI("[%s->%s->%d]",__FILE__,__func__,__LINE__);
#endif
        //bug!!
		char* data= const_cast<char*>(&prefix_and_bit_field[0]);
		Csz::LI("[Bit Field INFO]:len=%u,id=%d",ntohl(*reinterpret_cast<uint32_t*>(data)),int(*(data+ 4)));
		std::vector<std::bitset<8>> bit_field;
		for (int i= 5,len= prefix_and_bit_field.size();i< len; ++i)
		{
			bit_field.emplace_back(prefix_and_bit_field[i]);
		}
		std::string bit_info;
		bit_info.reserve(bit_field.size()* 8);
		for (auto& val : bit_field)
		{
			bit_info.append(std::move(val.to_string()));
		}
		bit_info.append(",total="+ std::to_string(total)+",cur_sum="+ std::to_string(cur_sum));
		if (!bit_info.empty())
			Csz::LI("%s",bit_info.c_str());
		return ;
	}
}
