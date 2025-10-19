#include "server.h"

int main( int argc, char *argv[] )
{
  Server serv;

  serv.listenConnections();
  system("pause");

  return 0;
}