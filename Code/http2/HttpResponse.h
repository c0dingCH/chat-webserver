#pragma once
#include<string>
#include<map>
#include<memory>
#include<nghttp2/nghttp2.h>

class Buffer;

class HttpResponse{
public:
//  enum HttpStatusCode{
//    kUnknow = 1,
//    k100Continue = 100,
//    k200K = 200,
//    k302K = 302,
//    k400BadRequest = 400,
//    k403Forbidden = 403,
//    k404NotFound = 404,
//    k500internalServerError = 500
//  };
  struct MmapRead{
    uint8_t * addr;
    size_t offset;
    size_t size;
  };


  HttpResponse(bool close_connection);
  ~HttpResponse();

  
  void AddHeader(const std::string& key, const std::string &value);
  std::string GetHeader(const std::string &key) const;

  void SetBody(const std::string & body);
  void SetMapRead(const MmapRead & map_read);

  int PushPromise(nghttp2_session * session, int push_id, const std::string method, const std::string scheme, const std::string authority, const std::string path);
  void ParseResponse(nghttp2_session * session, int stream_id, void * data = nullptr);
  //std::string GetBeforeBody();

  void SetCloseConnection(bool close_connection);
  bool IsInCloseConnection();

  void SetFileFd(int filefd);
  int GetFileFd();

private:
  std::map<std::string,std::string>headers_;

  std::unique_ptr<Buffer> body_;
  MmapRead map_read_;

  bool close_connection_{false};

  int filefd_{-1};
};
