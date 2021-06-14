#define ASIO_STANDALONE
#include <asio.hpp>
#include <iostream>
#include <thread>
#include <mutex>

std::mutex iosafe;
asio::io_context io;
bool exitf = false;
//Use this to tell sendMessage when to write message, also need something for who
size_t writebytes = 0;
std::vector<std::string> messages;
std::vector<asio::ip::tcp::socket*> sockets;

void sendMessage(asio::ip::tcp::socket *sock, std::string msg) {
	asio::error_code ec;
	asio::write(*sock, asio::buffer(msg.c_str(), msg.size()), ec);
}
//When writing or reading data you need data size to make sure not stuck forever reading or writing
void recieveMessage(asio::ip::tcp::socket *sock) {
	asio::error_code ec;
	char buf[128];
	size_t bytes = (*sock).available();
	std::cout << "Bytes: " << bytes << " Peer: " << sock->remote_endpoint() << std::endl;
	if (bytes > 0) {
		asio::read(*sock, asio::buffer(buf, bytes), ec);
		messages.push_back(buf);
		std::cout << "Message recieved from: " << sock->remote_endpoint() << std::endl;
		if (ec == asio::error::eof) {
			return;
		}
		else if (ec) {
			std::cout << "Writing error trying again: " << ec.message() << std::endl;
			return;
		}
	}
}
//System works but need to figure out how to stop blocking
//Stop blocking on listen so actual main loop can run and hand out data
void listeningThread() {
	while (!exitf) {
		if (iosafe.try_lock()) {
			asio::error_code ec;
			asio::ip::tcp::acceptor accept(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 13));
			asio::ip::tcp::socket* sock = new asio::ip::tcp::socket(io);
			accept.listen();
			accept.accept(*sock, ec);
			sockets.push_back((sock));
			accept.close();
			std::cout << "New connection from: " << sock->remote_endpoint() << std::endl;
			iosafe.unlock();
		}
	}
}


//I think socket is closed everytime accept is called again, and since client doesnt connect again no data passed
//https://www.boost.org/doc/libs/1_35_0/doc/html/boost_asio/tutorial/tutdaytime3.html
int main() {
	/*asio::ip::tcp::acceptor accept(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 13));
	asio::ip::tcp::socket sock(io);
	asio::error_code ec;
	accept.accept(sock, ec);*/
	std::thread lthread(listeningThread);
	while (!exitf) {
		if (iosafe.try_lock()) {
			for (auto& sock : sockets) {
				for (int i = 0; i < messages.size(); i++) {
					sendMessage(sock, messages[i]);
				}
				messages.clear();
				recieveMessage(sock);
			}
			std::cout << "Looped" << sockets.size() << std::endl;
			iosafe.unlock();
		}
	}
	lthread.join();
	return 0;
}