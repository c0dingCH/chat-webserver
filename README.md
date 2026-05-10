<img width="925" height="195" alt="image" src="https://github.com/user-attachments/assets/5cd34224-ef0a-4769-a17e-3655f93356bb" /># C++ Chat Server

一个基于Linux网络编程的高性能C++聊天服务器，采用Muduo风格的Reactor模式架构。

## 🚀 项目亮点

### 1. **高性能网络架构**
- **Reactor模式**：主从Reactor多线程模型，事件驱动
- **非阻塞IO + ET模式**：边缘触发epoll，最大化性能
- **线程池**：工作线程池处理业务逻辑，避免阻塞事件循环
- **连接管理**：智能连接生命周期管理，支持自动关闭超时连接

### 2. **完整的协议支持**
- **HTTP/1.1**：完整的HTTP协议解析与处理
- **HTTP/2.0**：基于nghttp2的HTTP/2支持

### 3. **数据库与缓存**
- **MySQL连接池**：高效数据库连接管理
- **Redis缓存层**：二级缓存策略，自动同步MySQL数据
- **消息持久化**：聊天消息的数据库存储与缓存
- **连接池模板**：通用的连接池设计模式

### 4. **异步日志系统**
- **多级缓冲**：前端缓冲+后端文件写入
- **异步写入**：日志线程独立，不影响主业务性能
- **滚动文件**：按大小和时间自动滚动日志文件
- **多级别日志**：DEBUG/INFO/WARN/ERROR/FATAL分级

### 5. **定时器系统**
- **红黑树管理定时器**：高效的定时任务调度、支持cancel
- **Timerfd精确时间控制**：微秒级定时精度

### 6. **业务功能**
- **用户系统**：注册、登录、注销、个人资料
- **文件传输**：上传、下载功能
- **实时聊天**：消息发送、接收、历史记录查询
- **会话管理**：用户会话状态维护

### 7. **代码质量与工程化**
- **Google编码规范**：严格的代码风格约束
- **CMake构建系统**：跨平台构建支持
- **单元测试**：全面的测试覆盖
- **-Werror编译**：零警告策略
- **智能指针管理**：自动内存管理，避免泄漏

## 📁 项目结构

```
chat-main/
├── Code/
│   ├── base/           # 基础工具类
│   │   ├── Common.h    # 通用宏定义
│   │   ├── CurrentThread.cpp/h  # 线程工具
│   │   └── Latch.h     # 同步原语
│   ├── tcp/           # Reactor核心
│   │   ├── EventLoop.cpp/h      # 事件循环
│   │   ├── Channel.cpp/h        # 事件通道
│   │   ├── Poller.cpp/h         # IO多路复用
│   │   ├── Acceptor.cpp/h       # 连接接收器
│   │   ├── TcpConnection.cpp/h  # TCP连接
│   │   ├── TcpServer.cpp/h      # TCP服务器
│   │   └── Buffer.cpp/h         # 缓冲区
│   ├── http1/          # HTTP协议
│   │   ├── HttpContext.cpp/h    # HTTP上下文
│   │   ├── HttpRequest.cpp/h    # HTTP请求
│   │   ├── HttpResponse.cpp/h   # HTTP响应
│   │   └── HttpServer.cpp/h     # HTTP服务器
│   ├── http2/         # HTTP/2.0
│   │   ├── HttpsContext.cpp/h   # HTTPS上下文
│   │   ├── HttpsRequest.cpp/h   # HTTPS请求
│   │   ├── HttpsResponse.cpp/h  # HTTPS响应
│   │   └── HttpsServer.cpp/h    # HTTPS服务器
│   ├── timer/         # 定时器
│   │   ├── Timer.cpp/h          # 定时器
│   │   ├── TimerId.cpp/h        # 定时器ID
│   │   └── TimerQueue.cpp/h     # 定时器队列
│   ├── log/           # 日志系统
│   │   ├── Logging.cpp/h        # 日志接口
│   │   ├── LogStream.cpp/h      # 日志流
│   │   ├── LogFile.cpp/h        # 日志文件
│   │   └── AsyncLogging.cpp/h   # 异步日志
│   ├── database/      # 数据库
│   │   ├── Mysql.cpp/h          # MySQL操作
│   │   ├── MysqlPool.cpp/h      # MySQL连接池
│   │   ├── Redis.cpp/h          # Redis操作
│   │   └── RedisPool.cpp/h      # Redis连接池
│   ├── business/      # 业务逻辑
│   │   └── User.cpp/h           # 用户管理
│   ├── test/          # 测试代码
│   │   ├── echo_server.cpp      # Echo服务器测试
│   │   ├── http_server.cpp      # HTTP服务器测试
│   │   ├── test_mysql.cpp       # MySQL测试
│   │   └── function_test.cpp    # 功能测试
│   └── CMakeLists.txt # 构建配置
├── LogFiles/          # 日志文件目录
├── public/            # 静态文件目录
├── client_files/      # 客户端文件
└── AGENTS.md          # 项目详细文档
```

## 🛠️ 技术栈

- **语言**: C++17
- **网络**: epoll, Reactor模式, 非阻塞IO
- **协议**: HTTP/1.1, HTTP/2.0, WebSocket
- **数据库**: MySQL 8, Redis
- **构建**: CMake 3.10+
- **编译**: g++ with -Werror -std=c++17
- **依赖**: pthread, mysqlclient, nghttp2, redis++

## 📦 构建与运行

### 环境要求
- Linux系统
- g++ 支持C++17
- MySQL 8.0+
- Redis 6.0+
- CMake 3.10+

### 构建步骤
```bash
cd Code
mkdir -p build && cd build
cmake .. && make
```

### 运行测试
```bash
# 运行HTTP服务器测试
./build/test/http_server

# 运行客户端功能测试
./build/test/http_server

```  

### 压力测试截图
虚拟机cpu参数：1 * 4   
纯粹的tcp压测，http只重写了onmessage，固定的字节回复  
<img width="931" height="196" alt="70ce0d6c-5a7e-497b-bd36-b6e7a0d2c02a" src="https://github.com/user-attachments/assets/b1d14589-f325-4fcb-b214-e5cea46005ec" />

   
<img width="925" height="195" alt="image" src="https://github.com/user-attachments/assets/00074758-badd-4093-8ae4-91e6fa36ac5e" />




### 生成库文件
构建完成后会生成动态库：
- `build/libchat.so` - 主聊天服务器库

## 🔧 配置说明

### 服务器配置
- **线程数**: 通过`SetThreadNums()`设置工作线程数
- **连接池**: 通过`SetMysqlNums()`设置数据库连接数
- **自动关闭**: 通过`SetAutoCloseConn()`设置连接自动关闭
- **超时时间**: 默认连接超时10秒

### 日志配置
- **日志级别**: DEBUG < INFO < WARN < ERROR < FATAL
- **输出位置**: 可配置控制台或文件输出
- **异步写入**: 默认启用异步日志，提高性能

## 🧪 测试覆盖

项目包含完整的测试套件：
- **网络测试**: Echo服务器、HTTP客户端/服务器
- **数据库测试**: MySQL连接、Redis缓存
- **功能测试**: 用户注册登录、文件传输
- **性能测试**: 使用webbench进行压力测试
- **单元测试**: 各模块独立测试

## 📈 性能特点

1. **高并发**: Reactor模式支持数千并发连接
2. **低延迟**: 非阻塞IO和事件驱动减少响应时间
3. **高吞吐**: 异步日志和数据库操作不阻塞主线程
4. **可扩展**: 模块化设计，易于添加新功能
5. **稳定性**: 完善的错误处理和资源管理

## 🔒 安全特性

- **输入验证**: 所有用户输入都经过验证
- **SQL防注入**: 参数化查询防止SQL注入
- **连接安全**: HTTPS支持加密通信
- **资源限制**: 连接数和请求大小限制
- **错误隔离**: 业务错误不影响服务器稳定性

## 📚 学习价值

这个项目是学习以下技术的优秀案例：
- C++17现代特性应用
- Linux网络编程最佳实践
- 高性能服务器架构设计
- 数据库连接池实现
- 异步编程模式
- 工程化C++项目组织

## 🤝 贡献指南

1. 遵循Google C++编码规范
2. 所有代码变更需通过现有测试
3. 新功能需添加相应测试用例
4. 提交前运行`make`确保编译通过
5. 使用有意义的提交信息

## 📄 许可证

本项目仅供学习交流使用。

---

*基于Muduo网络库设计思想，专注于高性能和可维护性的C++服务器实现。*
