#include"User.h"
#include"TcpConnection.h"

User::User(const std::shared_ptr<TcpConnection>&conn, const std::function<void()>&cb)
: conn_(conn)
{
  conn_->SetCloseBusi(cb); 
}

User::~User(){

}

void User::Send(const unique_ptr<User>& target, const std::string &msg){
  target->GetConn()->SendInLoop(msg);
}

std::shared_ptr<TcpConnection> GetConn(){
  return conn_;
}
