#pragma once

#include <iostream>

#include <boost/regex.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

using namespace std;
using namespace boost::interprocess;

struct position_t
{
	int id;
	std::time_t timestamp;
	double latitude;
	double longitude;
	int speed;
};

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

bool get_args(string msg, vector<string>& values);

struct position_t* preenche_position_t(vector<string> &args);

void imprime_position_t(struct position_t& nova_posicao);
