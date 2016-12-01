#ifndef SERVER_HTTP_HPP
#define	SERVER_HTTP_HPP

#include <boost/asio.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash.hpp>

#include <unordered_map>
#include <thread>
#include <functional>
#include <iostream>
#include <sstream>

#include "memoria_compartilhada_gateway_servidor.hpp"
#include "fila_gateway_historiador.hpp"

// Após 2017 a checagem abaixo deve ser removida (atualização do compilador)
#ifdef USE_BOOST_REGEX
#include <boost/regex.hpp>
#define REGEX_NS boost
#else
#include <regex>
#define REGEX_NS std
#endif

namespace SimpleWeb 
{
	#pragma region CLASS ServerBase

	template <class socket_type>
	class ServerBase 
	{
	public:
		virtual ~ServerBase() {}

		class Response : public std::ostream 
		{
			friend class ServerBase<socket_type>;

			boost::asio::streambuf streambuf;

			std::shared_ptr<socket_type> socket;

			Response(const std::shared_ptr<socket_type> &socket) : std::ostream(&streambuf), socket(socket) {}

			public:
				size_t size() 
				{
					return streambuf.size();
				}
		};

		class Content : public std::istream 
		{
			friend class ServerBase<socket_type>;
			
		public:
			size_t size() 
			{
				return streambuf.size();
			}
			std::string string() 
			{
				std::stringstream ss;
				ss << rdbuf();
				return ss.str();
			}
		private:
			boost::asio::streambuf &streambuf;
			Content(boost::asio::streambuf &streambuf) : std::istream(&streambuf), streambuf(streambuf) {}
		};

		class Request 
		{
			friend class ServerBase<socket_type>;

			//Based on http://www.boost.org/doc/libs/1_60_0/doc/html/unordered/hash_equality.html
			class iequal_to 
			{
			public:
				bool operator()(const std::string &key1, const std::string &key2) const 
				{
					return boost::algorithm::iequals(key1, key2);
				}
			};

			class ihash 
			{
			public:
				size_t operator()(const std::string &key) const 
				{
					std::size_t seed = 0;
					for (auto &c : key)
						boost::hash_combine(seed, std::tolower(c));
					return seed;
				}
			};
		public:
			std::string method, path, http_version;

			Content content;

			std::unordered_multimap<std::string, std::string, ihash, iequal_to> header;

			REGEX_NS::smatch path_match;

			std::string remote_endpoint_address;
			unsigned short remote_endpoint_port;

		private:
			Request() : content(streambuf) {}

			boost::asio::streambuf streambuf;
		};

		class Config 
		{
			friend class ServerBase<socket_type>;

			Config(unsigned short port, size_t num_threads) : num_threads(num_threads), port(port), reuse_address(true) {}

			size_t num_threads;
		public:
			unsigned short port;
			// IPv4: decimal
			// IPv6: hexadecimal
			std::string address;
			
			// Falso para evitar que o socket seja ligado a um endereço que já está em uso
			bool reuse_address;
		};

		// Setar antes de chamar start() no servidor
		Config config;

		std::unordered_map<std::string, std::unordered_map<std::string,
			std::function<void(std::shared_ptr<typename ServerBase<socket_type>::Response>, 
								std::shared_ptr<typename ServerBase<socket_type>::Request>)> > >  
				resource;

		std::unordered_map<std::string,
			std::function<void(std::shared_ptr<typename ServerBase<socket_type>::Response>, 
								std::shared_ptr<typename ServerBase<socket_type>::Request>)> >
			default_resource;

		std::function<void(const std::exception&)> exception_handler;

	private:
		std::vector<std::pair<std::string, std::vector<std::pair<REGEX_NS::regex,
			std::function<void( std::shared_ptr<typename ServerBase<socket_type>::Response>, 
			std::shared_ptr<typename ServerBase<socket_type>::Request>)>>>>> 
					opt_resource;

	public:
		void start() 
		{
			// Copia os recursos para opt_resource para maior eficiência
			// no processamento da requisição
			opt_resource.clear();

			for (auto& res : resource) 
			{
				for (auto& res_method : res.second) 
				{
					auto it = opt_resource.end();
					for (auto opt_it = opt_resource.begin(); opt_it != opt_resource.end(); opt_it++) 
					{
						if (res_method.first == opt_it->first) 
						{
							it = opt_it;
							break;
						}
					}
					if (it == opt_resource.end()) 
					{
						opt_resource.emplace_back();
						it = opt_resource.begin() + (opt_resource.size() - 1);
						it->first = res_method.first;
					}
					it->second.emplace_back(REGEX_NS::regex(res.first), res_method.second);
				}
			}

			if (!io_service)
				io_service = std::make_shared<boost::asio::io_service>();

			if (io_service->stopped())
				io_service->reset();

			boost::asio::ip::tcp::endpoint endpoint;

			if (config.address.size() > 0)
				endpoint = 
					boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(config.address), 
													config.port);
			else
				endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), config.port);

			if (!acceptor)
				acceptor = std::unique_ptr<boost::asio::ip::tcp::acceptor>
									(new boost::asio::ip::tcp::acceptor(*io_service));

			acceptor->open(endpoint.protocol());
			acceptor->set_option(boost::asio::socket_base::reuse_address(config.reuse_address));
			acceptor->bind(endpoint);
			acceptor->listen();

			accept();

			// Se o número de threads é maior que 1:
			// m_io_service.run() com (num_threads-1) threads para o thread-pooling
			threads.clear();
			for (size_t c = 1; c < config.num_threads; c++) 
			{
				threads.emplace_back([this]() 
				{
					io_service->run();
				});
			}

			// Thread principal
			if (config.num_threads>0)
				io_service->run();

			// Aguarda por todas as threads antes de finalizar
			for (auto& t : threads) 
			{
				t.join();
			}
		}

		void stop() 
		{
			acceptor->close();
			if (config.num_threads>0) io_service->stop();
		}

		// Função para enviar partes de uma grande mensagem recursivamente
		void send(const std::shared_ptr<Response> &response, 
			const std::function<void(const boost::system::error_code&)>& callback = nullptr) const 
		{
			boost::asio::async_write(*response->socket, response->streambuf, 
				[this, response, callback](const boost::system::error_code& ec, size_t /*bytes transferidos*/) 
			{
				if (callback)
					callback(ec);
			});
		}

		// Em caso de uso de uma io_service o endereço da mesma deve ser armazenado aqui
		// antes do start() no servidor ser executado
		std::shared_ptr<boost::asio::io_service> io_service;
	protected:
		std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;
		std::vector<std::thread> threads;

		long timeout_request;
		long timeout_content;

		ServerBase(unsigned short port, size_t num_threads, long timeout_request, long timeout_send_or_receive) :
			config(port, num_threads), timeout_request(timeout_request), timeout_content(timeout_send_or_receive) {}

		virtual void accept() = 0;

		std::shared_ptr<boost::asio::deadline_timer> 
			set_timeout_on_socket(const std::shared_ptr<socket_type> &socket, long seconds) 
		{
			std::shared_ptr<boost::asio::deadline_timer> timer(new boost::asio::deadline_timer(*io_service));
			timer->expires_from_now(boost::posix_time::seconds(seconds));
			timer->async_wait([socket](const boost::system::error_code& ec) 
			{
				if (!ec) 
				{
					boost::system::error_code ec;
					socket->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
					socket->lowest_layer().close();
				}
			});
			return timer;
		}

		void read_request_and_content(const std::shared_ptr<socket_type> &socket) 
		{
			std::shared_ptr<Request> request(new Request());
			try 
			{
				request->remote_endpoint_address = socket->lowest_layer().remote_endpoint().address().to_string();
				request->remote_endpoint_port = socket->lowest_layer().remote_endpoint().port();
			}
			catch (const std::exception &e) 
			{
				if (exception_handler)
					exception_handler(e);
			}

			std::shared_ptr<boost::asio::deadline_timer> timer;

			if (timeout_request > 0)
				timer = set_timeout_on_socket(socket, timeout_request);

			boost::asio::async_read_until(*socket, request->streambuf, "\r\n\r\n",
				[this, socket, request, timer](const boost::system::error_code& ec, size_t bytes_transferred) 
			{
				if (timeout_request > 0)
					timer->cancel();
				if (!ec) 
				{
					size_t num_additional_bytes = request->streambuf.size() - bytes_transferred;

					if (!parse_request(request, request->content))
						return;

					auto it = request->header.find("Content-Length");
					if (it != request->header.end()) 
					{
						std::shared_ptr<boost::asio::deadline_timer> timer;
						if (timeout_content>0)
							timer = set_timeout_on_socket(socket, timeout_content);
						unsigned long long content_length;
						try 
						{
							content_length = stoull(it->second);
						}
						catch (const std::exception &e) 
						{
							if (exception_handler)
								exception_handler(e);
							return;
						}
						if (content_length>num_additional_bytes) 
						{
							boost::asio::async_read(*socket, request->streambuf,
								boost::asio::transfer_exactly(content_length - num_additional_bytes),
								[this, socket, request, timer]
							(const boost::system::error_code& ec, size_t /*bytes_transferred*/) 
							{
								if (timeout_content>0)
									timer->cancel();
								if (!ec)
									find_resource(socket, request);
							});
						}
						else 
						{
							if (timeout_content>0)
								timer->cancel();
							find_resource(socket, request);
						}
					}
					else 
					{
						find_resource(socket, request);
					}
				}
				else
				{

				}
			});
		}

		bool parse_request(const std::shared_ptr<Request> &request, std::istream& stream) const 
		{
			std::string line;
			getline(stream, line);
			size_t method_end;
			if ((method_end = line.find(' ')) != std::string::npos) 
			{
				size_t path_end;
				if ((path_end = line.find(' ', method_end + 1)) != std::string::npos) 
				{
					request->method = line.substr(0, method_end);
					request->path = line.substr(method_end + 1, path_end - method_end - 1);

					size_t protocol_end;
					if ((protocol_end = line.find('/', path_end + 1)) != std::string::npos) 
					{
						if (line.substr(path_end + 1, protocol_end - path_end - 1) != "HTTP")
							return false;
						request->http_version = line.substr(protocol_end + 1, line.size() - protocol_end - 2);
					}
					else
						return false;

					getline(stream, line);
					size_t param_end;
					while ((param_end = line.find(':')) != std::string::npos) 
					{
						size_t value_start = param_end + 1;
						if ((value_start)<line.size()) 
						{
							if (line[value_start] == ' ')
								value_start++;
							if (value_start<line.size())
								request->header.insert(std::make_pair(line.substr(0, param_end), line.substr(value_start, line.size() - value_start - 1)));
						}

						getline(stream, line);
					}
				}
				else
					return false;
			}
			else
				return false;
			return true;
		}

		void find_resource(const std::shared_ptr<socket_type> &socket, const std::shared_ptr<Request> &request) 
		{
			//Find path- and method-match, and call write_response
			for (auto& res : opt_resource) 
			{
				if (request->method == res.first) 
				{
					for (auto& res_path : res.second) 
					{
						REGEX_NS::smatch sm_res;
						if (REGEX_NS::regex_match(request->path, sm_res, res_path.first)) 
						{
							request->path_match = std::move(sm_res);
							write_response(socket, request, res_path.second);
							return;
						}
					}
				}
			}

			auto it_method = default_resource.find(request->method);

			if (it_method != default_resource.end()) 
			{
				write_response(socket, request, it_method->second);
			}
		}

		void write_response(const std::shared_ptr<socket_type> &socket, const std::shared_ptr<Request> &request,
			std::function<void(std::shared_ptr<typename ServerBase<socket_type>::Response>,
				std::shared_ptr<typename ServerBase<socket_type>::Request>)>& resource_function) 
		{
			//Set timeout on the following boost::asio::async-read or write function
			std::shared_ptr<boost::asio::deadline_timer> timer;
			if (timeout_content>0)
				timer = set_timeout_on_socket(socket, timeout_content);

			auto response = std::shared_ptr<Response>(new Response(socket), [this, request, timer](Response *response_ptr) 
			{
				auto response = std::shared_ptr<Response>(response_ptr);
				send(response, [this, response, request, timer](const boost::system::error_code& ec) 
				{
					if (!ec) 
					{
						if (timeout_content>0)
							timer->cancel();
						float http_version;
						try 
						{
							http_version = stof(request->http_version);
						}
						catch (const std::exception &e) 
						{
							if (exception_handler)
								exception_handler(e);
							return;
						}

						auto range = request->header.equal_range("Connection");
						for (auto it = range.first; it != range.second; it++) 
						{
							if (boost::iequals(it->second, "close"))
								return;
						}
						if (http_version>1.05)
							read_request_and_content(response->socket);
					}
				});
			});

			try {
				resource_function(response, request);
			}
			catch (const std::exception &e) {
				if (exception_handler)
					exception_handler(e);
				return;
			}
		}
	};

	#pragma endregion CLASS ServerBase

	# pragma region CLASS Server
	template<class socket_type>
	class Server : public ServerBase<socket_type> {};
	# pragma endregion CLASS Server

	# pragma region CLASS Server HTTP
	typedef boost::asio::ip::tcp::socket HTTP;

	template<>
	class Server<HTTP> : public ServerBase<HTTP> 
	{
	public:

		Memoria_Compartilhada_Gateway_Servidor memoria_compartilhada_gateway_servidor;
		Fila_Gateway_Historiador fila_gateway_historiador;

		Server( unsigned short port, 
				size_t num_threads = 1, 
				long timeout_request = 5, 
				long timeout_content = 300
		) : ServerBase<HTTP>::ServerBase (
				port, 
				num_threads, 
				timeout_request, 
				timeout_content
			) {}


	protected:
		void accept() 
		{
			// Cria novo socket para a nova conexão
			// Shared_ptr é usado para passar objetos temporários para as funções 
			// assíncronas
			std::shared_ptr<HTTP> socket(new HTTP(*io_service));

			acceptor->async_accept(*socket, [this, socket](const boost::system::error_code& ec) 
			{
				// Começa a aceitar novas conexões se io_service não foi interrompida
				if (ec != boost::asio::error::operation_aborted)
					accept();

				if (!ec) 
				{
					boost::asio::ip::tcp::no_delay option(true);
					socket->set_option(option);

					read_request_and_content(socket);
				}
			});
		}
	};

	# pragma endregion CLASS Server HTTP
}
#endif	/* SERVER_HTTP_HPP */