#include <iostream>
#include <fstream>
#include <vector>

#include "Server_Http.hpp"
#include "fila_gateway_historiador.hpp"
#include "util.hpp"

using namespace std;

typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;

// Adicionado para o exemplo padrão

void default_resource_send(const HttpServer &server,
	const std::shared_ptr<HttpServer::Response> &response,
	const std::shared_ptr<ifstream> &ifs);

int main() {
	// Servidor http na porta 5055 com apenas uma thread
	HttpServer server(5055, 1);

	server.default_resource["GET"] =
		[&server](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request)
	{
		try
		{
			fila_gateway_historiador fila;
			vector<string> args;

			get_args(request->path, args);
			
			imprime_position_t(preenche_posiont_t(args));

			fila.write_position_t(preenche_posiont_t(args));

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

void default_resource_send(const HttpServer &server, const std::shared_ptr<HttpServer::Response> &response,
	const std::shared_ptr<ifstream> &ifs)
{
	//read and send 128 KB at a time
	static vector<char> buffer(131072); // Safe when server is running on one thread
	streamsize read_length;
	if ((read_length = ifs->read(&buffer[0], buffer.size()).gcount()) > 0)
	{
		response->write(&buffer[0], read_length);

		if (read_length == static_cast<streamsize>(buffer.size()))
		{
			server.send(response, [&server, response, ifs](const boost::system::error_code &ec)
			{
				if (!ec)
					default_resource_send(server, response, ifs);
				else
					cerr << "Conexão interrompida..." << endl;
			});
		}
	}
}