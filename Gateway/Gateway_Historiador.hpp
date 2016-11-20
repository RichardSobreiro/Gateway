#ifndef GATEWAY_HISTORIADOR_HPP
#define GATEWAY_HISTORIADOR_HPP

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

using namespace std;
using namespace boost::interprocess;

struct position_t
{
	int id;
	time_t timestamp;
	double latitude;
	double longitude;
	int speed;
};

struct shared_memory_buffer
{
	shared_memory_buffer()
		: mutex(1), numItems(0)
	{}

	// Semaforos para proteger e sincronizar o acesso a lista com o número
	// de elementos na fila
	boost::interprocess::interprocess_semaphore mutex;

	// Variável que controla número de elementos na lista
	int numItems;
};

class Gateway_Historiador_Comunicacao
{
public:

	static shared_memory_buffer* criar_semaforo();

private:
};

void thread_procedimento_gateway_historiador();

// Testes
void gera_position_t_random(struct  position_t* pos,
	int& IDTeste,
	double& LatTest,
	double& LongTest);

void imprime_position_t(struct position_t *nova_posicao);

#endif // !GATEWAY_HISTORIADOR_HPP
