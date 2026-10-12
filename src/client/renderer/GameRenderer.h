#ifndef NET_MINECRAFT_CLIENT_RENDERER__GameRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER__GameRenderer_H__

//package net.minecraft.client.renderer;

#include "gles.h"
#include <cstddef>
#include "../../util/SmoothFloat.h"
#include "../../world/phys/Vec3.h"
#include "../gui/components/ImageButton.h"

class Minecraft;
class Entity;
class ItemInHandRenderer;
class LevelRenderer;
class GameRenderer
{
public:
	GameRenderer(Minecraft* mc_);
	~GameRenderer();

	void pick(float a);

	void updateAllChunks();

	void zoomRegion(float zoom, float xa, float ya);
	void unZoomRegion();
	void setupGuiScreen(bool clearColorBuffer);

	void tick(int nTick, int maxTick);
	void render(double a);
	void renderLevel(double a);
	void renderItemInHand(float a, int eye);

	void onGraphicsReset();

	void setupCamera(double a, int eye);
	void moveCameraToPlayer(double a);

	void setupClearColor(float a);
	double getFov(double a, bool applyEffects);
private:
	void setupFog(int i);

	void tickFov();
	

	void bobHurt(double a);
	void bobView(double a);

	bool updateFreeformPickDirection(float a, Vec3& outDir);
	void prepareAndRenderClouds(LevelRenderer* levelRenderer, double a);

public:
	ItemInHandRenderer* itemInHandRenderer;
	double getRenderDistance() const { return renderDistance; }

private:
	Minecraft* mc;

	double renderDistance;
	int _tick;
	Vec3 pickDirection;

	// smooth camera movement
	SmoothFloat smoothTurnX;
	SmoothFloat smoothTurnY;

	//    // third-person distance etc
	//    SmoothFloat smoothDistance = /*new*/ SmoothFloat();
	//    SmoothFloat smoothRotation = /*new*/ SmoothFloat();
	//    SmoothFloat smoothTilt = /*new*/ SmoothFloat();
	//    SmoothFloat smoothRoll = /*new*/ SmoothFloat();

	float thirdDistance;
	float thirdDistanceO;
	float thirdRotation;
	float thirdRotationO;
	float thirdTilt;
	float thirdTiltO;

	// zoom
	double zoom;
	double zoom_x;
	double zoom_y;

	// fov modification
	double fov, oFov;
	double fovOffset;
	double fovOffsetO;
	double _setupCameraFov;

	// roll modification
	double cameraRoll;
	double cameraRollO;

	float fr;
	float fg;
	float fb;

	float fogBrO, fogBr;

	double _rotX;
	double _rotY;
	double _rotXlast;
	double _rotYlast;
	double _lastTickT;

	void saveMatrices();
	float lastProjMatrix[16];
	float lastModelMatrix[16];

	// Scissor area that Minecraft::screen defines
	bool useScreenScissor;
	IntRectangle screenScissorArea;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER__GameRenderer_H__*/
