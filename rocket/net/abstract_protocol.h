#ifndef ROCKET_NET_TCP_ASTRACT_PROTOCOL_H
#define ROCKET_NET_TCP_ASTRACT_PROTOCOL_H

#include <memory>


namespace rocket {

class AbstractProtocol {

public:

    typedef std::shared_ptr<AbstractProtocol> s_ptr;

    std::string getReqId() {
        return m_req_id;
    }

    void setReqId(const std::string& req_id) {
        m_req_id = req_id;
    }

    virtual ~AbstractProtocol() {};


protected:

    std::string m_req_id; // 请求号，唯一标识一个请求或者响应
};

}

#endif