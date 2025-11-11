#pragma once

#include <iostream>
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>

#include <vector>
#include <map>

#include "sha256.h"

constexpr unsigned long MSG_LENGTH = 4096;

#if 0
class User
{
private:
  SOCKET conn; // socket to provide connection

  enum struct status
  {
    auth_login,
    auth_password,
    registered_password,
    registered_name,
    connected,
    disconnected
  } state = status::auth_login;

public:
  unsigned char name[33] = {0};  // name :))
  unsigned char login[65]; // = username
  char hash[65];           // hashed password

  friend class Server;     // because we can
};

struct DBUser
{
  unsigned char name[32];
  unsigned char login[64];
  char hash[64];
};

class Server
{
  SOCKET listener;
  unsigned short port;
  WSAData wsaData;
  std::map<int, HANDLE> userThreads;

  enum class status : UINT
  {
    open,
    errSocketInit,
    errSocketBind,
    errSocketListen,
    close
  } state = status::close;

  struct HandlerParam
  {
    Server *serv;
    int index;
  };
  std::map<int, User> users;
  std::map<std::string, User> userNames;

public:

  Server( unsigned short newPort = 8080 );

  status start( void );
  status restart( void );
  void stop( void );

  void listenConnections( void );
  void handleMessage( int index );
  void sendMessage( int index, const std::string &msg );
  void sendAllUsers( const std::string &msg );

  static void clientHandler( HandlerParam *param );

  ~Server( void );
};
#endif