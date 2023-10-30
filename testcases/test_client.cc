#include <assert.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <string>
#include <memory>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "rocket/common/log.h"
#include "rocket/common/config.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/string_coder.h"
#include "rocket/net/abstract_protocol.h"

void test_connet() {
    // 调用connect连接server
    // write一个字符串
    // 等待read返回结果

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if(fd < 0) {
        ERRORLOG("invalid fd %d", fd);
        exit(0);
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    inet_aton("127.0.0.1", &server_addr.sin_addr);

    int rt = connect(fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));

    std::string msg = "hello socket!";

    rt = write(fd, msg.c_str(), msg.length());

    DEBUGLOG("success write %d butyes, [%s]", rt, msg.c_str());

    char buf[100];
    rt = read(fd, buf, 100);

    DEBUGLOG("success read %d butyes, [%s]", rt, std::string(buf).c_str());
    
}

void test_tcp_client() {
    rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1", 12345);
    rocket::TcpClient client(addr);
    client.connect([addr, &client]() {
        DEBUGLOG("connect to [%s] success", addr->toString().c_str());
        std::shared_ptr<rocket::StringProtocol> message = std::make_shared<rocket::StringProtocol>();
        message->info = "hello rocket";
        message->setReqId("123456");
        client.writeMessage(message, [](rocket::AbstractProtocol::s_ptr msg_ptr) {
            DEBUGLOG("send message success");
        });
    });
}

int main() {

    rocket::Config::SetGlobalConfig("../conf/rocket.xml");
    rocket::Logger::InitGlobalLogger();

    // test_connet();

    test_tcp_client();
    return 0;
}