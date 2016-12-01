#pragma once

#include <iostream>
#include <list>
// Bibliotecas Boost
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/thread.hpp>
// Bibliotecas do projeto
#include "util.hpp"

class Memoria_Compartilhada_Gateway_Servidor
{
public:

	string nome = "memoria_compartilhada_gateway_servidor";
	boost::scoped_ptr<shared_memory_object> shm;
	boost::scoped_ptr<mapped_region> region;
	void *addr;
	struct active_users_t *usuarios_ativos;

	Memoria_Compartilhada_Gateway_Servidor()
	{
		shm.reset(new shared_memory_object(open_only, nome.c_str(), read_write));

		// Mapeia a região de memória compartilhada nesse processo
		region.reset(new mapped_region((*shm), read_write));

		addr = (*region).get_address();

		usuarios_ativos = static_cast<struct active_users_t*>(addr);

	}
	~Memoria_Compartilhada_Gateway_Servidor() {}

	void atualiza_posicao(struct position_t& nova_posicao)
	{
		usuarios_ativos->mutex.lock();

		if(usuarios_ativos->list[nova_posicao.id].id == -1) usuarios_ativos->num_active_users++;

		usuarios_ativos->list[nova_posicao.id].id = nova_posicao.id;
		usuarios_ativos->list[nova_posicao.id].timestamp = nova_posicao.timestamp;
		usuarios_ativos->list[nova_posicao.id].longitude = nova_posicao.latitude;
		usuarios_ativos->list[nova_posicao.id].latitude = nova_posicao.longitude;
		usuarios_ativos->list[nova_posicao.id].speed = nova_posicao.speed;

		usuarios_ativos->mutex.unlock();
	}

	void desconecta_usuario(int id)
	{
		struct active_users_t *usuarios_ativos = static_cast<struct active_users_t *>((*region).get_address());

		usuarios_ativos->mutex.lock();

		usuarios_ativos->list[id].id = -1;

		usuarios_ativos->num_active_users--;

		usuarios_ativos->mutex.unlock();
	}
};

void thread_procedimento_gateway_historiador(struct position_t nova_posicao , bool status)
{
	Memoria_Compartilhada_Gateway_Servidor mc;
	//struct position_t *nova_posicao = static_cast<struct position_t *>(p);

	if (!status)
	{
		mc.desconecta_usuario(nova_posicao.id);
	}
	else
	{
		mc.atualiza_posicao(nova_posicao);
	}

}
