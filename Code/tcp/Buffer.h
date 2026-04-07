#pragma once
#include<string>
#include<vector>
#include"Common.h"

static const int kInitialSize = 1024;
static const int kPrePendIndex = 8;

class Buffer{
public:
  Buffer();
  ~Buffer();

  DISALLOW_COPY_AND_MOVE(Buffer);

  char *begin();
  const char *begin() const;

  char *beginread();
  const char *beginread() const;

  char *beginwrite();
  const char *beginwrite() const;


  void Append(const char *message);
  void Append(const char *message, int len);
  void Append(const std::string &message);

  int readablebytes() const;
  int writablebytes() const;
  int prependablebytes() const;


  char *Peek();
  const char *Peek() const;
  std::string PeekAsString(int len);
  std::string PeekAllAsString();

  void Retrieve(int len);
  std::string RetrieveAsString(int len);

  void RetrieveAll();
  std::string RetrieveAllAsString();

  void RetrieveUntil(const char *end);
  std::string RetrieveUntilAsString(const char *end);


  void EnsureWritableBytes(int len);

private:
  std::vector<char> buffer_;
  int read_index_;
  int write_index_;
  
};

