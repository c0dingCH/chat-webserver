#include "MysqlPool.h"
#include "Mysql.h"
#include <thread>

MysqlPool::MysqlPool()
: conn_nums_(std::thread::hardware_concurrency())
{}

MysqlPool::~MysqlPool(){
  
}

void MysqlPool::Start(){
  CreateConns();
}


std::unique_ptr<Mysql> MysqlPool::Get(){
  std::unique_ptr<Mysql>conn;
  
  {
    std::unique_lock<std::mutex>lock(mtx_);
    cv_.wait(lock,[&](){ return !conns_.empty(); });
    conn = std::move(conns_.front());
    conns_.pop();
  }

  return conn;
}

void MysqlPool::Put(std::unique_ptr<Mysql>&conn){
  {
    std::unique_lock<std::mutex>lock(mtx_);
    conns_.push(std::move(conn));
  }
  cv_.notify_one();
}

void MysqlPool::CreateConns(){
  for(int i = 0;i < conn_nums_;i++){
    conns_.emplace(std::make_unique<Mysql>());
  }
}

void MysqlPool::SetMysqlNums(const int & conn_nums){
  conn_nums_ = conn_nums;
}
