#ifndef NET_MINECRAFT_CLIENT_RENDERER__RenderList_H__
#define NET_MINECRAFT_CLIENT_RENDERER__RenderList_H__

//package net.minecraft.client.renderer;

class RenderChunk;

class RenderList
{
public:
	RenderList();
	~RenderList();

    void init(double xOff, double yOff, double zOff);

	void add(int list);
	void addR(const RenderChunk& chunk);

	void next();

    void render();
	void renderChunks();

    void clear();


	double xOff, yOff, zOff;
	int* lists;
	RenderChunk* rlists;

	int listIndex;
	bool inited;
	bool rendered;

private:
	void ensureCapacity(int needed);

	int capacity;
	int bufferLimit;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER__RenderList_H__*/
