#pragma once

class Server
{
public:
	Server();
	static Server *GetInstance()
	{
		if (myInstance == nullptr)
		{
			myInstance = new Server();
		}
		return myInstance;
	}

private:
	static Server *myInstance;
};