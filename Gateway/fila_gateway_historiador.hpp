#pragma once

#include <iostream>
#include <string>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/scoped_ptr.hpp>

#include "util.hpp"

using namespace std;
using namespace boost::interprocess;


class fila_gateway_historiador
{
	string nome = "gateway_historiador";
	boost::scoped_ptr<message_queue> mq;
public:
	fila_gateway_historiador () 
	{
		mq.reset(new message_queue(open_only, nome.c_str()));
	}
	~fila_gateway_historiador() 
	{
		message_queue::remove(nome.c_str());
	}

	void write_position_t(struct position_t &nova_posicao)
	{
		try
		{
		
			(*mq).send(&nova_posicao, sizeof(nova_posicao), 1);
		
		}
		catch (interprocess_exception &ex)
		{
			message_queue::remove(nome.c_str());
			cout << ex.what() << endl;
			exit(0);
		}
		catch (const std::exception &e)
		{
			cerr << "Erro: " << e.what() << endl;
			exit(0);
		}
	}

	size_t read_position_t(struct position_t &item)
	{
		size_t msg_size;
		unsigned int priority;

		try
		{ 
			(*mq).receive(&item, sizeof(item), msg_size, priority);
		}
		catch (interprocess_exception &ex)
		{
			message_queue::remove(nome.c_str());
			cout << ex.what() << endl;
			exit(0);
		}
		catch (const std::exception &e)
		{
			cerr << "Erro: " << e.what() << endl;
			exit(0);
		}
	}
};

	//#endif // !FILA_TEMPLATE_HPP