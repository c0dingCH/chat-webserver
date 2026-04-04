#pragma once
#include"Common.h"
#include<functional>
#include<memory>
#include<map>

class TcpConnection;
class TcpServer;
class HttpResponse;
class HttpContext;
class HttpRequest;
class EventLoop;

#define AUTOCLOSETIMEOUT 10.0


class HttpServer{
public:
  typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
  typedef std::function<void(HttpRequest *, HttpResponse *)> HttpResponseCallback;

  DISALLOW_COPY_AND_MOVE(HttpServer);
  HttpServer(const char * ip, const short & port);
  ~HttpServer();


  void HttpDefaultCallback(HttpRequest * request, HttpResponse * response);
  void SetHttpCallback(const HttpResponseCallback &);


  void OnConnection(const TcpConnectionPtr & conn);
  void OnMessage(const TcpConnectionPtr & conn);
  void Onrequest(const TcpConnectionPtr & conn, HttpRequest * request);

  void ActiveCloseConn(const std::weak_ptr<TcpConnection> & conn);
    
  
  void Start();
  void SetThreadNums(const int &);
  void SetAutoCloseConn(bool auto_close);


private:
  std::unique_ptr<TcpServer> server_;
  bool auto_close_conn_{false};
  HttpResponseCallback response_;

  std::map<std::string,TcpConnectionPtr>id_conn_;
};
