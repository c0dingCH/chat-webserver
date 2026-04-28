#pragma once

#include<memory>
#include<queue>
#include<mutex>
#include<condition_variable>
#include<iostream>

class Mysql;
class MysqlPool{
public:
  MysqlPool(); 
  ~MysqlPool();
  std::unique_ptr<Mysql> Get();
  void Put(std::unique_ptr<Mysql>&);
 
  void Start();
  void SetMysqlNums(const int & conn_nums);
  void CreateConns();

  template<typename Func>
  void WithConnection(Func&& func); 


private:
  std::queue<std::unique_ptr<Mysql>>conns_;
  std::mutex mtx_;
  std::condition_variable cv_;
  int conn_nums_;
};

template<typename Func>
void MysqlPool::WithConnection(Func&& func){
  std::unique_ptr<Mysql> db = Get();
  func(db);
  Put(db);
}


//没写get了就得put的保证逻辑， 所以调用之后记得Put
