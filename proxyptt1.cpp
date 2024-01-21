#include <iostream>
#include <boost/asio.hpp>

using namespace boost::asio;

class ProxyServer {
public:
    ProxyServer(io_service& ios, short port)
        : acceptor_(ios, ip::tcp::endpoint(ip::tcp::v4(), port)),
          socket_(ios) {
        startAccept();
    }

private:
    void startAccept() {
        acceptor_.async_accept(socket_,
            [this](boost::system::error_code ec) {
                if (!ec) {
                    std::cout << "Connection accepted" << std::endl;
                    handleClient();
                }

                startAccept();
            });
    }

    void handleClient() {
        io_service& ios = socket_.get_io_service();

        // Connect to the destination server
        ip::tcp::resolver resolver(ios);
        ip::tcp::resolver::query query("example.com", "http");
        ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        ip::tcp::socket server_socket(ios);
        connect(server_socket, endpoint_iterator);

        // Transfer data between client and server
        std::array<char, 8192> buffer;
        socket_.async_read_some(buffer, 
            [this, &buffer](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    async_write(server_socket, buffer.data(), 
                        [this](boost::system::error_code ec, std::size_t length) {
                            if (!ec) {
                                handleClient();
                            }
                        });
                }
            });

        server_socket.async_read_some(buffer, 
            [this, &buffer](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    async_write(socket_, buffer.data(), 
                        [this](boost::system::error_code ec, std::size_t length) {
                            if (!ec) {
                                handleClient();
                            }
                        });
                }
            });
    }

    ip::tcp::acceptor acceptor_;
    ip::tcp::socket socket_;
};

int main() {
    try {
        io_service ios;
        ProxyServer proxy(ios, 8080);
        ios.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
