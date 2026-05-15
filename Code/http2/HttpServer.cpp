#include"HttpServer.h"
#include"HttpContext.h"
#include"HttpResponse.h"
#include"HttpRequest.h"
#include"TcpServer.h"
#include"TcpConnection.h"
#include"Buffer.h"
#include"EventLoop.h"
#include"Logging.h"
#include"MysqlPool.h"
#include"RedisPool.h"
#include"User.h"

#include<functional>
#include<memory>
#include<arpa/inet.h>
#include<CurrentThread.h>
#include<unistd.h>
#include<fstream>

HttpServer::HttpServer(const char * ip, const short & port){
  authority_ = ip + std::string(":",1) + std::to_string(port);
  server_ = std::make_unique<TcpServer>(ip, port);
  server_ -> OnConnect(std::bind(&HttpServer::OnConnection, this, std::placeholders::_1));
  server_ -> OnMessage(std::bind(&HttpServer::OnMessage, this, std::placeholders::_1));
  SetHttpCallback(std::bind(&HttpServer::HttpDefaultCallback, this,std::placeholders::_1));
  mysql_pool_ = std::make_unique<MysqlPool>();
  redis_pool_ = std::make_unique<RedisPool>(mysql_pool_.get());
}

HttpServer::~HttpServer(){
  
}

void HttpServer::HttpDefaultCallback(const HttpObjs & hs){
  hs.response->AddHeader(":status", "404");
  hs.response->SetCloseConnection(true);
}


void HttpServer::SetHttpCallback(const HttpResponseCallback & cb){
  response_ = std::move(cb);
}


void HttpServer::OnConnection(const TcpConnectionPtr & conn){
  if(conn->GetState() == TcpConnection::State::kConnected){
    sockaddr_in addr{};
    socklen_t len = sizeof addr;
    getpeername(conn->GetFd(), (sockaddr *)&addr, &len);
  
    LOG_INFO  <<  CurrentThread::tid()
              <<  " EchoServer::OnNewConnection : new connection"
              <<  " fd[#"<< conn->GetFd()<<"]"
              <<  " from "<< inet_ntoa(addr.sin_addr)<<":"<<ntohs(addr.sin_port);
  
    if(auto_close_conn_){
      auto loop = conn->GetLoop();
      loop -> RunAfter(AUTOCLOSETIMEOUT, std::bind(&HttpServer::ActiveCloseConn, this, conn));
    }
  }
  else if(conn->GetState() == TcpConnection::State::kConnected){
    if(on_close_){
      on_close_();
      on_close_ = nullptr;
    }
  }
}

void HttpServer::OnMessage(const TcpConnectionPtr &conn){
  if(conn->GetState() == TcpConnection::State::kConnected){
    HttpContext * context = conn->GetContext();
    
    int handshake_init = context->GetHandshakeDone();

    context->ParseRequest(conn->GetReadBuffer()->beginread(), conn->GetReadBuffer()->readablebytes());
    HttpRequest * request = context->GetRequest();
    
    int handshake_cur = context->GetHandshakeDone();
  
    if(handshake_init == 0){
      if(handshake_cur == 1){
        conn->Send(context->GetMessage());
      }
      else{
        LOG_INFO << "waiting for shaking";
      }
      return;
    }
   
    if(!context->GetParseStatus()){
      HttpResponse response(context->GetSession());
      response.AddHeader(":status", "400");
      Send(conn, &response);
      HandleCloseConnection(conn);
      return;
    }

    if(context->current_id_ == context->push_id_){
      return;
    }
    else if(request && request->GetRequestStatus() == HttpRequest::HttpRequestStatus::kComplete){
      if(request->GetHeader("content-type") == "application/json")request->ParseJson2Dom();
      Onrequest(conn, request);
    }
    else{
      //placehoders
  
    }
  }   
}

void HttpServer::Onrequest(const TcpConnectionPtr & conn, HttpRequest * request){
  std::string connection_state = request->GetHeader("Connection");
  bool close = (
      strcasecmp(connection_state.c_str(),"close") == 0
      //(request->GetVersion() == HttpRequest::Version::kHttp10 && connection_state != "keep-alive" )
  );

  HttpResponse response(close);
  response_({conn, this, request, &response});

  Send(conn ,&response);

  if(response.IsInCloseConnection()){
    HandleCloseConnection(conn);
  }
}

void HttpServer::ActiveCloseConn(const std::weak_ptr<TcpConnection> & weak_ptr){
  auto conn = weak_ptr.lock();
  if(conn){
    if(TimeStamp::Now() < TimeStamp::AddTime(conn->GetTimeStamp(),AUTOCLOSETIMEOUT)){
      auto loop = conn->GetLoop();
      loop -> RunAt(TimeStamp::AddTime(conn->GetTimeStamp(),AUTOCLOSETIMEOUT), std::bind(&HttpServer::ActiveCloseConn, this, weak_ptr));
    } 
    else{
      HandleCloseConnection(conn);
    }
  }
}


void HttpServer::Start(){
  mysql_pool_->Start();
  redis_pool_->Start();
  server_->Start();
}

void HttpServer::SetThreadNums(const int & thread_nums){
  server_ -> SetThreadNums(thread_nums);
}

void HttpServer::SetMysqlNums(const int & conn_nums){
  mysql_pool_ -> SetMysqlNums(conn_nums);
}

void HttpServer::SetAutoCloseConn(bool auto_close){
  auto_close_conn_ = auto_close;
}

void HttpServer::AddConn(const std::string &id, const TcpConnectionPtr &conn){
  std::unique_lock<std::mutex>lock(conn_mtx_);
  id_conn_[id] = conn;
}

void HttpServer::RemoveConn(const std::string &id){
  std::unique_lock<std::mutex>lock(conn_mtx_);
  auto it = id_conn_.find(id);
  if(it != id_conn_.end()){
    id_conn_.erase(it);
  }
}

HttpServer::TcpConnectionPtr HttpServer::GetConn(const std::string &id){
  std::unique_lock<std::mutex>lock(conn_mtx_);
  if(!id_conn_.count(id))return nullptr;
  else return id_conn_[id];
}

User * HttpServer::AddUser(const std::string& user_name){
  std::unique_lock<std::mutex> lock(user_mtx_);
  auto user = std::make_unique<User>(user_name);
  auto user_ptr = user.get();
  users_[user_name] = std::move(user);
  return user_ptr;
}

void HttpServer::RemoveUser(const std::string& user_name){
  std::unique_lock<std::mutex> lock(user_mtx_);
  auto it = users_.find(user_name);
  if(it != users_.end()){
    users_.erase(it);
  }
}

void HttpServer::HandleCloseConnection(const TcpConnectionPtr & conn){
  conn->HandleClose(); 
}

MysqlPool * HttpServer::GetMysqlPool(){
  return mysql_pool_.get();
} 

RedisPool * HttpServer::GetRedisPool(){
  return redis_pool_.get();
}

void HttpServer::Send(const TcpConnectionPtr & conn, HttpResponse * response){
  auto context = conn -> GetContext();
  response->ParseResponse(context->GetSession(), context->current_id_);
  conn->Send(context->GetMessage());
}

std::string HttpServer::GetAuthority() const{
  return authority_;
}

