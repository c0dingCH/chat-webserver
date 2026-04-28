#pragma once
#include <mysql/mysql.h>
#include <string>
#include <vector>


class Mysql {
public:
  Mysql();
  ~Mysql();

  bool Insert(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals);
  bool Insert(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals, std::string& msg);

  bool Select(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals, std::string& msg);

  bool Delete(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals);
  bool Delete(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals, std::string& msg);

  bool Update(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals,
              const std::vector<std::pair<std::string, std::string>>& new_field_vals);
  bool Update(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals,
              const std::vector<std::pair<std::string, std::string>>& new_field_vals, std::string& msg);

  std::vector<std::string> SelectMsgs(const std::string& sender_id, const std::string& receiver_id, int n, std::string& msg);

private:
  bool ValidateTable(const std::string &table);
  bool ValidateFields(const std::string &table, const std::vector<std::pair<std::string, std::string>> &fields);

  MYSQL conn_{};
};