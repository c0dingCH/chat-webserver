#pragma once
#include<string>
#include<memory>
#include<map>
#include<nghttp2/nghttp2.h> 

class HttpRequest;
class Buffer;


class HttpContext{
public:

  // 状态转换机，保存解析的状态

  HttpContext();
  ~HttpContext();


  void ParseRequest(const char * msg, size_t len);
  
  void RemoveStream();
  HttpRequest *GetRequest();
  
  void SetHandshakeDone(int done);
  int GetHandshakeDone();

  nghttp2_session * GetSession() const; 
  bool GetParseStatus() const;

  void Append(const char * msg, size_t len);
  std::string GetMessage() const;
  
  void AddStream();
  int current_id_{0};
  int push_id_{0};
  //后续session析构会自动清理push_id
  
private:

  std::map<int,std::unique_ptr<HttpRequest> >requests_;
  int handshake_done_{0}; // 0 无连接 1 等待ack 2 已连接

  std::unique_ptr<Buffer> buffer_;
  
  bool parse_status_{true};

  //in oreder !
  std::unique_ptr<nghttp2_session_callbacks, decltype(&nghttp2_session_callbacks_del)> callbacks_{nullptr,nghttp2_session_callbacks_del};
  //std::unique_ptr<nghttp2_option, decltype(&nghttp2_option_del)> option_{nullptr,nghttp2_option_del};
  std::unique_ptr<nghttp2_session, decltype(&nghttp2_session_del)> session_{nullptr,nghttp2_session_del};

};
