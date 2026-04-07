#pragma once
#include<mysql/mysql.h>
#include<iostream>
#include<string>

// 针对users表的操作
class Mysql{
public:
  enum MysqlStatus{
    kUserIdIllegal,
    kPasswordIllegal,
    kAlreadyIn,
    kNotFound,
    kSuccess,
    kFailed
  };
  Mysql();
  ~Mysql();

  MysqlStatus Insert(const std::string &user_id, const std::string &password);
  MysqlStatus Delete(const std::string &user_id);
  MysqlStatus Update(const std::string &user_id, const std::string &password);
  MysqlStatus Select(const std::string &user_id); 
  MysqlStatus Login(const std::string &user_id, const std::string &password);
  const char * GetMsg(MysqlStatus state); 

private:
  MYSQL conn_;
};
