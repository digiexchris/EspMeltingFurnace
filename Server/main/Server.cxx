#include "Server.hxx"
#include <esp_log.h>

Server *Server::myInstance = nullptr;

static const char *ServerTAG = "Server";

Server::Server()
{
	ESP_LOGI(ServerTAG, "Server initialized");
}