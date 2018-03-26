#include "CszSocket.h"
#include <bthread/bthread.h>
#define TIMEOUT300MS (300000)

namespace Csz
{
    int RecvTime_us(int T_socket,char* T_buf,int32_t T_len,int T_time)
    {
        if (T_time<= 0 || nullptr== T_buf || T_len<= 0)
        {
            Csz::ErrMsg("[%s->%d]->failed,time< 0 or buf is nullptr or len< 0",__func__,__LINE__);
            return -1;
        }
        //300ms once
        int time_count= T_time/ TIMEOUT300MS;
        int cur_len= 0;
        int code= 0;
        for (int i= 0; i< time_count && T_len> 0; ++i)
        {
            code =recv(T_socket,T_buf+ cur_len,T_len,MSG_DONTWAIT);
            if (code< 0)
            {
                if (errno== EAGAIN || errno== EWOULDBLOCK)
                {
                    //300ms
                    bthread_usleep(TIMEOUT300MS);
                    continue;
                }
                Csz::ErrRet("[%s->%d=%dus,socket=%d]->failed,",__func__,__LINE__,T_time,T_socket);
                break;
            }
            else if (0== code)
            {
                Csz::ErrMsg("[%s->%d time out=%dus,socket=%d]->failed,peer close",__func__,__LINE__,T_time,T_socket);
                break;
            }
			else
			{
				cur_len+= code;
				T_len-= code;
			}
        }
		if (0== T_len)
		{
			return cur_len;
		}
		if (code!= 0 && T_len> 0)
		{
			//four recv
			code =recv(T_socket,T_buf+ cur_len,T_len,MSG_DONTWAIT);
			if (code< 0)
			{
				if (errno== EAGAIN || errno== EWOULDBLOCK)
				{
					Csz::ErrRet("[%s->%d time out=%dus,socket=%d]->failed",__func__,__LINE__,T_time,T_socket);
					return cur_len;
				}
				else if (0== code)
				{
					Csz::ErrMsg("[%s->%d time out=%dus,socket=%d]->failed,peer close",__func__,__LINE__,T_time,T_socket);
					return cur_len;
				}
			}
			else
			{
				cur_len+= code;
				T_len-= code;
			}
		}
        return cur_len;
    }
}
