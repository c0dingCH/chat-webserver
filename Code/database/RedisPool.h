#pragma once

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>

class MysqlPool;
class Redis;

class RedisPool {
public:
  explicit RedisPool(MysqlPool* mysql_pool);
  ~RedisPool();

  std::unique_ptr<Redis> Get();
  void Put(std::unique_ptr<Redis>& conn);

  void Start();
  void CreateConns();
  
  void SetPoolSize(size_t size);
  void SetMysqlPool(MysqlPool* pool);

  template<typename Func>
  void WithConnection(Func&& func);

private:
  std::queue<std::unique_ptr<Redis>> conns_;
  std::mutex mtx_;
  std::condition_variable cv_;
  size_t pool_size_;
  MysqlPool* mysql_pool_;
};

template<typename Func>
void RedisPool::WithConnection(Func&& func) {
  auto conn = Get();
  func(conn);
  Put(conn);
}
