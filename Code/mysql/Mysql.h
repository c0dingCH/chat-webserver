#pragma once
#include <mysql/mysql.h>
#include <string>
#include <vector>


class Mysql {
public:
  Mysql();
  ~Mysql();

  bool Insert(std::string table, std::vector<std::pair<std::string, std::string>> field_vals);
  bool Insert(std::string table, std::vector<std::pair<std::string, std::string>> field_vals, std::string &msg);

  bool Select(std::string table, std::vector<std::pair<std::string, std::string>> field_vals, std::string &msg);

  bool Delete(std::string table, std::vector<std::pair<std::string, std::string>> field_vals);
  bool Delete(std::string table, std::vector<std::pair<std::string, std::string>> field_vals, std::string &msg);

  bool Update(std::string table, std::vector<std::pair<std::string, std::string>> field_vals,
              std::vector<std::pair<std::string, std::string>> new_field_vals);
  bool Update(std::string table, std::vector<std::pair<std::string, std::string>> field_vals,
              std::vector<std::pair<std::string, std::string>> new_field_vals, std::string &msg);

private:
  bool ValidateTable(const std::string &table);
  bool ValidateFields(const std::string &table, const std::vector<std::pair<std::string, std::string>> &fields);

  MYSQL conn_{};
};