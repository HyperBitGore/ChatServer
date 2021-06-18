#include <iostream>
#include <thread>
#define ASIO_STANDALONE
#include <asio.hpp>
bool exitf = false;
void recieveMessages(asio::ip::tcp::socket *soc) {
	char buf[128];
	size_t bytes = (*soc).available();
	if (bytes > 0) {
		asio::error_code ec;
		size_t len = asio::read(*soc, asio::buffer(buf, bytes), ec);
		std::string out = buf;
		for (int i = 0; i < out.size(); i++) {
			if (out[i] <= 255 && out[i] >= 1) {
				std::cout << out[i];
			}
		}
		std::cout << std::endl;
		if (ec == asio::error::eof) {
			return;
		}
		else if (ec) {
			std::cout << "Reading error trying again" << ec.message() << std::endl;
			return;
		}
	}
}
void sendMessage(std::string name, std::string msg, asio::ip::tcp::socket *sock) {
	asio::error_code ec;
	std::string message = name + ":" + msg;
	asio::write(*sock, asio::buffer(message, message.length()), ec);
	if (ec == asio::error::eof) {
		return;
	}
	else if (ec) {
		std::cout << "Connection error trying again: " << ec.message() << std::endl;
		return;
	}
}
void sendT(asio::ip::tcp::socket* sock, std::string name) {
	while (!exitf) {
		char message[128];
		std::cin.getline(message, 128);
		sendMessage(name, message, sock);
		std::cin.clear();
	}
}

//switch over to using streams so data is continous
int main() {
	std::string name;
	std::cout << "Input Name: ";
	std::cin >> name;
	std::cin.clear();
	asio::io_context io;
	asio::ip::tcp::resolver resolv(io);
	std::string ipstring;
	std::cout << "Input IP of server: ";
	std::cin >> ipstring;
	std::cin.clear();
	unsigned short port = 13;
	asio::ip::address ip = asio::ip::address::from_string(ipstring);
	asio::ip::tcp::endpoint end(ip, port);
	asio::ip::tcp::resolver::results_type endpoints = resolv.resolve(end);
	asio::ip::tcp::socket sock(io);
	asio::error_code ef;
	sock.connect(*endpoints, ef);
	std::thread send(sendT, &sock, name);
	while (!exitf) {
		recieveMessages(&sock);
	}
	send.join();
	sock.close();
	return 0;
}