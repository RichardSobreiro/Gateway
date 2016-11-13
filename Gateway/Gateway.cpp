#include <iostream>
// Bibliotecas Boost
#include <boost/thread.hpp>
// Bibliotecas do projeto
#include "gateway_historiador.h"

using namespace std;

int main()
{
	boost::thread thread_gateway_historiador{ thread_procedimento_gateway_historiador };

	thread_gateway_historiador.join();

	system("PAUSE");
	return 0;
}