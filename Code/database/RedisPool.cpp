#include "RedisPool.h"
#include "MysqlPool.h"
#include "Redis.h"
#include <thread>

RedisPool::RedisPool(MysqlPool* mysql_pool)
  : pool_size_(std::thread::hardware_concurrency() * 2),
    mysql_pool_(mysql_pool) {}

RedisPool::~RedisPool() {}

void RedisPool::Start() {
  CreateConns();
}

std::unique_ptr<Redis> RedisPool::Get() {
  std::unique_ptr<Redis> conn;
  {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [&]() { return !conns_.empty(); });

    conn = std::move(conns_.front());
    conns_.pop();
  }

  return conn;
}

void RedisPool::Put(std::unique_ptr<Redis>& conn) {
  {
    std::unique_lock<std::mutex> lock(mtx_);
    conns_.push(std::move(conn));
  }
  cv_.notify_one();
}

void RedisPool::SetPoolSize(size_t size) {
  pool_size_ = size;
}

void RedisPool::SetMysqlPool(MysqlPool* pool) {
  mysql_pool_ = pool;
}

void RedisPool::CreateConns() {
  for (size_t i = 0; i < pool_size_; ++i) {
    conns_.emplace(std::make_unique<Redis>(mysql_pool_));
  }
}
