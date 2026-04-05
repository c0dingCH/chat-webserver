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
  server_ = std::make_unique<TcpServer>(ip, port);
  server_ -> OnConnect(std::bind(&HttpServer::OnConnection, this, std::placeholders::_1));
  server_ -> OnMessage(std::bind(&HttpServer::OnMessage, this, std::placeholders::_1));
  SetHttpCallback(std::bind(&HttpServer::HttpDefaultCallback, this,std::placeholders::_1, std::placeholders::_2));
}

HttpServer::~HttpServer(){
  
}

void HttpServer::HttpDefaultCallback(HttpRequest * request, HttpResponse * response){
  response->SetStatusCode(HttpResponse::HttpStatusCode::k404NotFound);
  response->SetStatusMessage("Not Found");
  response->SetCloseConnection(true);
}


void HttpServer::SetHttpCallback(const HttpResponseCallback & cb){
  response_ = std::move(cb);
}


void HttpServer::OnConnection(const TcpConnectionPtr & conn){
  sockaddr_in addr{};
  socklen_t len = sizeof addr;
  getpeername(conn->GetFd(), (sockaddr *)&addr, &len);

//  std::cout <<  CurrentThread::tid()
//            <<  " EchoServer::OnNewConnection : new connection"
//            <<  " fd[#"<< conn->GetFd()<<"]"
//            <<  " from "<< inet_ntoa(addr.sin_addr)<<":"<<ntohs(addr.sin_port)
//            <<  std::endl;

  if(auto_close_conn_){
    auto loop = conn->GetLoop();
    loop -> RunAfter(AUTOCLOSETIMEOUT, std::bind(&HttpServer::ActiveCloseConn, this, conn));
  }
}

void HttpServer::OnMessage(const TcpConnectionPtr &conn){
  if(conn->GetState() == TcpConnection::State::Connected){
    HttpContext * context = conn->GetContext();
    if(!context->ParseRequest(conn->GetReadBuffer()->beginread(), conn->GetReadBuffer()->readablebytes())){
      conn->Send("HTTP/1.1 400 Bad Request \r\n\r\n");
      conn->HandleClose();
    }
    if(context->GetCompleteRequest()){
      Onrequest(conn, context->GetRequest());
      context->ResetContextStatus();
    }
  }   
}

void HttpServer::Onrequest(const TcpConnectionPtr & conn, HttpRequest * request){
  std::string connection_state = request->GetHeader("Connection");
  bool close = connection_state == "Close" ||
                ( request->GetVersion() == HttpRequest::Version::kHttp10 && connection_state != "keep-alive" );
  
  if(request->GetHeader("Content-Type").find("multipart/form-data") != std::string::npos){
    size_t it_boundary = request->GetHeader("Content-Type").find("boundary=");
    if(it_boundary == std::string::npos){
      LOG_ERROR << "upload Content-Type parse error !";
    }
    else{
      std::string boundary = request->GetHeader("Content-Type").substr(it_boundary + 9);
      std::string file_message = request->GetBody();
      size_t it_filename_begin = file_message.find("filename=\"") + 10;
      size_t it_filename_end   = file_message.find("\"",it_filename_begin);
      std::string filename = file_message.substr(it_filename_begin, it_filename_end - it_filename_begin);

      size_t it_content = file_message.find("Content-Type");
      size_t it_file_begin = file_message.find("\r\n", it_content) + 4;
      size_t it_file_end   = file_message.find("--" + boundary + "--", it_file_begin);
      std::string file_data = file_message.substr(it_file_begin, it_file_end - it_file_begin);

      std::ofstream of(("../static/" + filename).c_str(), std::ios::out | std::ios::binary); // 这里假设文件很小，能一次性传完
      of.write(file_data.c_str(), file_data.size());
      of.close();
    }
  }

  HttpResponse response(close);
 
  response_(request, &response);
  
  // 这里只判断html 和 file 两种
  if(response.GetBodyType() == HttpResponse::HttpBodyType::kHtml){
    conn->SendInLoop(response.GetMessage().c_str());
  }
  else{
    conn->Send(response.GetBeforeBody());
    conn->SendFile(response.GetFileFd(),response.GetBodyLength());
      
    if(::close(response.GetFileFd()) == -1){
      LOG_ERROR<< "Close File Error";
    }
    else{
      LOG_INFO << "Close FIle Ok !";
    }
    
  }

  if(response.IsInCloseConnection()){
    conn->HandleClose();
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
      conn->HandleClose();
    }
  }
}


void HttpServer::Start(){
  server_->Start();
  mysql_pool_->Start();
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
