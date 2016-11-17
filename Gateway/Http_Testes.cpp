#include "Server_Http.hpp"
#include "Cliente_Http.hpp"

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
//Added for the json-example:
using namespace boost::property_tree;

typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;
typedef SimpleWeb::Client<SimpleWeb::HTTP> HttpClient;

// Adicionado para o exemplo padr�o
void default_resource_send(const HttpServer &server, const shared_ptr<HttpServer::Response> &response,
	const shared_ptr<ifstream> &ifs);

int main() {
	// Servidor http na porta 8080 com apenas uma thread
	HttpServer server(8080, 1);

	// Add recursos: path-regex/method-string e fun��o an�nima/virtual
	//POST-example para o caminho /string, responde a string postada
	server.resource["^/string$"]["POST"] = []
		(shared_ptr<HttpServer::Response> response, 
		shared_ptr<HttpServer::Request> request) 
	{
		auto content = request->content.string();
		//stringstream ss;
		//ss << request->content.rdbuf();
		//string content=ss.str();

		*response << "HTTP/1.1 200 OK\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
	};

	//POST-example for the path /json, responds firstName+" "+lastName from the posted json
	//Responds with an appropriate error message if the posted json is not valid, or if firstName or lastName is missing
	//Example posted json:
	//{
	//  "firstName": "John",
	//  "lastName": "Smith",
	//  "age": 25
	//}
	server.resource["^/json$"]["POST"] = 
		[](
		shared_ptr<HttpServer::Response> response, 
		shared_ptr<HttpServer::Request> request
		) 
	{
		try 
		{
			ptree pt;
			read_json(request->content, pt);

			string name = pt.get<string>("firstName") + " " + pt.get<string>("lastName");

			*response << "HTTP/1.1 200 OK\r\n"
				<< "Content-Type: application/json\r\n"
				<< "Content-Length: " << name.length() << "\r\n\r\n"
				<< name;
		}
		catch (exception& e) 
		{
			*response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << strlen(e.what()) << "\r\n\r\n" << e.what();
		}
	};

	server.resource["^/info$"]["GET"] = 
		[](
			shared_ptr<HttpServer::Response> response, 
			shared_ptr<HttpServer::Request> request
		) 
	{
		stringstream content_stream;
		content_stream << "<h1>Request from " << request->remote_endpoint_address 
			<< " (" << request->remote_endpoint_port << ")</h1>";
		content_stream << request->method << " " << request->path 
			<< " HTTP/" << request->http_version << "<br>";

		for (auto& header : request->header) 
		{
			content_stream << header.first << ": " << header.second << "<br>";
		}

		// Obtem o comprimento do content_stream (content_stream.tellp())
		content_stream.seekp(0, ios::end);

		*response << "HTTP/1.1 200 OK\r\nContent-Length: " << content_stream.tellp() << "\r\n\r\n" << content_stream.rdbuf();
	};

	server.resource["^/match/([0-9]+)$"]["GET"] = [&server]
		(
			shared_ptr<HttpServer::Response> response, 
			shared_ptr<HttpServer::Request> request
		)
	{
		string number = request->path_match[1];
		*response << "HTTP/1.1 200 OK\r\nContent-Length: " << number.length() << "\r\n\r\n" << number;
	};

	// Thread executa trabalho pesado
	server.resource["^/work$"]["GET"] = 
		[&server]
		(
			shared_ptr<HttpServer::Response> response, 
			shared_ptr<HttpServer::Request> /*request*/
		) 
	{
		thread work_thread([response] 
		{
			this_thread::sleep_for(chrono::seconds(5));
			string message = "Work done";
			*response << "HTTP/1.1 200 OK\r\nContent-Length: " << message.length() << "\r\n\r\n" << message;
		});
		work_thread.detach();
	};

	server.default_resource["GET"] = 
		[&server]
	(
		shared_ptr<HttpServer::Response> response, 
		shared_ptr<HttpServer::Request> request
	) 
	{
		try 
		{
			auto web_root_path = boost::filesystem::canonical("web");
			auto path = boost::filesystem::canonical(web_root_path / request->path);
			// Checa se o caminho esta nos caminhos do servidor
			if (distance(web_root_path.begin(), web_root_path.end())>
				distance(path.begin(), path.end()) ||
					!equal(web_root_path.begin(), 
					web_root_path.end(), path.begin())
				) throw invalid_argument("path must be within root path");

			if (boost::filesystem::is_directory(path))
				path /= "index.html";
			if (!(boost::filesystem::exists(path) && boost::filesystem::is_regular_file(path)))
				throw invalid_argument("file does not exist");

			auto ifs = make_shared<ifstream>();
			ifs->open(path.string(), ifstream::in | ios::binary);

			if (*ifs) 
			{
				ifs->seekg(0, ios::end);
				auto length = ifs->tellg();

				ifs->seekg(0, ios::beg);

				*response << "HTTP/1.1 200 OK\r\nContent-Length: " << length << "\r\n\r\n";
				default_resource_send(server, response, ifs);
			}
			else
				throw invalid_argument("could not read file");
		}
		catch (const exception &e) 
		{
			string content = "Could not open path " + request->path + ": " + e.what();
			*response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
		}
	};

	thread server_thread([&server]() 
	{
		//Start server
		server.start();
	});

	//Wait for server to start so that the client can connect
	this_thread::sleep_for(chrono::seconds(1));

	//Client examples
	HttpClient client("localhost:8080");
	auto r1 = client.request("GET", "/match/123");
	cout << r1->content.rdbuf() << endl;

	string json_string = "{\"firstName\": \"John\",\"lastName\": \"Smith\",\"age\": 25}";
	auto r2 = client.request("POST", "/string", json_string);
	cout << r2->content.rdbuf() << endl;

	auto r3 = client.request("POST", "/json", json_string);
	cout << r3->content.rdbuf() << endl;

	server_thread.join();

	return 0;
}

void default_resource_send(const HttpServer &server, const shared_ptr<HttpServer::Response> &response,
	const shared_ptr<ifstream> &ifs) {
	//read and send 128 KB at a time
	static vector<char> buffer(131072); // Safe when server is running on one thread
	streamsize read_length;
	if ((read_length = ifs->read(&buffer[0], buffer.size()).gcount())>0) {
		response->write(&buffer[0], read_length);
		if (read_length == static_cast<streamsize>(buffer.size())) {
			server.send(response, [&server, response, ifs](const boost::system::error_code &ec) {
				if (!ec)
					default_resource_send(server, response, ifs);
				else
					cerr << "Connection interrupted" << endl;
			});
		}
	}
}