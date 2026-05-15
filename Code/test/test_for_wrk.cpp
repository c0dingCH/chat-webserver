#include"TcpConnection.h"
#include<string>
#include"TcpServer.h"
#include<functional>
#include<memory>
#include"Buffer.h"

void OnMessage(const std::shared_ptr<TcpConnection> & conn){
  conn->GetReadBuffer()->RetrieveAll();
  std::string msg = "HTTP/1.1 200 OK\r\n"
    "Content-Length: 1000\r\n"
    "Connection: keep-alive\r\n"
    "\r\n";

  std::string ttt(1000,'a');
  msg += ttt;

  conn->Send(msg);
}



int main(){
  int port;
  port = 8888;
  TcpServer *server = new TcpServer("0.0.0.0", port);
  server->OnMessage(OnMessage);
  server->Start();
     

}



