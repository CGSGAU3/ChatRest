#include "server.h"

Server::Server( unsigned short newPort ) : port(newPort)
{
  state = start();
}

Server::status Server::start( void )
{
  system("chcp 1251 > nul");

  WORD DLLVersion = MAKEWORD(2, 2);
  if (WSAStartup(DLLVersion, &wsaData) != 0)
  {
    std::cout << "Error init" << std::endl;
    return status::errSocketInit;
  }

  SOCKADDR_IN addr{};
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  addr.sin_family = AF_INET;

  listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (listener == SOCKET_ERROR)
    return state = status::errSocketInit;

  if (bind(listener, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
    return state = status::errSocketBind;

  if (listen(listener, SOMAXCONN) == SOCKET_ERROR)
    return state = status::errSocketListen;

  state = status::open;
  std::cout << "Server started!" << std::endl;
  return state;
}

void Server::listenConnections( void )
{
  SOCKET newConnection;
  SOCKADDR_IN addr{};
  int sizeofaddr = sizeof(addr);

  while (state == status::open)
  {
    newConnection = accept(listener, (SOCKADDR*)&addr, &sizeofaddr);

    if (newConnection == 0)
    {
      std::cout << "Error connect!\n";
    }
    else
    {
      std::cout << "Client connected!\n";
      char msg[MSG_LENGTH] = "Добро пожаловать в ацкий сервер!\r\n";
      send(newConnection, msg, sizeof(msg), 0);

      int i = 0;

      while (users.find(i) != users.end())
        i++;

      users[i].conn = newConnection;
      users[i].state = User::status::auth_login;

      memset(msg, 0, MSG_LENGTH);
      strcpy(msg, "Введите логин: ");
      send(newConnection, msg, sizeof(msg), 0);

      HandlerParam param = {this, i};
      userThreads[i] = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)clientHandler, &param, NULL, NULL);
    }
  }
}

void Server::clientHandler( HandlerParam *param )
{
  Server *serv = param->serv;
  int index = param->index;

  while (serv->state == status::open && serv->users[index].state != User::status::disconnected)
    serv->handleMessage(index);
}

void Server::handleMessage( int index )
{
  char msg[MSG_LENGTH];

  memset(msg, 0, MSG_LENGTH);
  int flag = recv(users[index].conn, msg, sizeof(msg), 0);

  if (flag == -1 || flag == 0)
  {
    std::cout << "Client disconnected!" << std::endl;

    if (users[index].state != User::status::connected)
    {
      userNames.erase((char *)users[index].login);
      users.erase(index);
      users[index].state = User::status::disconnected;
    }
    else
    {
      users[index].state = User::status::disconnected;
      if (std::string((char *)users[index].name) != "")
      {
        sprintf(msg, "           %s ливнул с ацкого сервера!\r\n", users[index].name);
        sendAllUsers(msg);
      }
    }
    return;
  }

  if (users[index].state != User::status::auth_password &&
     users[index].state != User::status::registered_password)
    std::cout << msg << std::endl;

  std::string strMsg = msg;
  std::string addon;

  switch (users[index].state)
  {
  case User::status::auth_login:
    strncpy((char *)users[index].login, msg, 64);
    users[index].login[64] = 0;

    addon = (char *)users[index].login;
    if (userNames.find(strMsg) != userNames.end())
    {
      SOCKET temp = users[index].conn;;
      users[index] = userNames[addon];
      users[index].conn = temp;
      sendMessage(index, "\r\nВведите пароль: ");
      users[index].state = User::status::auth_password;
    }
    else
    {
      userNames[addon] = users[index];
      sendMessage(index, "\r\nПридумайте пароль: ");
      users[index].state = User::status::registered_password;
    }
    break;
  case User::status::auth_password:
    addon = (char *)users[index].login;
    if (strcmp(SHA256(strMsg).c_str(), userNames[addon].hash) == 0)
    {
      sendMessage(index, "\r\nПодключено!\r\n");
      users[index].state = User::status::connected;
      sprintf(msg, "           %s зашел на ацкий сервер\r\n", users[index].name);
      sendAllUsers(msg);
    }
    else
    {
      sendMessage(index, "\r\nНеверный пароль!\r\n");
    }
    break;
  case User::status::registered_password:
    strncpy((char *)users[index].hash, SHA256(strMsg).c_str(), 64);
    users[index].hash[64] = 0;
    users[index].state = User::status::registered_name;
    sendMessage(index, "\r\nВведите имя (до 32 символов): ");
    break;
  case User::status::registered_name:
    strncpy((char *)users[index].name, strMsg.c_str(), 32);
    users[index].name[32] = 0;
    addon = (char *)users[index].login;
    userNames[addon] = users[index];
    sendMessage(index, "\r\nЗарегистрировано!\r\n");
    users[index].state = User::status::connected;
    sprintf(msg, "           %s зашел на ацкий сервер\r\n", users[index].name);
    sendAllUsers(msg);
    break;
  case User::status::connected:
    for (const auto &user : users)
    {
      if (user.second.state == User::status::connected)
      {
        if (user.first == index)
        {
          char msgBetter[MSG_LENGTH] = {0};
          sprintf(msgBetter, "%s: %s\r\n", "You", msg);
          send(user.second.conn, msgBetter, sizeof(msgBetter), 0);
        }
        else
        {
          char msgBetter[MSG_LENGTH] = {0};
          sprintf(msgBetter, "%s: %s\r\n", users[index].name, msg);
          send(user.second.conn, msgBetter, sizeof(msgBetter), 0);
        }
      }
    }
    break;
  }
}

void Server::sendAllUsers( const std::string &msg )
{
  for (const auto &user : users)
  {
    if (user.second.state == User::status::connected)
      sendMessage(user.first, msg);
  }
}

void Server::sendMessage( int index, const std::string &msg )
{
  if (state == Server::status::open && users[index].state != User::status::disconnected)
    send(users[index].conn, msg.c_str(), (int)msg.length(), 0);
}

void Server::stop( void )
{
  state = status::close;
  closesocket(listener);

  for (const auto &handle : userThreads)
    WaitForSingleObject(handle.second, INFINITE);

  userThreads.clear();
  users.clear();
}

Server::status Server::restart( void )
{
  if (state == status::open)
    stop();
  return start();
}

Server::~Server( void )
{
  if (state == status::open)
    stop();
  WSACleanup();
}