#ifndef NET_MINECRAFT_CLIENT__MouseHandler_H__
#define NET_MINECRAFT_CLIENT__MouseHandler_H__

//package net.minecraft.client;

class ITurnInput;
class AppPlatform;

class MouseHandler
{
public:
	MouseHandler(ITurnInput* turnInput);
	MouseHandler();
	~MouseHandler();

	void setTurnInput(ITurnInput* turnInput);
	void setPlatform(AppPlatform* platform);

    void grab();
    void release();

    void poll();

    float xd, yd;
private:
	int toSkip;
	ITurnInput* _turnInput;
	AppPlatform* _platform;
};

#endif /*NET_MINECRAFT_CLIENT__MouseHandler_H__*/
