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
#include <google/protobuf/service.h>

#include "rocket/common/log.h"
#include "rocket/common/config.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/coder/tinypb_coder.h"
#include "rocket/net/coder/tinypb_protocol.h"
#include "order.pb.h"
#include "rocket/net/tcp/tcp_server.h"
#include "rocket/net/rpc/rpc_dispatcher.h"
#include "rocket/net/rpc/rpc_channel.h"
#include "rocket/net/rpc/rpc_controller.h"
#include "rocket/net/rpc/rpc_closure.h"

void test_tcp_client() {
    rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1", 12345);
    rocket::TcpClient client(addr);
    client.connect([addr, &client]() {
        DEBUGLOG("connect to [%s] success", addr->toString().c_str());
        std::shared_ptr<rocket::TinyPBProtocol> message = std::make_shared<rocket::TinyPBProtocol>();
        message->m_msg_id = "99998888";
        message->m_pb_data = "test pb data";

        makeOrderRequest request;
        request.set_price(100);
        request.set_goods("apple");

        if(!request.SerializeToString(&(message->m_pb_data))) {
            ERRORLOG("serialize error");
            return;
        }

        message->m_method_name = "Order.makeOrder";

        client.writeMessage(message, [request](rocket::AbstractProtocol::s_ptr msg_ptr) {
            DEBUGLOG("send message success, request[%s]", request.ShortDebugString().c_str());
        });

        client.readMessage("99998888", [](rocket::AbstractProtocol::s_ptr msg_ptr) {
            std::shared_ptr<rocket::TinyPBProtocol> message = std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg_ptr);
            DEBUGLOG("msg_id[%s], get response %s", message->m_msg_id.c_str(), message->m_pb_data.c_str());
            makeOrderResponse response;

            if(!response.ParseFromString(message->m_pb_data)) {
                ERRORLOG("deserialize error");
                return;
            }
            DEBUGLOG("get response success, response[%s]", response.ShortDebugString().c_str());
        });

    });
}

void test_rpc_channel() {
    rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1", 12345);
    std::shared_ptr<rocket::RpcChannel> channel = std::make_shared<rocket::RpcChannel>(addr);

    std::shared_ptr<makeOrderRequest> request = std::make_shared<makeOrderRequest>();
    request->set_price(100);
    request->set_goods("apple");

    std::shared_ptr<makeOrderResponse> response = std::make_shared<makeOrderResponse>();

    std::shared_ptr<rocket::RpcController> controller = std::make_shared<rocket::RpcController>();
    controller->SetMsgId("99998888");

    std::shared_ptr<rocket::RpcClosure> closure = std::make_shared<rocket::RpcClosure>([request, response, channel]() mutable{
        INFOLOG("call rpc success, request[%s], response[%s]", request->ShortDebugString().c_str(), response->ShortDebugString().c_str());
        INFOLOG("now exit eventloop")
        channel->getTcpClient()->stop();
        channel.reset();
    });

    channel->Init(controller, request, response, closure);

    Order_Stub stub(channel.get());

    stub.makeOrder(controller.get(), request.get(), response.get(), closure.get()); 
}

int main() {

    rocket::Config::SetGlobalConfig("../conf/rocket.xml");
    rocket::Logger::InitGlobalLogger();

    test_rpc_channel();
    INFOLOG("test_rpc_channel end");
    // test_tcp_client();
    return 0;
}