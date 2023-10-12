#include "rocket/net/tcp/tcp_server.h"
#include "rocket/net/eventloop.h"
#include "rocket/common/log.h"
#include "rocket/net/tcp/tcp_connection.h"

namespace rocket {

TCPServer::TCPServer(NetAddr::s_ptr local_addr) : m_local_addr(local_addr) {

    init();

    INFOLOG("rocket TcpServer listen success on [%s]", m_local_addr->toString().c_str());
}

TCPServer::~TCPServer() {
    if (m_main_event_loop) {
        delete m_main_event_loop;
        m_main_event_loop = NULL;
    }
    if(m_io_thread_group) {
        delete m_io_thread_group;
        m_io_thread_group = NULL;
    }
}

void TCPServer::start() {
   m_io_thread_group->start();
   m_main_event_loop->loop();
}

void TCPServer::init() {
    m_acceptor = std::make_shared<TcpAcceptor>(m_local_addr);

    m_main_event_loop = EventLoop::GetCurrentEventLoop();
    m_io_thread_group = new IOThreadGroup(2);

    m_listen_fd_event = new FdEvent(m_acceptor->getListenFd());
    m_listen_fd_event->listen(FdEvent::IN_EVENT, std::bind(&TCPServer::onAccept, this));

    m_main_event_loop->addEpollEvent(m_listen_fd_event);
}

void TCPServer::onAccept() {
    auto re = m_acceptor->accept();
    int client_fd = re.first;
    NetAddr::s_ptr peer_addr = re.second;
    // FdEvent client_fd_event(client_fd);
    m_client_counts++;

    // 把client_fd添加到IO线程里面
    // m_io_thread_group->getIOThread()->getEventLoop()->addEpollEvent(c);
    IOThread* io_thread = m_io_thread_group->getIOThread();
    TcpConnection::s_ptr connection = std::make_shared<TcpConnection>(io_thread, client_fd, 128, peer_addr);
    

    INFOLOG("TcpServer succ get client, fd=%d", client_fd);
}

}