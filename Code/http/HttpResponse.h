#pragma once
#include<string>
#include<map>

class HttpResponse{
public:
  enum HttpStatusCode{
    kUnknow = 1,
    k100Continue = 100,
    k200K = 200,
    k302K = 302,
    k400BadRequest = 400,
    k403Forbidden = 403,
    k404NotFound = 404,
    k500internalServerError = 500
  };
  

  HttpResponse(bool close_connection);
  ~HttpResponse();

  void SetVersion(const std::string &version);
  void SetStatusCode(HttpStatusCode status_code);
  void SetStatusMessage(const std::string & message);
  void SetCloseConnection(bool close_connection);
  
  void AddHeader(const std::string& key, const std::string &value);
  std::string GetHeader(const std::string &key) const;

  void SetBody(const std::string & body);
  void SetContentLength(int len);
  int GetContentLength() const;

  std::string GetMessage();
  std::string GetBeforeBody();

  bool IsInCloseConnection();

  void SetFileFd(int filefd);
  int GetFileFd();

private:
  std::map<std::string,std::string>headers_;
  
  std::string version_;
  HttpStatusCode status_code_;
  std::string status_message_;
  std::string body_;

  int content_length_{0};
  bool close_connection_{false};
  int filefd_{-1};
};
