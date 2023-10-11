#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/net/fd_event_group.h"
#include "rocket/common/log.h"

namespace rocket {

TcpConnection::TcpConnection(IOThread* IO_thread, int fd, int buffer_size, NetAddr::s_ptr peer_addr)
    : m_io_thread(IO_thread), m_peer_addr(peer_addr), m_state(NotConnected) {
    m_in_buffer = std::make_shared<TcpBuffer>(buffer_size);
    m_out_buffer = std::make_shared<TcpBuffer>(buffer_size);

    m_fd_event = FdEventGroup::GetFdEventGroup()->getFdEvent(fd);
    m_fd_event->listen(FdEvent::IN_EVENT, std::bind(&TcpConnection::read, this));
}

TcpConnection::~TcpConnection() {

}

void TcpConnection::read() {
    // 1.从socket缓冲区，调用系统的read函数读取字节到in_buffer里面
    if(m_state != Connected) {
        INFOLOG("client has already disconnected, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd_event->getFd());
        return ;
    }

}

void TcpConnection::excute() {

}

void TcpConnection::write() {

}




}