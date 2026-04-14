#pragma once
#include<string>
#include<map>
#include<memory>
#include<rapidjson/document.h>

class Buffer;

class HttpRequest{
public:

  enum HttpRequestStatus{
    kStart,
    kHeader,
    kData, 
    kComplete,
    kClose,
    kFaild
  };

  HttpRequest(int stream_id);
  ~HttpRequest();

  void AddHeader(const std::string &field, const std::string &value); 
  std::string GetHeader(const std::string &field) const;

//const uint8_t * GetBody() const;
  void Append(const uint8_t * data, size_t len);
  
  HttpRequestStatus GetRequestStatus();
  void SetRequestStatus(const HttpRequestStatus status);

//json
  void ParseJson2Dom();
  std::string ParseJson2String();
  std::string ParseJson2String(rapidjson::Document & dom);
  rapidjson::Document& GetDom();

private:
  HttpRequestStatus status_{HttpRequestStatus::kStart};
  int stream_id_{-1};
  

//head
  std::map<std::string, std::string> headers_; 
//body
  rapidjson::Document dom_;
  std::unique_ptr<Buffer> buffer_;
};
