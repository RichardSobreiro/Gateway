#ifndef GATEWAY_SERVIDOR_HPP
#define GATEWAY_SERVIDOR_HPP

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

using namespace std;
using namespace boost::interprocess;

struct active_users_t
{
	active_users_t() : num_active_users(0)
	{
		for (unsigned i = 0; i < LIST_SIZE; ++i)
		{
			list[i].id = -1;
		}
	}
	int num_active_users;
	enum { LIST_SIZE = 1000000 };
	position_t list[LIST_SIZE];
	boost::interprocess::interprocess_mutex mutex;
};

void thread_procedimento_gateway_servidor();

#endif // !GATEWAY_SERVIDOR_HPP

