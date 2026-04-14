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

    if(request && request->GetRequestStatus() == HttpRequest::HttpRequestStatus::kComplete){
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

  if(response.GetHeader("content-type") == "text/html"){
    Send(conn ,&response);
  }
  else{
    //conn->Send(response.GetBeforeBody());
    //conn->SendFile(response.GetFileFd(),response.GetContentLength());
      
    //if(::close(response.GetFileFd()) == -1){
    //  LOG_ERROR<< "Close File Error";
    //}
    //else{
    //  LOG_INFO << "Close FIle Ok !";
    //}
    
  }

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
  std::unique_lock<std::mutex>lock(mtx_);
  id_conn_[id] = conn;
}

void HttpServer::RemoveConn(const std::string &id){
  std::unique_lock<std::mutex>lock(mtx_);
  auto it = id_conn_.find(id);
  if(it != id_conn_.end()){
    id_conn_.erase(it);
  }
  else{
    LOG_ERROR << "No such id login !";
  }
}

HttpServer::TcpConnectionPtr HttpServer::GetConn(const std::string &id){
  std::unique_lock<std::mutex>lock(mtx_);
  if(!id_conn_.count(id))return nullptr;
  else return id_conn_[id];
}
// 这里会崩，记得改
void HttpServer::HandleCloseConnection(const TcpConnectionPtr & conn){
  const std::string & id = conn->GetContext()->GetRequest()->GetDom()["username"].GetString();
  RemoveConn(id);
  conn->HandleClose();
}

std::unique_ptr<MysqlPool>& HttpServer::GetMysqlPool(){
  return mysql_pool_;
}

void HttpServer::Send(const TcpConnectionPtr & conn, HttpResponse * response){
  auto context = conn -> GetContext();
  response->ParseResponse(context->GetSession(), context->current_id_);
  conn->Send(context->GetMessage());
}

std::string HttpServer::GetAuthority() const{
  return authority_;
}

