#pragma once

#include <iostream>
#include <Windows.h>
#include <chrono>
// Bibliotecas Boost
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/thread.hpp>
// Bibliotecas do projeto
#include "util.hpp"


class memoria_compartilhada_gateway_servidor
{
	string nome = "MC_S_Gateway_Historiador";
	boost::scoped_ptr<shared_memory_object> shm;
public:
	memoria_compartilhada_gateway_servidor()
	{
		struct shm_remove
		{
			shm_remove() { shared_memory_object::remove("MC_S_Gateway_Historiador"); }
			~shm_remove() { shared_memory_object::remove("MC_S_Gateway_Historiador"); }
		} remover;

		shm.reset( new shared_memory_object( open_only, "MC_S_Gateway_Historiador", read_write));

		// Atribui o tamanho da região de memória compartilhada
		(*shm).truncate(sizeof(shared_memory_buffer));

		// Mapeia a região de memória compartilhada nesse processo
		mapped_region region((*shm), read_write);
	}
	~memoria_compartilhada_gateway_servidor() {}
};

shared_memory_buffer* Gateway_Historiador_Comunicacao::criar_semaforo()
{
	// Remove a região de memória compartilhada para controle de acesso a 
	// fila de mensagens entre o Gateway e o Historiador
	struct shm_remove
	{
		shm_remove() { shared_memory_object::remove("MC_S_Gateway_Historiador"); }
		~shm_remove() { shared_memory_object::remove("MC_S_Gateway_Historiador"); }
	} remover;

	// Cria a região de memória compartilhada entre o Gateway e o Historiador
	// para emular semáforo de controle de acesso a lista
	shared_memory_object shm
	(
		create_only,					// Cria a região
		"MC_S_Gateway_Historiador",		// Nome da região
		read_write			       		// Permissão de leitura e escrita
	);

	// Atribui o tamanho da região de memória compartilhada
	shm.truncate(sizeof(shared_memory_buffer));

	// Mapeia a região de memória compartilhada nesse processo
	mapped_region region
	(shm
		, read_write
	);

	// Cria ponteiro com o endereço da região da região
	void * addr = region.get_address();

	// Constrói a região de memória compartilhada 
	shared_memory_buffer * data = new (addr) shared_memory_buffer;

	return data;
}

void thread_procedimento_gateway_historiador()
{
	// Abre a fila de mensagens para a comunicação entre gateway e o historiador
	try {
		message_queue fila_gateway_historiador
			( 
				open_only,					// Apenas abre a fila
				"gateway_historiador"		// Nome da fila
			);

		while (TRUE)
		{
			struct position_t *nova_posicao = new struct position_t();

			fila_gateway_historiador.send(nova_posicao, sizeof(struct position_t), 1);

			boost::this_thread::sleep_for(boost::chrono::seconds(1));
		}
	}
	catch (interprocess_exception &ex) {
		message_queue::remove("message_queue");
		cout << ex.what() << endl;
		return;
	}
	message_queue::remove("message_queue");
}
