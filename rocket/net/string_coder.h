#ifndef ROCKET_NET_STRING_CODER_H
#define ROCKET_NET_STRING_CODER_H

#include "rocket/net/abstract_coder.h"

namespace rocket {

class StringProtocol : public AbstractProtocol{
public:
    std::string info;
};

class StringCoder : public AbstractCoder {

   // 将message对象转化为字节流，写入到buffer
    virtual void encode(std::vector<AbstractProtocol*>& messages, TcpBuffer::s_ptr out_buffer) {

        for(size_t i = 0; i < messages.size(); ++i) {
            StringProtocol* msg = dynamic_cast<StringProtocol*>(messages[i]);
            out_buffer->writeToBuffer(msg->info.c_str(), msg->info.length());

        }
    }

    // 将buffer里面的字节流转换为哦message对象
    virtual void decode(std::vector<AbstractProtocol*>& out_messages, TcpBuffer::s_ptr buffer) {
        std::vector<char> re;
        buffer->readFromBuffer(re, buffer->readAble());
        std::string info;
        for(size_t i = 0; i < re.size(); ++i) {
            info += re[i];
        }

        StringProtocol* msg = new StringProtocol();
        msg->info = info;
        out_messages.push_back(msg);
    }

};

}


#endif