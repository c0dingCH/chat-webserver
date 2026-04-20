#pragma once
#include"Common.h"
#include<functional>
#include<memory>
#include<map>
#include<unordered_map>
#include<mutex>

class TcpConnection;
class TcpServer;
class HttpResponse;
class HttpContext;
class HttpRequest;
class EventLoop;
class MysqlPool;
class HttpServer;
class User;
#define AUTOCLOSETIMEOUT 10.0

struct HttpObjs{
  std::shared_ptr<TcpConnection> conn; 
  HttpServer * server;
  HttpRequest *request; 
  HttpResponse *response;
};

class HttpServer{
public:
  typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
  typedef std::function<void(const HttpObjs &)> HttpResponseCallback;

  DISALLOW_COPY_AND_MOVE(HttpServer);
  HttpServer(const char * ip, const short & port);
  ~HttpServer();


  void HttpDefaultCallback(const HttpObjs &);
  void SetHttpCallback(const HttpResponseCallback &);


  void OnConnection(const TcpConnectionPtr & conn);
  void OnMessage(const TcpConnectionPtr & conn);
  void Onrequest(const TcpConnectionPtr & conn, HttpRequest * request);

  void ActiveCloseConn(const std::weak_ptr<TcpConnection> & conn);
    
  
  void Start();
  void SetThreadNums(const int &);
  void SetMysqlNums(const int &);
  void SetAutoCloseConn(bool auto_close);
  
  void AddConn(const std::string &id, const TcpConnectionPtr &conn);
  void RemoveConn(const std::string &id);
  TcpConnectionPtr GetConn(const std::string &id);
  
  User * AddUser(const std::string& user_name);
  void RemoveUser(const std::string& user_name);
  
  std::unique_ptr<MysqlPool>& GetMysqlPool(); //只传出引用

  void HandleCloseConnection(const TcpConnectionPtr & conn);

  void Send(const TcpConnectionPtr & conn, HttpResponse * response); 

  std::string GetAuthority() const;
private:
  std::string authority_;
  std::unique_ptr<TcpServer> server_;
  std::unique_ptr<MysqlPool> mysql_pool_;
  bool auto_close_conn_{false};
  HttpResponseCallback response_;

  std::unordered_map<std::string,TcpConnectionPtr >id_conn_;
  std::unordered_map<std::string, std::unique_ptr<User>> users_;

  std::mutex conn_mtx_;
  std::mutex user_mtx_;
  
};
