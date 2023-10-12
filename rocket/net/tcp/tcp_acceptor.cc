#include <assert.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>

#include "rocket/common/log.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_acceptor.h"

namespace rocket {

TcpAcceptor::TcpAcceptor(NetAddr::s_ptr local_addr) : m_local_addr(local_addr){
    if(!local_addr->checkValid()) {
        ERRORLOG("invalid local addr %s", local_addr->toString().c_str());
        exit(0);
    }

    m_family = m_local_addr->getFamily();

    m_listenfd = socket(m_family, SOCK_STREAM, 0);

    if(m_listenfd < 0) {
        ERRORLOG("invalid listen fd %d", m_listenfd);
        exit(0);
    }

    int val = 1;
    if(setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) != 0) {
        ERRORLOG("setsockopt REUSEADDR erro, errno=%d, error=%s", errno, strerror(errno));
    }

    socklen_t len = m_local_addr->getSockLen();
    if(bind(m_listenfd, m_local_addr->getSockAddr(), len) != 0) {
        ERRORLOG("bind erro, errno=%d, error=%s", errno, strerror(errno));
        exit(0);
    }

    if(listen(m_listenfd, 1000) != 0) {
        ERRORLOG("listen erro, errno=%d, error=%s", errno, strerror(errno));
        exit(0);
    }
}

TcpAcceptor::~TcpAcceptor() {

}

std::pair<int, NetAddr::s_ptr> TcpAcceptor::accept() {
    if(m_family == AF_INET) {
        sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_addr_len = sizeof(client_addr);

        int client_fd = ::accept(m_listenfd, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);
        if(client_fd < 0) {
            ERRORLOG("accept erro, errno=%d, error=%s", errno, strerror(errno));
        }
        IPNetAddr::s_ptr peer_addr = std::make_shared<IPNetAddr>(client_addr);
        INFOLOG("A client have accepted succ, peer addr [%s]", peer_addr->toString().c_str());
        return std::make_pair(client_fd, peer_addr);
    }else {
        //..其他的ip地址类型处理
        return std::make_pair(-1, nullptr);
    }
}

int TcpAcceptor::getListenFd() {
    return m_listenfd;
}

}