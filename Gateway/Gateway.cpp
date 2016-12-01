#include <iostream>
#include <fstream>
#include <vector>

#include <boost/thread.hpp>

#include "Server_Http.hpp"
#include "fila_gateway_historiador.hpp"
#include "memoria_compartilhada_gateway_servidor.hpp"
#include "util.hpp"

using namespace std;

typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;

int main() {
	// Servidor http na porta 5055
	HttpServer server(5055, 1);

	server.default_resource["GET"] =
		[&server](std::shared_ptr<HttpServer::Response> response, 
			std::shared_ptr<HttpServer::Request> request)
	{
		try
		{
			struct position_t *nova_posicao = new struct position_t();
			vector<string> args;

			get_args(request->path, args);

			nova_posicao = preenche_position_t(args);
			server.fila_gateway_historiador.write_position_t(*nova_posicao);
			server.memoria_compartilhada_gateway_servidor.atualiza_posicao(*nova_posicao);

			*response << "HTTP/1.1 200 OK\r\nContent-Length: 0 \r\n Keep-Alive: timeout=15,max=100\r\n\r\n";
		}
		catch (const std::exception &e)
		{
			cerr << "Erro: " << e.what() << endl;
		}

	};

	thread server_thread([&server]()
	{
		server.start();
	});

	server_thread.join();

	return 0;
}
