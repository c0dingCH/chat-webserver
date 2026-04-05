#pragma once
#include<memory>
#include<string>
#include<functional>
class TcpConnection;

class User{
public:
  User(const std::shared_ptr<TcpConnection>&conn, const std::function<void()>&cb);
  ~User();

  void Send(const unique_ptr<User>& target, const std::string &msg);
  std::shared_ptr<TcpConnection> GetConn();
  

private:
  std::shared_ptr<TcpConnection>conn_;
}
