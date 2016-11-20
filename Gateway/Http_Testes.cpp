#include "Server_Http.hpp"

//Added for the json-example
#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

//Added for the default_resource example
#include <fstream>
#include <boost/filesystem.hpp>
#include <vector>
#include <algorithm>

using namespace std;

typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;

// Adicionado para o exemplo padrão
void default_resource_send( const HttpServer &server, 
							const shared_ptr<HttpServer::Response> &response,
							const shared_ptr<ifstream> &ifs );

int main() {
	// Servidor http na porta 5055 com apenas uma thread
	HttpServer server(5055, 1);

	server.default_resource["GET"] = 
		[&server]( shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request ) 
	{
		try 
		{ 
			auto ifs = make_shared<ifstream>();

			ifs->seekg(0, ios::end);
			auto length = ifs->tellg();

			ifs->seekg(0, ios::beg);

			// Recupera a string enviada pelo aplicatico TRACCAR
			//auto content = request->content.string();
			stringstream ss;
			ss << request->path;
			string content = ss.str();

			cout << content << endl;

			*response << "HTTP/1.1 200 OK\r\nContent-Length: " << length << "\r\n\r\n";
			
		}
		catch (const exception &e) 
		{
			cerr << "Erro: " << e.what() << endl;
		}
	};

	thread server_thread([&server]() 
	{
		//Start server
		server.start();
	});

	//Wait for server to start so that the client can connect
	//this_thread::sleep_for(chrono::seconds(1));
	
	server_thread.join();

	system("PAUSE");
	return 0;
}

void default_resource_send(const HttpServer &server, const shared_ptr<HttpServer::Response> &response,
	const shared_ptr<ifstream> &ifs) 
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