#include "CszBitTorrent.h"

namespace Csz
{
	TrackerInfo::TrackerInfo(TrackerInfo&& T_data)
	{
		host.assign(std::move(T_data.host));
		serv.assign(std::move(T_data.serv));
		uri.assign(std::move(T_data.uri));
		socket_fd= T_data.socket_fd;
	}
};
