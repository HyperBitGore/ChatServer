#define ASIO_STANDALONE
#include <asio.hpp>
#include <iostream>
#include <thread>
#include <mutex>

std::mutex iosafe;
asio::io_context io;
bool exitf = false;
struct Msg {
	std::string body;
	asio::ip::tcp::socket* sock;
	size_t bytes;
};
std::vector<Msg> messages;
std::vector<asio::ip::tcp::socket*> sockets;

void sendMessage(asio::ip::tcp::socket *sock, std::string msg, size_t bytes) {
	asio::error_code ec;
	std::cout << "Writing to " << sock->remote_endpoint() << " With " << bytes << " Bytes" << std::endl;
	asio::write(*sock, asio::buffer(msg.c_str(), bytes), ec);
}
//When writing or reading data you need data size to make sure not stuck forever reading or writing
bool recieveMessage(asio::ip::tcp::socket *sock) {
	asio::error_code ec;
	char buf[128];
	size_t bytes = (*sock).available();
	if (bytes > 0) {
		asio::read(*sock, asio::buffer(buf, bytes), ec);
		std::cout << "Bytes: " << bytes << " Peer: " << sock->remote_endpoint() << std::endl;
		Msg m;
		m.body = buf;
		m.sock = sock;
		m.bytes = bytes;
		messages.push_back(m);
		std::cout << "Message recieved from: " << sock->remote_endpoint() << std::endl;
		if (ec == asio::error::eof) {
			return false;
		}
		else if (ec) {
			std::cout << "Writing error trying again: " << ec.message() << std::endl;
			return true;
		}
	}
	return true;
}
//System works but need to figure out how to stop blocking
//Stop blocking on listen so actual main loop can run and hand out data
void listeningThread() {
	while (!exitf) {
			asio::error_code ec;
			asio::ip::tcp::acceptor accept(io, asio::ip::tcp::endpoint(asio::ip::tcp::v6(), 13));
			asio::ip::tcp::socket* sock = new asio::ip::tcp::socket(io);
			accept.listen();
			accept.accept(*sock, ec);
			iosafe.lock();
			sockets.push_back((sock));
			std::cout << "New connection from: " << sock->remote_endpoint() << std::endl;
			iosafe.unlock();
	}
}


//I think socket is closed everytime accept is called again, and since client doesnt connect again no data passed
//https://www.boost.org/doc/libs/1_35_0/doc/html/boost_asio/tutorial/tutdaytime3.html
//Fix not sending data to all of the clients
int main() {
	std::thread lthread(listeningThread);
	while (!exitf) {
		if (iosafe.try_lock()) {
			for (int j = 0; j < sockets.size(); j++) {
				for (int i = 0; i < messages.size(); i++) {
					if (sockets[j] != messages[i].sock) {
						sendMessage(sockets[j], messages[i].body, messages[i].bytes);
					}
				}
				if (j == sockets.size() - 1) {
					messages.clear();
				}
				//Add pinging to time out sockets that dc because this doesn't work
				if (!recieveMessage(sockets[j])) {
					sockets.erase(sockets.begin() + j);
				}
			}
			iosafe.unlock();
		}
	}
	lthread.join();
	return 0;
}