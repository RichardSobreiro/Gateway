#ifndef GATEWAY_HISTORIADOR_INCLUDE
#define GATEWAY_HISTORIADOR_INCLUDE

#include <boost/interprocess/sync/interprocess_semaphore.hpp>

struct position_t
{
	int id;
	time_t timestamp;
	double latitude;
	double longitude;
	int speed;

	template <typename Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & id;
		ar & timestamp;
		ar & latitude;
		ar & longitude;
		ar & speed;
	}
};

struct shared_memory_buffer
{
	shared_memory_buffer()
		: mutex(1), numItems(0)
	{}

	// Semaforos para proteger e sincronizar o acesso a lista com o n�mero
	// de elementos na fila
	boost::interprocess::interprocess_semaphore
		mutex;

	// Vari�vel que controla n�mero de elementos na lista
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

#endif // !GATEWAY_HISTORIADOR_INCLUDE
