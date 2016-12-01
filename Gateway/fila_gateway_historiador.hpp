#pragma once

#include <iostream>
#include <string>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/scoped_ptr.hpp>

#include "util.hpp"

using namespace std;
using namespace boost::interprocess;


class Fila_Gateway_Historiador
{
	string nome = "gateway_historiador";
	boost::scoped_ptr<message_queue> mq;
public:
	Fila_Gateway_Historiador () 
	{
		mq.reset(new message_queue(open_only, nome.c_str()));
	}
	~Fila_Gateway_Historiador() 
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
		size_t msg_size(0);
		unsigned int priority(1);

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