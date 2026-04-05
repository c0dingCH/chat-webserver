#pragma once

#include<memory>
#include<queue>
#include<mutex>
#include<condition_variable>

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
private:
  std::queue<std::unique_ptr<Mysql>>conns_;
  std::mutex mtx_;
  std::condition_variable cv_;
  int conn_nums_;
};

//没写get了就得put的保证逻辑， 所以调用之后记得Put
