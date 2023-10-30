#include <unistd.h>

#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/net/fd_event_group.h"
#include "rocket/common/log.h"

namespace rocket {

TcpConnection::TcpConnection(EventLoop* event_loop, int fd, int buffer_size, NetAddr::s_ptr peer_addr)
    : m_event_loop(event_loop), m_peer_addr(peer_addr), m_state(NotConnected), m_fd(fd) {
    m_in_buffer = std::make_shared<TcpBuffer>(buffer_size);
    m_out_buffer = std::make_shared<TcpBuffer>(buffer_size);

    m_fd_event = FdEventGroup::GetFdEventGroup()->getFdEvent(fd);

    m_fd_event->setNonBlock();
    m_fd_event->listen(FdEvent::IN_EVENT, std::bind(&TcpConnection::onRead, this));

    m_event_loop->addEpollEvent(m_fd_event);
}

TcpConnection::~TcpConnection() {
    DEBUGLOG("~TcpConnection");
}

void TcpConnection::onRead() {
    // 1.从socket缓冲区，调用系统的read函数读取字节到in_buffer里面
    if(m_state != Connected) {
        ERRORLOG("onRead error, client has already disconnected, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
        return ;
    }

    bool is_read_all = false;
    bool is_close = false;
    while(!is_read_all) {
        if(m_in_buffer->writeAble() == 0) {
            m_in_buffer->resizeBuffer(2 * m_in_buffer->m_buffer.size());
        }
        int read_count = m_in_buffer->writeAble();
        int write_index = m_in_buffer->writeIndex();

        int rt = read(m_fd, &(m_in_buffer->m_buffer[write_index]), read_count);
        DEBUGLOG("success read %d bytes from addr[%s], client fd[%d]", rt, m_peer_addr->toString().c_str(), m_fd);
        if(rt > 0) {
            m_in_buffer->moveWriteIndex(rt);

            if(rt == read_count) {
                continue;
            }else if(rt < read_count) {
                is_read_all = true;
                break;
            }
        }else if(rt == 0) {
            is_close = true; 
            break;
        }else if(rt == -1 && errno == EAGAIN) {
            is_read_all = true;
            break;
        }
    }

    if(is_close) {
        // TODO: 处理关闭连接
        clear();
        INFOLOG("peer closed, peer addr [%d], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
        return;
    }

    if(!is_read_all) {
        ERRORLOG("not read all data");
    }

    //TODO: 简单的echo，后面补充RPC协议的解析
    excute();

}

void TcpConnection::excute() {
    // 将RPC请求执行业务逻辑，获取RPC响应，再把RPC响应发回去
    std::vector<char> tmp;
    int size = m_in_buffer->readAble();
    tmp.resize(size);
    m_in_buffer->readFromBuffer(tmp, size);

    std::string msg;
    for(size_t i = 0; i < tmp.size(); i++) {
        msg += tmp[i];
    }

    INFOLOG("succ get request[%s] from client[%s]", msg.c_str(), m_peer_addr->toString().c_str());

    m_out_buffer->writeToBuffer(msg.c_str(), msg.length());

    m_fd_event->listen(FdEvent::OUT_EVENT, std::bind(&TcpConnection::onWrite, this));

    m_event_loop->addEpollEvent(m_fd_event);

}

void TcpConnection::onWrite() {
    // 将当前out_buffer 里面的数据全部发送给client
    if(m_state != Connected) {
        ERRORLOG("onWrite error, client has already disconnected, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
        return ;
    }

    bool is_write_all = false;
    while(true) {
        if(m_out_buffer->readAble() == 0) {
            DEBUGLOG("no data need to seed to client[%s]", m_peer_addr->toString().c_str());
            is_write_all = true;
            break;
        }

        int write_size = m_out_buffer->readAble();
        int read_index = m_out_buffer->readIndex();

        int rt = write(m_fd, &(m_out_buffer->m_buffer[read_index]), write_size);

        if(rt >= write_size) {
            DEBUGLOG("no data need to seed to client[%s]", m_peer_addr->toString().c_str());
            is_write_all = true;
            break;
        }if(rt == -1 && errno == EAGAIN) {
            // 发送缓冲区已满，不能再发送了 等下次fd可写的时候发送数据即可
            ERRORLOG("write data error, errno == EAGAIN and rt == -1");
            break;
        }
    }
    if(is_write_all) {
        m_fd_event->cancle(FdEvent::OUT_EVENT);
        m_event_loop->addEpollEvent(m_fd_event);
    }

}

void TcpConnection::setState(const TcpState state) {
    m_state = Connected;
}

TcpState TcpConnection::getState() {
    return m_state;
}

void TcpConnection::clear() {
    // 处理一些关闭连接后的清理动作
    if(m_state == Closed) {
        return;
    }
    m_fd_event->cancle(FdEvent::IN_EVENT);
    m_fd_event->cancle(FdEvent::OUT_EVENT);

    m_event_loop->deleteEpollEvent(m_fd_event);

    m_state = Closed;

}

void TcpConnection::shutdown() {
    if(m_state == Closed || m_state == NotConnected) {
        return;
    }

    //处于半关闭状态
    m_state = HalfClosing;

    //调用shutdown关闭读写，意味着服务器不再会对这个fd进行读写操作了
    //发送FIN报文，触发了四次挥手的第一个阶段
    //当fd发生可读事件，但是可读的数据为0时，即对端发送了FIN
    ::shutdown(m_fd, SHUT_RDWR);
}

void TcpConnection::setConnectionType(TcpConnectionType type) {
    m_connection_type = type;
}


}