template <class T> class WsPacket {
private:
  static std::string getHeader(typename T::Message &message,
                               size_t headerLen) {
    auto pbuf = message.rdbuf();
    const size_t sizeBuffer = headerLen;
    if (message.size() < sizeBuffer) {
      LOG(ERROR) << "Illegal Size:" << message.size();
      return "";
    }
    char buffer[sizeBuffer + 1];
    pbuf->sgetn(buffer, sizeBuffer);
    pbuf->pubseekpos(sizeBuffer);
    buffer[sizeBuffer] = 0;
    return buffer;
  }
  WsPacket(typename T::Message &message, std::string header, size_t headerLen)
      : message(message), header(header),
        headerLen(headerLen), packetLen(message.size() - headerLen) {}

public:
  static WsPacket from(typename T::Message &message,
                       size_t headerLen = sizeof("BUFF") - 1) {
    return WsPacket(message, getHeader(message, headerLen), headerLen);
  }
  typename T::Message &message;
  const std::string header;
  const size_t headerLen;
  const size_t packetLen;
  ssize_t copyOut(void *dst, size_t lenDst) const {
    if (lenDst < packetLen) {
      LOG(INFO) << "packet will be cut";
    }
    size_t plen = std::min(lenDst, packetLen);
    message.rdbuf()->sgetn(static_cast<char *>(dst), plen);
    //pbuf->pubseekpos(sizeBuffer);
    return static_cast<ssize_t>(plen);
  }
};
