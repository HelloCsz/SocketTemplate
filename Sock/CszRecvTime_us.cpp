#include "CszSocket.h"
#include <bthread/bthread.h>
#define TIMEOUT300MS (300000)

namespace Csz
{
    int RecvTime_us(int T_socket,char* T_buf,size_t T_len,int T_time)
    {
        if (T_time<= 0)
        {
            Csz::ErrMsg("[Recv Timeus]->failed,time< 0");
            return -1;
        }
        //300ms once
        int time_count= T_time/ TIMEOUT300MS;
        int cur_len= 0;
        int code= 0;
        for (int i= 0; i< time_count && T_len> 0; ++i)
        {
            code =recv(T_socket,T_buf+ cur_len,T_len,MSG_DONTWAIT);
            if (-1== code)
            {
                if (errno== EAGAIN || errno== EWOULDBLOCK)
                {
                    //300ms
                    bthread_usleep(TIMEOUT300MS);
                    continue;
                }
                Csz::ErrRet("[Recv Timeus time out=%d]->failed,",T_time);
                break;
            }
            else if (0== code)
            {
                Csz::ErrMsg("[Recv Timeus time out=%d]->failed,peer close",T_time);
                break;
            }
            cur_len+= code;
            T_len= T_len- code;
        }
        if (0== T_len)
        {
            return 0;
        }
		if (code!= 0)
		{
			//four recv
			code =recv(T_socket,T_buf+ cur_len,T_len,MSG_DONTWAIT);
			if (-1== code)
			{
				if (errno== EAGAIN || errno== EWOULDBLOCK)
				{
					Csz::ErrRet("[Recv Timeus time out=%d]->failed",T_time);
					return -1;
				}
				else if (0== code)
				{
					Csz::ErrMsg("[Recv Timeus time out=%d]->failed,peer close",T_time);
					return -1;
				}
			}
			T_len= T_len- code;
		}
		if (0== T_len)
			return 0;
        return -1;
    }
    int RecvTimeP_us(int T_socket,char* T_buf,size_t* T_len,int T_time)
    {
        if (T_time<= 0)
        {
            Csz::ErrMsg("[Recv Timeus]->failed,time< 0");
            return -1;
        }
        //300ms once
        int time_count= T_time/ TIMEOUT300MS;
        int cur_len= 0;
        int code= 0;
        for (int i= 0; i< time_count && *T_len> 0; ++i)
        {
            code =recv(T_socket,T_buf+ cur_len,*T_len,MSG_DONTWAIT);
            if (-1== code)
            {
                if (errno== EAGAIN || errno== EWOULDBLOCK)
                {
                    //300ms
                    bthread_usleep(TIMEOUT300MS);
                    continue;
                }
                Csz::ErrRet("[Recv Timeus time out=%d]->failed,",T_time);
                break;
            }
            else if (0== code)
            {
                Csz::ErrMsg("[Recv Timeus time out=%d]->failed,peer close",T_time);
                break;
            }
            cur_len+= code;
            *T_len= *T_len- code;
        }
        if (0== *T_len)
        {
            return 0;
        }
		if (code!= 0)
		{
			//four recv
			code =recv(T_socket,T_buf+ cur_len,*T_len,MSG_DONTWAIT);
			if (-1== code)
			{
				if (errno== EAGAIN || errno== EWOULDBLOCK)
				{
					Csz::ErrRet("[Recv Timeus time out=%d]->failed,",T_time);
					return -1;
				}
				else if (0== code)
				{
					Csz::ErrMsg("[Recv Timeus time out=%d]->failed,peer close",T_time);
					return -1;
				}
			}
			*T_len= *T_len- code;
		}
		if (0== *T_len)
			return 0;
        return -1;
    }
}
