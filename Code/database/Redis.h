#pragma once
#include <sw/redis++/redis++.h>
#include <string>
#include <vector>
#include <mutex>
#include <random>

class MysqlPool;
class RedisPool;

class Redis {
public:
  explicit Redis(MysqlPool* mysql_pool = nullptr);
  ~Redis();

  bool Insert(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals);
  bool Insert(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals, std::string& msg);

  bool Select(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals, std::string& msg);

  bool Delete(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals);
  bool Delete(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals, std::string& msg);

  bool Update(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals,
              const std::vector<std::pair<std::string, std::string>>& new_field_vals);
  bool Update(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals,
              const std::vector<std::pair<std::string, std::string>>& new_field_vals, std::string& msg);

  std::vector<std::string> GetRecentMsgs(std::string sender_id, std::string receiver_id, int n, std::string& msg);

private:
  bool ValidateTable(const std::string &table);
  bool ValidateFields(const std::string &table, const std::vector<std::pair<std::string, std::string>> &fields);

  void CacheFromMysql(const std::string &table, const std::vector<std::pair<std::string, std::string>> &field_vals,
                     bool found, const std::string &mysql_result);

  std::string BuildMsgKey(const std::string &user1, const std::string &user2);
  int GetRandomTtl();

  std::unique_ptr<sw::redis::Redis> conn_;
  MysqlPool* mysql_pool_{nullptr};
  std::mutex mtx_{};

};
