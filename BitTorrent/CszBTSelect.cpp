#include "CszBitTorrent.h"

namespace Csz
{
    void CszBtSelect::operator()(std::vector<int>&& T_socket_queue) const
    {
        fd_set wset,rset;
        fd_set wset_save,rset_save;
        int fd_max= -1;
        struct timeval time_val;
        
    }
}
