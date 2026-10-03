#include "Minecraft.h"
#include "Options.h"
#include "client/Options.h"
#include "client/player/input/IBuildInput.h"
#include "platform/input/Keyboard.h"
#include "world/item/Item.h"
#include "world/item/ItemInstance.h"
#include <string>
#include <cstdlib>


#if defined(APPLE_DEMO_PROMOTION)
#define NO_NETWORK
#endif

#if defined(RPI)
#define CREATORMODE
#endif
#include "../network/RakNetInstance.h"
#include "../network/ClientSideNetworkHandler.h"
#include "../network/ServerSideNetworkHandler.h"
//#include "../network/Packet.h"
#include "../world/entity/player/Inventory.h"
#include "../world/level/tile/Tile.h"
#include "../world/level/storage/LevelStorageSource.h"
#include "../world/level/storage/LevelStorage.h"
#include "player/input/KeyboardInput.h"
#include "player/input/ControllerTurnInput.h"
#include "player/input/XperiaPlayInput.h"
#include "world/level/chunk/ChunkSource.h"

#ifndef STANDALONE_SERVER
#include "player/input/touchscreen/TouchInputHolder.h"
#include "particle/ParticleEngine.h"
#include "gui/Screen.h"
#include "gui/Font.h"
#ifndef STANDALONE_SERVER
#include "gui/screens/BetaInventoryScreen.h"
#include "gui/screens/DisconnectionScreen.h"
#endif
#include "gui/screens/RenameMPLevelScreen.h"
#include "gui/screens/ConsoleScreen.h"
#include "gui/screens/ChatScreen.h"
#include "sound/SoundEngine.h"
#include "player/input/touchscreen/TouchscreenInput.h"
#include "renderer/Chunk.h"
#include "gui/screens/PrerenderTilesScreen.h"
#include "renderer/Textures.h"
#include "gui/screens/DeathScreen.h"
#include "gui/screens/FurnaceScreen.h"
#include "gui/screens/ArmorScreen.h"
#include "renderer/tileentity/TileEntityRenderDispatcher.h"
#include "renderer/ptexture/DynamicTexture.h"
#include "renderer/GameRenderer.h"
#include "renderer/ItemInHandRenderer.h"
#include "renderer/LevelRenderer.h"
#include "gui/Font.h"
#include "gui/screens/RenameMPLevelScreen.h"
#include "../platform/PngLoader.h"
#include "renderer/culling/AllowAllCuller.h"
#include "renderer/entity/EntityRenderDispatcher.h"
#include "renderer/Lighting.h"
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include "sound/SoundEngine.h"
#endif // STANDALONE_SERVER

#include "player/LocalPlayer.h"
#include "gamemode/CreativeMode.h"
#include "gamemode/SurvivalMode.h"
#include "player/LocalPlayer.h"
#include "../platform/CThread.h"
#include "../platform/input/Mouse.h"
#include "../AppPlatform.h"
#include "../LicenseCodes.h"
#include "../util/PerfTimer.h"
#include "../util/PerfRenderer.h"
#include "player/input/MouseBuildInput.h"

#include "player/input/IInputHolder.h"

#include "player/input/MouseTurnInput.h"
#include "../world/entity/MobFactory.h"
#include "../world/level/MobSpawner.h"
#include "../util/Mth.h"
#include "../network/packet/InteractPacket.h"
#include "../network/packet/RespawnPacket.h"
#include "../network/packet/AdventureSettingsPacket.h"
#include "../network/packet/SetSpawnPositionPacket.h"
#include "IConfigListener.h"
#include "../world/entity/MobCategory.h"
#include "../world/Difficulty.h"
#include "../server/ServerLevel.h"

#ifdef CREATORMODE
#include "../server/CreatorLevel.h"
#endif

#include "../network/command/CommandServer.h"
#include "gamemode/CreatorMode.h"

#include "../world/level/GrassColor.h"
#include "renderer/LevelRenderer.h"
#include "particle/ParticleEngine.h"
#include "../world/level/Level.h"
static void checkGlError(const char* tag) {
#ifdef GLDEBUG
	while (1) {
		const int errCode = glGetError();
		if (errCode == GL_NO_ERROR) break;

		LOGE("################\nOpenGL-error @ %s : #%d\n", tag, errCode);
	}
#endif /*GLDEBUG*/
}

/*static*/
const char* Minecraft::progressMessages[] = {
	"Locating server",
	"Building terrain",
	"Preparing",
	"Saving chunks"
};

int Minecraft::customDebugId = Minecraft::CDI_NONE;

#if defined(_MSC_VER)
#pragma warning( disable : 4355 ) // 'this' pointer in initialization list which is perfectly legal
#endif

bool Minecraft::useAmbientOcclusion = false;

Minecraft::Minecraft() :	
	level(NULL),
	player(NULL),
	cameraTargetPlayer(NULL),
	levelRenderer(NULL),
	gameRenderer(NULL),
#ifndef STANDALONE_SERVER
	particleEngine(NULL),
	_perfRenderer(NULL),
#endif
	_commandServer(NULL),
#ifndef STANDALONE_SERVER
	textures(NULL),
#endif
	lastTickTime(-1),
	lastTime(0),
	ticksSinceLastUpdate(0),
	gameMode(NULL),
	mouseGrabbed(true),
	missTime(0),
	pause(false),
	_running(false),
	timer(20),
#ifndef STANDALONE_SERVER
	gui(this),
#endif
	netCallback(NULL),
#ifndef STANDALONE_SERVER
	screen(NULL),
	font(NULL),
#endif
	screenMutex(false),
#ifndef STANDALONE_SERVER
	scheduledScreen(NULL),
	hasScheduledScreen(false),
	soundEngine(NULL),
#endif
	ticks(0),
	isGeneratingLevel(false),
	_hasSignaledGeneratingLevelFinished(true),
	generateLevelThread(NULL),
	progressStagePercentage(0),
	progressStageStatusId(0),
	isLookingForMultiplayer(false),
	_licenseId(LicenseCodes::WAIT_PLATFORM_NOT_READY),
	inputHolder(0),
	_supportsNonTouchscreen(false),
#ifndef STANDALONE_SERVER
	screenChooser(this),
#endif
	width(1), height(1),
	//_respawnPlayerTicks(-1),
#ifdef __APPLE__
	_isSuperFast(false),
#endif
	_powerVr(false),
	commandPort(4711),
	reserved_d1(0),reserved_d2(0),
	reserved_f1(0),reserved_f2(0), options(this)
{
	//#ifdef ANDROID

#if defined(NO_NETWORK)
	raknetInstance = new IRakNetInstance();
#else
	raknetInstance = new RakNetInstance();
#endif
#ifndef STANDALONE_SERVER
	soundEngine = new SoundEngine(20.0f);
	soundEngine->init(this, &options);
#endif
	//setupPieces();
}

Minecraft::~Minecraft()
{
	delete netCallback;
	delete raknetInstance;
#ifndef STANDALONE_SERVER
	delete levelRenderer;
	delete gameRenderer;
	delete particleEngine;

	delete soundEngine;
#endif
	delete gameMode;
#ifndef STANDALONE_SERVER
	delete font;
	delete textures;

	if (screen != NULL) {
		delete screen;
		screen = NULL;
	}
#endif
	if (level != NULL) {
		level->saveGame();
		if (level->getChunkSource())
			level->getChunkSource()->saveAll(true);
		delete level->getLevelStorage();
		delete level;
		level = NULL;
	}

	//delete player;
	delete inputHolder;

	delete storageSource;
	delete _perfRenderer;
	delete _commandServer;

	MobFactory::clearStaticTestMobs();
#ifndef STANDALONE_SERVER
	EntityRenderDispatcher::destroy();
#endif
}

// Only called by server
bool Minecraft::selectLevel( const std::string& levelId, const std::string& levelName, const LevelSettings& settings, int generatorVersion )
{
#if defined(CREATORMODE)
	level = new CreatorLevel(
#else
	level = new ServerLevel(
#endif
		storageSource->selectLevel(levelId, false),
		levelName,
		settings,
		generatorVersion);

	// Worlds without an explicit generator version predate world types
	// and are refused with a message instead of loading halfway.
	if (!level->isNew() && !level->getLevelData()->hasGeneratorVersion()) {
		delete level->getLevelStorage();
		delete level;
		level = NULL;
#ifndef STANDALONE_SERVER
		setScreen(new DisconnectionScreen("Incompatible world: missing generator version."));
#endif
		return false;
	}

	// note: settings is useless beyond this point, since it's
	//       either copied to LevelData (or LevelData read from file)
	setLevel(level, "Generating level");
	setIsCreativeMode(level->getLevelData()->getGameType() == GameType::Creative);
	_running = true;
	return true;
}

void Minecraft::setLevel(Level* level, const std::string& message /* ="" */, LocalPlayer* forceInsertPlayer /* = NULL */) {
	cameraTargetPlayer = NULL;
	LOGI("Seed is %ld\n", level->getSeed());

	if (level != NULL) {
		level->raknetInstance = raknetInstance;
		gameMode->initLevel(level);

		if (!player && forceInsertPlayer)
		{
			player = forceInsertPlayer;
			player->resetPos(false);
			//level->addEntity(forceInsertPlayer);
		}
		else if (player != NULL) {
			player->resetPos(false);
			if (level != NULL) {
				level->addEntity(player);
			}
		}
		this->level = level;
		_hasSignaledGeneratingLevelFinished = false;
#ifdef STANDALONE_SERVER
		const bool threadedLevelCreation = false;
#else
		const bool threadedLevelCreation = true;
#endif

		if (threadedLevelCreation) {
			// Threaded
			// "Lock"
			isGeneratingLevel = true;
			generateLevelThread = new CThread(Minecraft::prepareLevel_tspawn, this);
		} else {
			// Non-threaded
			generateLevel("Currently not used", level);
		}
	} else {
		player = NULL;
	}

		this->lastTickTime = 0;
	this->_running = true;
	applyWorldPresentation(level);
}

void Minecraft::applyWorldPresentation(Level* level) {
	int genVersion = level ? level->getLevelData()->getGeneratorVersion() : (int)LGV_ORIGINAL;
	bool beta = genVersion == (int)LGV_BETA173;
	bool alpha = genVersion == (int)LGV_ALPHA112;
	platform()->setWindowTitle(alpha ? "Minecraft Alpha 1.1.2"
		: (beta ? "Minecraft Beta 1.7.3" : "Minecraft PE 0.6.1"));
#ifndef STANDALONE_SERVER
	if (textures)
		textures->setTerrainMode(alpha ? 2 : (beta ? 1 : 0));
#endif
}

void Minecraft::leaveGame(bool renameLevel /*=false*/)
{
	if (isGeneratingLevel || !_hasSignaledGeneratingLevelFinished)
		return;

	isGeneratingLevel = false;
	bool saveLevel = level && (!level->isClientSide || renameLevel);

	raknetInstance->disconnect();
	if (saveLevel) {
		// If server or wanting to save level as client, save all unsaved chunks!
		level->getChunkSource()->saveAll(true);
	}

	LOGI("Clearing levels\n");

	cameraTargetPlayer = NULL;
#ifndef STANDALONE_SERVER
	levelRenderer->setLevel(NULL);
	particleEngine->setLevel(NULL);
#endif
	LOGI("Erasing callback\n");
	delete netCallback;
	netCallback = NULL;

	LOGI("Erasing level\n");
	if (level != NULL) {
		level->clearDynamicLight();
		delete level->getLevelStorage();
		delete level;
		level = NULL;
	}
	applyWorldPresentation(NULL);
	//delete player;
	player = NULL;
	cameraTargetPlayer = NULL;

	_running = false;
#ifndef STANDALONE_SERVER
	gui.clearMessages();
	if (renameLevel) {
		setScreen(new RenameMPLevelScreen(LevelStorageSource::TempLevelId));
	}
	else
		screenChooser.setScreen(SCREEN_STARTMENU);
#endif
}

void Minecraft::prepareLevel(const std::string& title) {
	LOGI("status: 1\n");
	progressStageStatusId = 1;

	Stopwatch A, B, C, D;
	A.start();

	Stopwatch L;

	// Dont update lights if we load the level (ok, actually just with leveldata version=1.+(?))
	if (!level->isNew())
		level->setUpdateLights(false);

	if (level->isNew())
		level->setInitialSpawn();

	const int prepareRadius = 4;
	const int spawnChunkX = Mth::floor(level->getLevelData()->getXSpawn() / (float)CHUNK_WIDTH);
	const int spawnChunkZ = Mth::floor(level->getLevelData()->getZSpawn() / (float)CHUNK_DEPTH);
	int Max = (prepareRadius * 2 + 1) * (prepareRadius * 2 + 1);
	int pp = 0;
	for (int cx = spawnChunkX - prepareRadius; cx <= spawnChunkX + prepareRadius; ++cx) {
		for (int cz = spawnChunkZ - prepareRadius; cz <= spawnChunkZ + prepareRadius; ++cz) {
			progressStagePercentage = 100 * pp++ / Max;
			//printf("level generation progress %d\n", progressStagePercentage);
			B.start();
			level->getTile(cx * CHUNK_WIDTH + 8, 64, cz * CHUNK_DEPTH + 8);
			B.stop();
			L.start();
			if (level->isNew())
				while (level->updateLights())
					;
			L.stop();
		}
	}
	A.stop();
	level->setUpdateLights(true);

	C.start();
	for (int cx = spawnChunkX - prepareRadius; cx <= spawnChunkX + prepareRadius; ++cx) {
		for (int cz = spawnChunkZ - prepareRadius; cz <= spawnChunkZ + prepareRadius; ++cz) {
			LevelChunk* chunk = level->getChunk(cx, cz);
			if (chunk && !chunk->createdFromSave)
			{
				chunk->unsaved = false;
				chunk->clearUpdateMap();
			}
		}
	}
	C.stop();

	LOGI("status: 3\n");
	progressStageStatusId = 3;
	if (level->isNew()) {
		level->saveLevelData();
		level->getChunkSource()->saveAll(false);
		level->saveGame();
	} else {
		level->saveLevelData();
		level->loadEntities();
	}

	progressStagePercentage = -1;
	progressStageStatusId = 2;
	LOGI("status: 2\n");

	D.start();
	level->prepare();
	D.stop();

	A.print("Generate level: ");
	L.print(" - light: ");
	B.print(" - getTl: ");
	C.print(" - clear: ");
	D.print(" - prepr: ");
	progressStageStatusId = 0;
}

void Minecraft::update() {
	//LOGI("Enter Update\n");

	if (Options::debugGl)
		LOGI(">>>>>>>>>>\n");

	TIMER_PUSH("root");

	//if (level) {
	//	LOGI("numplayers: %d\n", level->players.size());
	//	for (int i = 0; i < level->players.size(); ++i) {
	//		Player* p = level->players[i];
	//		bool inEnt = std::find(level->entities.begin(), level->entities.end(), p) != level->entities.end();
	//		LOGI("  %p, %d, %d - in? %d\n", p, p->entityId, p->owner.ToUint32(p->owner), inEnt);
	//	}
	//}

	// If we're paused (local world / invisible server), freeze gameplay and
	// networking and only keep UI responsive.
	bool freezeGame = pause;

	if (!freezeGame) {
		timer.advanceTime();
	}

	if (raknetInstance && !freezeGame) {
		raknetInstance->runEvents(netCallback);
	}

	TIMER_PUSH("tick");
	int toTick = freezeGame ? 1 : timer.ticks;
	if (!freezeGame) timer.ticks = 0;
	for (int i = 0; i < toTick; ++i, ++ticks)
		tick(i, toTick-1);

	TIMER_POP_PUSH("updatelights");
	if (level && !isGeneratingLevel) {
		level->updateLights();
	}
	TIMER_POP();

#ifndef STANDALONE_SERVER
	if (gameMode != NULL) gameMode->render(timer.a);
	TIMER_PUSH("sound");
	soundEngine->update(player, timer.a);
	TIMER_POP_PUSH("render");
	gameRenderer->render(timer.a);
	TIMER_POP();
#else
	CThread::sleep(1);
#endif
#ifndef STANDALONE_SERVER
	Multitouch::resetThisUpdate();
#endif

#ifndef STANDALONE_SERVER
	TIMER_POP();
	checkGlError("Update finished");

	if (options.getBooleanValue(OPTIONS_RENDER_DEBUG)) {
		//#ifndef PLATFORM_DESKTOP
		if (!PerfTimer::enabled) {
			PerfTimer::reset();
			PerfTimer::enabled = true;
		}
		_perfRenderer->renderFpsMeter(1);
		checkGlError("render debug");
		//#endif
	} else {
		PerfTimer::enabled = false;
	}
#endif
	//LOGI("Exit Update\n");
}

void Minecraft::tick(int nTick, int maxTick) {
	if (missTime > 0) missTime--;
	bool traceTick = level && !pause && (ticks % SharedConstants::TicksPerSecond == 0);
	if (traceTick) {
		if (player) {
			MCPE_CRASH_TRACE("[IW] Minecraft::tick begin tick=%d player=(%.2f,%.2f,%.2f) chunk=(%d,%d) chunkStats=%s\n",
				ticks, player->x, player->y, player->z,
				Mth::floor(player->x / 16.0f), Mth::floor(player->z / 16.0f),
				level->gatherChunkSourceStats().c_str());
		} else {
			MCPE_CRASH_TRACE("[IW] Minecraft::tick begin tick=%d no-player chunkStats=%s\n",
				ticks, level->gatherChunkSourceStats().c_str());
		}
	}
#ifndef STANDALONE_SERVER
	if (!screen && player) {
		if (player->health <= 0) {
			setScreen(new DeathScreen());
		}
	}
#endif
	TIMER_PUSH("gameMode");
	if (level && !pause) {
		gameMode->tick();
#ifndef STANDALONE_SERVER
		if (player) {
			ItemInstance* held = player->getSelectedItem();
			int heldLight = 0;
			if (held) {
				int id = held->id;
				if (Tile::torch && id == Tile::torch->id) heldLight = 14;
				else if (Tile::lightGem && id == Tile::lightGem->id) heldLight = 15;
				else if (Tile::glowingObsidian && id == Tile::glowingObsidian->id) heldLight = 14;
			}
			if (heldLight > 0) {
				int px = Mth::floor(player->x);
				int py = Mth::floor(player->y + 0.6f);
				int pz = Mth::floor(player->z);
				level->updateDynamicLight(px, py, pz, heldLight);
			} else {
				level->clearDynamicLight();
			}
		}
#endif
	}

	TIMER_POP_PUSH("commandServer");
	if (level && _commandServer) {
		_commandServer->tick();
	}

	TIMER_POP_PUSH("input");
	tickInput();
#ifndef STANDALONE_SERVER
	TIMER_POP_PUSH("gui");
	gui.tick();
#endif
	//
	// Ongoing level generation in a (perhaps) different thread. When it's
	// ready, _levelGenerated() is called once and any threads are deleted.
	//
	if (isGeneratingLevel) {
		return;
	} else if (!_hasSignaledGeneratingLevelFinished) {
		if (generateLevelThread) {
			delete generateLevelThread;
			generateLevelThread = NULL;
		}
		_levelGenerated();
	}

	//
	// Normal game loop, run before or efter level generation
	//
	if (level != NULL)
	{
		if (!pause) {
#ifndef STANDALONE_SERVER
			TIMER_POP_PUSH("gameRenderer");
			gameRenderer->tick(nTick, maxTick);

			TIMER_POP_PUSH("levelRenderer");
			levelRenderer->tick();
#endif
			level->difficulty = options.getIntValue(OPTIONS_DIFFICULTY);
			if (level->isClientSide) level->difficulty = Difficulty::EASY;

			TIMER_POP_PUSH("level");
			if (traceTick) MCPE_CRASH_TRACE("[IW] Minecraft::tick before tickEntities tick=%d\n", ticks);
			level->tickEntities();
			if (traceTick) MCPE_CRASH_TRACE("[IW] Minecraft::tick after tickEntities tick=%d\n", ticks);
			level->tick();
			if (traceTick) MCPE_CRASH_TRACE("[IW] Minecraft::tick after level tick=%d\n", ticks);
			if (level->isClientSide && netCallback) {
				ClientSideNetworkHandler* clientHandler = dynamic_cast<ClientSideNetworkHandler*>(netCallback);
				if (clientHandler)
					clientHandler->requestChunksAroundPlayer();
			}
#ifndef STANDALONE_SERVER
			TIMER_POP_PUSH("animateTick");
			if (player) {
				if (traceTick) MCPE_CRASH_TRACE("[IW] Minecraft::tick before animateTick tick=%d\n", ticks);
				level->animateTick(Mth::floor(player->x), Mth::floor(player->y), Mth::floor(player->z));
				if (traceTick) MCPE_CRASH_TRACE("[IW] Minecraft::tick after animateTick tick=%d\n", ticks);
			}
#endif
		}
	}
#ifndef STANDALONE_SERVER
	textures->loadAndBindTexture("terrain.png");
	if (!pause && !(screen && !screen->renderGameBehind())) {
#if !defined(RPI)
#ifdef __APPLE__
		if (isSuperFast())
#endif
		{
			if (nTick == maxTick) {
				TIMER_POP_PUSH("textures");
				textures->tick(true);
			}
		}
#endif
	}
	TIMER_POP_PUSH("particles");
	if (!pause) {
		particleEngine->tick();
	}
	if (screen) {
		screenMutex = true;
		screen->tick();
		screenMutex = false;
	}

	// @note: fix to keep "isPressed" and "isReleased" as long as necessary.
	//      Most likely keyboard and mouse could/should be reset here as well.
	Multitouch::reset();
#endif
	TIMER_POP();
}

class InputRAII {
public:
	~InputRAII() {
#ifndef STANDALONE_SERVER
		Mouse::reset();
		Keyboard::reset();
#endif
	}
};

void Minecraft::tickInput() {
#ifndef STANDALONE_SERVER
	InputRAII raiiInput;

	if (screen && !screen->passEvents) {
		screenMutex = true;
		screen->updateEvents();
		//screen->updateSetScreen();
		screenMutex = false;
		if (hasScheduledScreen) {
			setScreen(scheduledScreen);
			scheduledScreen = NULL;
			hasScheduledScreen = false;
		}
		return;
	}

	if (!player) {
		return;
	}

#ifdef RPI
	bool mouseDiggable = true;
	bool allowGuiClicks = !mouseGrabbed;
#else
	bool mouseDiggable = !gui.isInside(Mouse::getX(), Mouse::getY());
	bool allowGuiClicks = true;
#endif

	TIMER_PUSH("mouse");
	while (Mouse::next()) {
		//if (Mouse::getButtonState(MouseAction::ACTION_LEFT))
		//	LOGI("mouse-down-at: %d, %d\n", Mouse::getX(), Mouse::getY());
		int passedTime = getTimeMs() - lastTickTime;
		if (passedTime > 200) continue; // @note: As long Mouse::clear CLEARS the whole buffer, it's safe to break here
		// But since it might be rewritten anyway (and hopefully there aren't a lot of messages, we just continue.

		const MouseAction& e = Mouse::getEvent();

		if (!useTouchscreen() && !mouseGrabbed) {
			if (!screen && e.data == MouseAction::DATA_DOWN) {
				grabMouse();
			}
		}

		if (allowGuiClicks && e.action == MouseAction::ACTION_LEFT && e.data == MouseAction::DATA_DOWN) {
			gui.handleClick(MouseAction::ACTION_LEFT, Mouse::getX(), Mouse::getY());
		}

		if (e.action == MouseAction::ACTION_WHEEL) {
			// If chat/console is open, use the wheel to scroll through chat history.
			if (screen && (dynamic_cast<ChatScreen*>(screen) || dynamic_cast<ConsoleScreen*>(screen))) {
				gui.scrollChat(e.dy);
			} else {
				Inventory* v = player->inventory;

				int numSlots = gui.getNumSlots();
#if defined(PLATFORM_DESKTOP)
				// Desktop has no "..." button: all link slots scroll.
#else
				if (!useTouchscreen()) numSlots--;
#endif

				int slot = (v->selected - e.dy + numSlots) % numSlots;
				v->selectSlot(slot);
			}
		}
		/*
		if (mouseDiggable && options.useMouseForDigging) {
		if (Mouse::getEventButton() == MouseAction::ACTION_LEFT && Mouse::getEventButtonState()) {
		handleMouseClick(MouseAction::ACTION_LEFT);
		lastClickTick = ticks;
		}
		if (Mouse::getEventButton() == MouseAction::ACTION_RIGHT && Mouse::getEventButtonState()) {
		handleMouseClick(MouseAction::ACTION_RIGHT);
		lastClickTick = ticks;
		}
		}
		*/
	}

	TIMER_POP_PUSH("keyboard");
	while (Keyboard::next()) {
		int key = Keyboard::getEventKey();
		bool isPressed = (Keyboard::getEventKeyState() == KeyboardAction::KEYDOWN);
		player->setKey(key, isPressed);

		if (key == options.getIntValue(OPTIONS_KEY_SPRINT)) {
			player->setSprinting(isPressed);
		}

		if (isPressed) {
			gui.handleKeyPressed(key);

			if (key >= '0' && key <= '9') {
				int digit = key - '0';
				int slot = digit - 1;

				if (slot >= 0 && slot < gui.getNumSlots())
					player->inventory->selectSlot(slot);

#if defined(WIN32)
				if (digit >= 1 && GetAsyncKeyState(VK_CONTROL) < 0) {
					// Set adventure settings here!
					AdventureSettingsPacket p(level->adventureSettings);
					p.toggle((AdventureSettingsPacket::Flags)(1 << slot));
					p.fillIn(level->adventureSettings);
					raknetInstance->send(p);
				}
				if (digit == 0) {
					Pos pos((int)player->x, (int)player->y-1, (int)player->z);
					SetSpawnPositionPacket p(pos);
					raknetInstance->send(p);
				}
#endif
			}

			if (key == Keyboard::KEY_E) {
				// Survival gets the classic Beta-style inventory,
				// creative keeps the block selection screen. On desktop
				// the touch flow is never used, whatever the option says.
#if defined(PLATFORM_DESKTOP)
				if (!isCreativeMode())
					setScreen(new BetaInventoryScreen());
				else
					screenChooser.setScreen(SCREEN_BLOCKSELECTION);
#else
				if (!isCreativeMode() && !useTouchscreen())
					setScreen(new BetaInventoryScreen());
				else
					screenChooser.setScreen(SCREEN_BLOCKSELECTION);
#endif
			}

			if (!screen && key == Keyboard::KEY_T && level) {
				setScreen(new ConsoleScreen());
			}

			if (key == Keyboard::KEY_F2) {
				takeScreenshot();
			}

			if (key == Keyboard::KEY_F7) {
				takeIsometricScreenshot();
			}

			if (key == Keyboard::KEY_F3) {
				options.toggle(OPTIONS_RENDER_DEBUG);
			}

			if (key == Keyboard::KEY_F5) {
				options.toggle(OPTIONS_THIRD_PERSON_VIEW);
				/*
				ImprovedNoise noise;
				for (int i = 0; i < 16; ++i)
				printf("%d\t%f\n", i, noise.grad2(i, 3, 8));
				*/
			}

			if (!screen && key == Keyboard::KEY_O || key == 250) {
				releaseMouse();
			}

			if (key == Keyboard::KEY_F) {
				int dst = options.getIntValue(OPTIONS_VIEW_DISTANCE);
				options.set(OPTIONS_VIEW_DISTANCE, (dst + 1) % 4);
			}

#ifdef CHEATS
			if (key == Keyboard::KEY_U) {
				onGraphicsReset();
				player->heal(100);
			}

			if (key == Keyboard::KEY_B || key == 108) // Toggle the game mode
				setIsCreativeMode(!isCreativeMode());

			if (key == Keyboard::KEY_P) // Step forward in time
				level->setTime( level->getTime() + 1000);

			if (key == Keyboard::KEY_G) {
				setScreen(new ArmorScreen());
				/*
				std::vector<AABB>& boxs = level->getCubes(NULL, AABB(128.1f, 73, 128.1f, 128.9f, 74.9f, 128.9f));
				LOGI("boxes: %d\n", (int)boxs.size());
				*/
			}

			if (key == Keyboard::KEY_Y) {
				textures->reloadAll();
				player->hurtTo(2);
			}
			if (key == Keyboard::KEY_Z || key == 108) {
				for (int i = 0; i < 1; ++i) {
					Mob* mob = NULL;
					int forceId = 0;//MobTypes::Sheep;

					int types[] = {
						MobTypes::Sheep,
						MobTypes::Pig,
						MobTypes::Chicken,
						MobTypes::Cow,
					};

					int mobType = (forceId > 0)? forceId : types[Mth::random(sizeof(types) / sizeof(int))];
					mob = MobFactory::CreateMob(mobType, level);

					//((Animal*)mob)->setAge(-1000);
					float dx = 4 - 8 * Mth::random() + 4 * Mth::sin(Mth::DEGRAD * player->yRot);
					float dz = 4 - 8 * Mth::random() + 4 * Mth::cos(Mth::DEGRAD * player->yRot);
					if (mob && !MobSpawner::addMob(level, mob, player->x + dx, player->y, player->z + dz, Mth::random()*360, 0, true))
						delete mob;
				}
			}

			if (key == Keyboard::KEY_X) {
				const EntityList& entities = level->getAllEntities();
				for (int i = entities.size()-1; i >= 0; --i) {
					Entity* e = entities[i];
					if (!e->isPlayer())
						level->removeEntity(e);
				}
			}

			if (key == Keyboard::KEY_C /*|| key == 4*/) {
				player->inventory->clearInventoryWithDefault();
				// @todo: Add saving here for benchmarking
			}
			if (key == Keyboard::KEY_H) {
				setScreen( new PrerenderTilesScreen() );
			}

			if (key == Keyboard::KEY_O) {
				for (int i = Inventory::MAX_SELECTION_SIZE; i < player->inventory->getContainerSize(); ++i)
					if (player->inventory->getItem(i))
						player->inventory->dropSlot(i, false);
			}
			if (key == Keyboard::KEY_M) {
				Difficulty difficulty = (Difficulty)options.getIntValue(OPTIONS_DIFFICULTY);
				options.set(OPTIONS_DIFFICULTY, (difficulty == Difficulty::PEACEFUL)?
					Difficulty::NORMAL : Difficulty::PEACEFUL);
				//setIsCreativeMode( !isCreativeMode() );
			}

			if (options.getBooleanValue(OPTIONS_RENDER_DEBUG)) {
				if (key >= '0' && key <= '9') {
					_perfRenderer->debugFpsMeterKeyPress(key - '0');
				}
			}
#endif

			if (options.getBooleanValue(OPTIONS_RENDER_DEBUG)) {
				if (key >= '0' && key <= '9') {
					_perfRenderer->debugFpsMeterKeyPress(key - '0');
				}
			}

			if (key == Keyboard::KEY_ESCAPE)
				pauseGame(false);

#ifdef PLATFORM_DESKTOP
			if (key == Keyboard::KEY_P) {
				static bool isWireFrame = false;
				isWireFrame = !isWireFrame;
				glPolygonMode(GL_FRONT, isWireFrame? GL_LINE : GL_FILL);
				//glPolygonMode(GL_BACK, isWireFrame? GL_LINE : GL_FILL);
			}
#endif
		}
#ifdef WIN32
		if (key == Keyboard::KEY_M) {
			for (int i = 0; i < 5 * SharedConstants::TicksPerSecond; ++i)
				level->tick();
		}

#endif
	}

	TIMER_POP_PUSH("handlemouse");

	static bool prevMouseDownLeft = false;

	if (!useTouchscreen()) {
		if (Mouse::getButtonState(MouseAction::ACTION_LEFT) == 0) {
			gameMode->stopDestroyBlock();
		}

		if (!Mouse::isButtonDown(MouseAction::ACTION_RIGHT)) {
			gameMode->releaseUsingItem(player);
		}
	}

	if (useTouchscreen()) {
		// Touch: gesture recognizer classifies the action type (turn/destroy/build)
		BuildActionIntention bai;

		if (inputHolder && inputHolder->getBuildInput()->tickBuild(player, &bai)) {
			handleBuildAction(&bai);
		} else {
			gameMode->stopDestroyBlock();
			if (player && player->isUsingItem()) {
                gameMode->releaseUsingItem(player);
            }
		}
	} else {
		// Desktop: left mouse = destroy/attack
		if (Mouse::isButtonDown(MouseAction::ACTION_LEFT)) {
			auto baiFlags = BuildActionIntention::BAI_REMOVE | BuildActionIntention::BAI_ATTACK;

			if (!prevMouseDownLeft) baiFlags |= BuildActionIntention::BAI_FIRSTREMOVE;

			BuildActionIntention bai(baiFlags);
			handleBuildAction(&bai);
		}

		prevMouseDownLeft = Mouse::isButtonDown(MouseAction::ACTION_LEFT);

		// Build and use/interact is on same button
		// USPESHNO spizheno
		static int buildHoldTicks = 0;
		if (Mouse::isButtonDown(MouseAction::ACTION_RIGHT)) {
			if (buildHoldTicks >= 5) buildHoldTicks = 0;

			if (++buildHoldTicks == 1) {
				BuildActionIntention bai(BuildActionIntention::BAI_BUILD | BuildActionIntention::BAI_INTERACT);
				handleBuildAction(&bai);
			}
		} else {
			buildHoldTicks = 0;
			gameMode->releaseUsingItem(player);
		}
	}

	lastTickTime = getTimeMs();

	// we have (hopefully) handled the keyboard & mouse queue and it
	// can now be emptied. If wanted, the reset could be changed to:
	// index -= numRead; // then this code doesn't have to be placed here
	// + it prepares for tick not handling all or any events.
	// update: RAII'ing instead, see above
	//Keyboard::reset();
	//Mouse::reset();

	TIMER_POP();
#endif
}

void Minecraft::handleBuildAction(BuildActionIntention* action) {
#ifndef STANDALONE_SERVER
	if (action->isRemove()) {
		if (missTime > 0) return;
		player->swing();
	}
	if(player->isUsingItem())
		return;
	bool mayUse = true;

	if (!hitResult.isHit()) {
		if (action->isRemove() && !gameMode->isCreativeType()) {
			missTime = 10;
		}
	} else if (hitResult.type == ENTITY) {
		if (action->isAttack()) {
			player->swing();
			//LOGI("attacking!\n");
			InteractPacket packet(InteractPacket::Attack, player->entityId, hitResult.entity->entityId);
			raknetInstance->send(packet);
			gameMode->attack(player, hitResult.entity);
		} else if (action->isInteract()) {
			if (hitResult.entity->interactPreventDefault())
				mayUse = false;
			//LOGI("interacting!\n");
			InteractPacket packet(InteractPacket::Interact, player->entityId, hitResult.entity->entityId);
			raknetInstance->send(packet);
			gameMode->interact(player, hitResult.entity);
		}
	} else if (hitResult.type == TILE) {
		int x = hitResult.x;
		int y = hitResult.y;
		int z = hitResult.z;
		int face = hitResult.f;

		int oldTileId = level->getTile(x, y, z);
		Tile* oldTile = Tile::tiles[oldTileId];

		//bool tryDestroyBlock = false;

		if (action->isRemove()) {
			if (!oldTile)
				return;

			//LOGI("tile: %s - %d, %d, %d. b: %f - %f\n", oldTile->getDescriptionId().c_str(), x, y, z, oldTile->getBrightness(level, x, y, z), oldTile->getBrightness(level, x, y+1, z));
			level->extinguishFire(x, y, z, hitResult.f);

			if (action->isFirstRemove()) {
				gameMode->startDestroyBlock(x, y, z, hitResult.f);
			} else {
				gameMode->continueDestroyBlock(x, y, z, hitResult.f);
			}

			particleEngine->crack(x, y, z, hitResult.f);
			player->swing();
		}
		else {
			ItemInstance* item = player->inventory->getSelected();
			if (gameMode->useItemOn(player, level, item, x, y, z, face, hitResult.pos)) {
				mayUse = false;
				player->swing();
#ifdef RPI
			} else if (item && item->id == ((Item*)Item::sword_iron)->id) {
				player->swing();
#endif
			}
			if (item && item->count <= 0) {
				player->inventory->clearSlot(player->inventory->selected);
			}
			//} else if (item && item->count != oldCount) {
			//	  gameRenderer->itemInHandRenderer->itemPlaced();
			//}
		}
	}
	if (mayUse && action->isInteract()) {
		ItemInstance* item = player->inventory->getSelected();
		if (item && !player->isUsingItem()) {
			if (gameMode->useItem(player, level, item)) {
				gameRenderer->itemInHandRenderer->itemUsed();
			}
			if (item && item->count <= 0) {
				player->inventory->clearSlot(player->inventory->selected);
			}
		}
	}
#endif
}

bool Minecraft::isOnlineClient()
{
	return (level != NULL && level->isClientSide);
}

bool Minecraft::isOnline()
{
	return netCallback != NULL;
}

void Minecraft::pauseGame(bool isBackPaused) {
	// Only freeze gameplay when running a local server and it is not accepting
	// incoming connections (invisible server), which includes typical single-
	// player/lobby mode. If the server is visible, the game should keep ticking.
	bool canFreeze = false;
	if (raknetInstance && raknetInstance->isServer() && netCallback) {
		ServerSideNetworkHandler* ss = (ServerSideNetworkHandler*) netCallback;
		if (!ss->allowsIncomingConnections())
			canFreeze = true;
	}
	pause = canFreeze;

#ifndef STANDALONE_SERVER
	if (screen != NULL) return;
	screenChooser.setScreen(isBackPaused? SCREEN_PAUSEPREV : SCREEN_PAUSE);
#endif
}
void Minecraft::gameLostFocus() {
#ifndef STANDALONE_SERVER
	if(screen != NULL) {
		screen->lostFocus();
	}
#endif
}


void Minecraft::setScreen( Screen* screen )
{
#ifndef	STANDALONE_SERVER
	Mouse::reset();
	Multitouch::reset();
	Multitouch::resetThisUpdate();
	if (player) {
		player->releaseAllKeys();
	}

	if (screenMutex) {
		hasScheduledScreen = true;
		scheduledScreen = screen;
		return;
	}

	if (screen != NULL && screen->isErrorScreen())
		return;
	if (screen == NULL && level == NULL)
		screen = screenChooser.createScreen(SCREEN_STARTMENU);

	if (this->screen != NULL) {
		this->screen->removed();
		delete this->screen;
	}

	this->screen = screen;
	if (screen != NULL) {
		releaseMouse();
		//ScreenSizeCalculator ssc = new ScreenSizeCalculator(options, width, height);
		int screenWidth = (int)(width * Gui::InvGuiScale); //ssc.getWidth();
		int screenHeight = (int)(height * Gui::InvGuiScale); //ssc.getHeight();
		screen->init(this, screenWidth, screenHeight);

		if (screen->isInGameScreen() && level) {
			level->saveLevelData();
			level->saveGame();
		}

		//noRender = false;
	} else {
		// Closing a screen and returning to the game should unpause.
		pause = false;
		// Full grab cycle, not just grab: if the cursor mode desynced
		// (grabbed platform but no motion events, e.g. Wayland lock
		// negotiated only once), a fresh release+grab re-syncs it.
		// This is the same thing as opening/closing a menu by hand.
		releaseMouse();
		grabMouse();
	}
#endif
}

void Minecraft::grabMouse()
{
#ifndef STANDALONE_SERVER
	// Always sync the platform grab: the flag alone can desync from the
	// real cursor mode (missed call, another thread without GL context,
	// or a Wayland compositor that only honors a fresh grab).
	mouseHandler.grab();
	if (mouseGrabbed) return;
	mouseGrabbed = true;
	//setScreen(NULL);
#endif
}

void Minecraft::releaseMouse()
{
#ifndef STANDALONE_SERVER
	// Always sync the platform release for the same reason.
	mouseHandler.release();
	if (!mouseGrabbed) {
		return;
	}
	if (player) {
		player->releaseAllKeys();
	}
	mouseGrabbed = false;
#endif
}

bool Minecraft::useTouchscreen() {
#if defined(TARGET_OS_IPHONE)
	return true;
#elif defined(RPI)
	return false;
#endif
	if (platform()->supportsTouchscreen()) {
		return options.getBooleanValue(OPTIONS_USE_TOUCHSCREEN);
	}
	return options.getBooleanValue(OPTIONS_USE_TOUCHSCREEN) && !_supportsNonTouchscreen;
}
bool Minecraft::supportNonTouchScreen() {
	return _supportsNonTouchscreen;
}
void Minecraft::init()
{
#ifndef STANDALONE_SERVER
	checkGlError("Init enter");

	_supportsNonTouchscreen = !platform()->supportsTouchscreen();

	LOGI("IS TOUCHSCREEN? %d\n", options.getBooleanValue(OPTIONS_USE_TOUCHSCREEN));

	textures = new Textures(&options, platform());
	textures->addDynamicTexture(new WaterTexture());
	textures->addDynamicTexture(new WaterSideTexture());
	textures->addDynamicTexture(new LavaTexture());
	textures->addDynamicTexture(new LavaSideTexture());
	textures->addDynamicTexture(new FireTexture(0));
	textures->addDynamicTexture(new FireTexture(1));
	gui.texturesLoaded(textures);

	levelRenderer = new LevelRenderer(this);
	gameRenderer = new GameRenderer(this);
	particleEngine = new ParticleEngine(level, textures);

	// 4j's code for reference
	//	FoliageColor::init(textures->loadTexturePixels(L"misc/foliagecolor.png"));


	// my code
	TextureId foliageId = (textures->loadTexture("misc/foliagecolor.png")); // loading the uh png for foliage color
	int* foliagePixels = textures->loadTexturePixels(foliageId, "misc/foliagecolor.png");
	// now i can finally initialize foliage color, probably not the best way to handle this but i cant be arsed rn
	FoliageColor::init(foliagePixels);

	TextureId grassId = (textures->loadTexture("misc/grasscolor.png")); // loading the uh png for foliage color
	int* grassPixels = textures->loadTexturePixels(grassId, "misc/grasscolor.png");
	GrassColor::init(grassPixels);

	bool tint = options.getBooleanValue(OPTIONS_FOLIAGE_TINT); // finally, toggleable foliage color
	FoliageColor::setUseTint(tint);
	GrassColor::setUseTint(tint);

	bool sideTint = options.getBooleanValue(OPTIONS_TINTED_SIDE);
	TileRenderer::setUseTint(sideTint);


	// Platform specific initialization here
	font = new Font(&options, "font/default8.png", textures);

	_perfRenderer = new PerfRenderer(this, font);

	checkGlError("Init complete");
#endif

	options.load();

	setIsCreativeMode(false); // false means it's Survival Mode
	reloadOptions();
}

void Minecraft::setSize(int w, int h) {
#ifndef STANDALONE_SERVER
	transformResolution(&w, &h);

	width  = w;
	height = h;
	int screenWidth;
	int screenHeight;
	//#ifdef PLATFORM_DESKTOP
	int guiScaleOpt = options.getIntValue(OPTIONS_GUI_SCALE);
	int scale = 1;
	while (scale < 1000 && width / (scale + 1) >= 320 && height / (scale + 1) >= 240) {
		scale++;
	}
	if (guiScaleOpt > 0 && scale > guiScaleOpt) {
		scale = guiScaleOpt;
	}
	Gui::GuiScale = (float)scale;
	Gui::InvGuiScale = 1.0f / Gui::GuiScale;
	screenWidth  = (int)(width  * Gui::InvGuiScale);
	screenHeight = (int)(height * Gui::InvGuiScale);

	Config config = createConfig(this);
	gui.onConfigChanged(config);

	if (screen)
		screen->setSize(screenWidth, screenHeight);

	if (inputHolder)
		inputHolder->onConfigChanged(config);
	//LOGI("Setting size: %d, %d: %f\n", width, height, Gui::InvGuiScale);

#ifdef WIN32
	char resbuf[128];
	sprintf(resbuf, "            %d x %d @ scale %.2f", width, height, Gui::GuiScale);
	//gui.addMessage(resbuf);
#endif
#endif /* STANDALONE_SERVER */
}

void Minecraft::reloadOptions() {
	options.save();
	bool wasTouchscreen = options.getBooleanValue(OPTIONS_USE_TOUCHSCREEN);
	options.set(OPTIONS_USE_TOUCHSCREEN, useTouchscreen());
	options.save();

	if ((wasTouchscreen != useTouchscreen()) || (inputHolder == 0))
		_reloadInput();

	// user->name = options.username;

	LOGI("Reloading-options\n");

	// @todo @fix Android and iOS behaves a bit differently when leaving
	//            an options screen (Android recreates OpenGL surface)
	setSize(width, height);
}

void Minecraft::_reloadInput() {
#ifndef STANDALONE_SERVER
	delete inputHolder;

	const bool useTouchHolder = useTouchscreen();
	if (useTouchHolder) {
		inputHolder = new TouchInputHolder(this, &options);
	} else {
#if defined(ANDROID) || defined(__APPLE__) 
		inputHolder = new CustomInputHolder(
			new XperiaPlayInput(&options),
			new ControllerTurnInput(2, ControllerTurnInput::MODE_DELTA),
			new IBuildInput());
#else
		inputHolder = new CustomInputHolder(
			new KeyboardInput(&options),
			new MouseTurnInput(MouseTurnInput::MODE_DELTA, width/2, height/2),
			new MouseBuildInput());
#endif
	}

	mouseHandler.setTurnInput(inputHolder->getTurnInput());
	if (level && player) {
		player->input = inputHolder->getMoveInput();
	}
#endif
}


//
// Multiplayer
//
void Minecraft::locateMultiplayer() {
	isLookingForMultiplayer = true;

	raknetInstance->pingForHosts(19132);
	netCallback = new ClientSideNetworkHandler(this, raknetInstance);
}

void Minecraft::cancelLocateMultiplayer() {
	isLookingForMultiplayer = false;

	raknetInstance->stopPingForHosts();

	delete netCallback;
	netCallback = NULL;
}

bool Minecraft::joinMultiplayer( const PingedCompatibleServer& server )
{
	if (isLookingForMultiplayer && netCallback) {
		isLookingForMultiplayer = false;
		return raknetInstance->connect(server.address.ToString(false), server.address.GetPort());
	}
	return false;
}

bool Minecraft::joinMultiplayerFromString( const std::string& server )
{	
	std::string ip = "";
	std::string port = "19132";

	size_t pos = server.find(":");

	if (pos != std::string::npos) {
		ip = server.substr(0, pos);
		port = server.substr(pos + 1);
	} else {
		ip = server;
	}

	printf("%s \n", port.c_str());

	if (isLookingForMultiplayer && netCallback) {
		isLookingForMultiplayer = false;
		printf("test");
		int portNum = atoi(port.c_str());
		return raknetInstance->connect(ip.c_str(), portNum);
	}
	return false;
}

void Minecraft::hostMultiplayer(int port) {
	// Tear down last instance
	raknetInstance->disconnect();
	delete netCallback;
	netCallback = NULL;

#if !defined(NO_NETWORK)
	netCallback = new ServerSideNetworkHandler(this, raknetInstance);
#ifdef STANDALONE_SERVER
	raknetInstance->host("Server", port, 16);
#else
	raknetInstance->host(options.getStringValue(OPTIONS_USERNAME), port);
#endif
#endif
}

//
// Level generation
//
/*static*/

void* Minecraft::prepareLevel_tspawn(void *p_param)
{
	Minecraft* mc = (Minecraft*) p_param;
	mc->generateLevel("Currently not used", mc->level);
	return 0;
}

void Minecraft::generateLevel( const std::string& message, Level* level )
{
	Stopwatch s;
	s.start();
	prepareLevel(message);
	s.stop();
	s.print("Level generated: ");

	// "Unlock"
	isGeneratingLevel = false;
}

void Minecraft::_levelGenerated()
{
#ifndef STANDALONE_SERVER
	if (player == NULL) {
		player = (LocalPlayer*) gameMode->createPlayer(level);
		gameMode->initPlayer(player);
	}

	if (player) {
		player->input = inputHolder->getMoveInput();
	}

	if (levelRenderer != NULL) levelRenderer->setLevel(level);
	if (particleEngine != NULL) particleEngine->setLevel(level);

	gameMode->adjustPlayer(player);
	gui.onLevelGenerated();
#endif

	level->validateSpawn();
	level->loadPlayer(player, true);
	// if we are client side, we trust the server to have given us a correct position
	if (player && !level->isClientSide) {
		player->resetPos(false);
	}

	if (level && level->dimension) {

		level->dimension->FogType = options.getIntValue(OPTIONS_FOG_TYPE);
	}


	this->cameraTargetPlayer = player;

	std::string serverName = options.getStringValue(OPTIONS_USERNAME) + " - " + level->getLevelData()->levelName;

	if (raknetInstance->isServer())
		raknetInstance->announceServer(serverName);

	if (netCallback) {
		netCallback->levelGenerated(level);
	}

#if defined(WIN32) || defined(RPI)
	if (_commandServer) {
		delete _commandServer;
	}
	_commandServer = new CommandServer(this);
	_commandServer->init(commandPort);
#endif

	// Hack to (hopefully) get the players to show (note: in LevelListener
	// instead, since adding yourself always generates a entityAdded)
	//EntityRenderDispatcher::getInstance()->onGraphicsReset();
	_hasSignaledGeneratingLevelFinished = true;
}

std::string Minecraft::gatherStats1() {
#ifndef STANDALONE_SERVER
	return levelRenderer->gatherStats1();
#endif
	return "Blank";
}

std::string Minecraft::gatherStats2() {
#ifndef STANDALONE_SERVER
	return levelRenderer->gatherStats2();
#endif
	return "Blank";
}

std::string Minecraft::gatherStats3() {
#ifndef STANDALONE_SERVER
	return ("P: " + particleEngine->countParticles() + ". T: " + (level->gatherStats()));
#endif
	return "Blank";
}

std::string Minecraft::gatherStats4() {
	return level->gatherChunkSourceStats();
}




Player* Minecraft::respawnPlayer(int playerId) {
	for (unsigned int i = 0; i < level->players.size(); ++i) {
		if (level->players[i]->entityId == playerId) {
			resetPlayer(level->players[i]);
			return level->players[i];
		}
	}
	return NULL;
}

void Minecraft::resetPlayer(Player* player) {
	level->validateSpawn();
	player->reset();

	Pos p;
	if(player->hasRespawnPosition()) {
		p = player->getRespawnPosition();
	}
	else {
		p = level->getSharedSpawnPos();
	}
	player->setPos((float)p.x + 0.5f, (float)p.y + 1.0f, (float)p.z + 0.5f);
	player->resetPos(true);

	if (isCreativeMode())
		player->inventory->clearInventoryWithDefault();
}

void Minecraft::respawnPlayer() {
	// RESET THE FRACKING PLAYER HERE
	//bool slowCheck = false;
	//for (int i = 0; i < level->entities.size(); ++i)
	//	if (level->entities[i] == player) slowCheck = true;
	//
	//LOGI("Has entity? %d, %d\n", level->getEntity(player->entityId), slowCheck);

	resetPlayer(player);

	// tell server (or other client) that we re-spawned
	RespawnPacket packet(player);
	raknetInstance->send(packet);
}

void Minecraft::onGraphicsReset()
{
#ifndef STANDALONE_SERVER
	textures->clear();

	font->onGraphicsReset();
	gui.onGraphicsReset();

	if (levelRenderer) levelRenderer->onGraphicsReset();
	if (gameRenderer) gameRenderer->onGraphicsReset();

	EntityRenderDispatcher::getInstance()->onGraphicsReset();
	TileEntityRenderDispatcher::getInstance()->onGraphicsReset();
#endif
}

int Minecraft::getProgressStatusId() {
	return progressStageStatusId;
}

const char* Minecraft::getProgressMessage()
{
	return progressMessages[progressStageStatusId];
}

bool Minecraft::isLevelGenerated()
{
	return level != NULL && !isGeneratingLevel;
}

LevelStorageSource* Minecraft::getLevelSource()
{
	return storageSource;
}

// int Minecraft::getLicenseId() {
// 	if (!LicenseCodes::isReady(_licenseId))
// 		_licenseId = platform()->checkLicense();
// 	return _licenseId;
// }

void Minecraft::audioEngineOn() {
#ifndef STANDALONE_SERVER
	soundEngine->enable(true);
#endif
}
void Minecraft::audioEngineOff() {
#ifndef STANDALONE_SERVER
	soundEngine->enable(false);
#endif
}

void Minecraft::setIsCreativeMode(bool isCreative)
{
#ifdef CREATORMODE
	delete gameMode;
	gameMode = new CreatorMode(this);
	_isCreativeMode = true;
#else
	if (!gameMode || isCreative != _isCreativeMode)
	{
		delete gameMode;
		if (isCreative) gameMode = new CreativeMode(this);
		else			gameMode = new SurvivalMode(this);
		_isCreativeMode = isCreative;
	}
#endif
	if (player)
		gameMode->initAbilities(player->abilities);
}

bool Minecraft::isCreativeMode() {
	return _isCreativeMode;
}

bool Minecraft::isKindleFire(int kindleVersion) {
	if (kindleVersion != 1)
		return false;

	std::string model = platform()->getPlatformStringVar(PlatformStringVars::DEVICE_BUILD_MODEL);
	std::string modelLower(model);
	std::transform(modelLower.begin(), modelLower.end(), modelLower.begin(), tolower);

	return (modelLower.find("kindle") != std::string::npos) && (modelLower.find("fire") != std::string::npos);
}

bool Minecraft::transformResolution(int* w, int* h)
{
	bool changed = false;

	// Kindle Fire 1: reporting wrong height in
	// certain cases (e.g. after screen lock)
	if (isKindleFire(1) && *h >= 560 && *h <= 620) {
		*h = 580;
		changed = true;
	}

	return changed;
}

ICreator* Minecraft::getCreator()
{
#ifdef CREATORMODE
	return ((CreatorMode*)gameMode)->getCreator();
#else
	return NULL;
#endif
}

void Minecraft::optionUpdated(OptionId option, bool value ) {
	if(netCallback != NULL && option == OPTIONS_SERVER_VISIBLE) {
		ServerSideNetworkHandler* ss = (ServerSideNetworkHandler*) netCallback;
		ss->allowIncomingConnections(value);
	} else if (option == OPTIONS_USE_TOUCHSCREEN) {
		_reloadInput();
	}
}

void Minecraft::optionUpdated(OptionId option, float value ) {
	// #ifndef STANDALONE_SERVER
	// 	if(option == OPTIONS_PIXELS_PER_MILLIMETER) {
	// 		pixelCalcUi.setPixelsPerMillimeter(value * Gui::InvGuiScale);
	// 		pixelCalc.setPixelsPerMillimeter(value);
	// 	}
	// #endif
}

void Minecraft::optionUpdated(OptionId option, int value ) {
	if(option == OPTIONS_GUI_SCALE) {
		// reapply screen scaling using current window size
		setSize(width, height);
	}
	else if (option == OPTIONS_LOG_LEVEL) {
		g_mcpeLogLevel = value;
	}
}

void Minecraft::takeScreenshot() {
#ifndef STANDALONE_SERVER
	if (!level) return;

	std::vector<unsigned char> pixels(width * height * 4);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

#ifdef _WIN32
	_mkdir("screenshots");
#else
	mkdir("screenshots", 0755);
#endif

	time_t rawtime;
	time(&rawtime);
	struct tm* timeinfo = localtime(&rawtime);
	char timeStr[64];
	strftime(timeStr, sizeof(timeStr), "%Y-%m-%d_%H.%M.%S", timeinfo);

	std::string filename = "screenshots/" + std::string(timeStr) + ".png";
	bool saved = savePngToFile(filename, width, height, pixels.data(), true);

	if (saved) {
		gui.addMessage("Saved screenshot as " + filename);
	} else {
		gui.addMessage("Failed to save screenshot!");
	}
#endif
}

void Minecraft::takeIsometricScreenshot() {
#ifndef STANDALONE_SERVER
	if (!level || !player) return;

	int sw = width;
	int sh = height;
	double targetX = player->x;
	double targetY = player->y;
	double targetZ = player->z;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix2();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix2();

	glViewport(0, 0, sw, sh);
	glClearColor(0.5f, 0.7f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Setup Orthographic Projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity2();
	float aspect = (float)sw / (float)sh;
	float orthoHeight = 48.0f; // Shows ~48 blocks vertically
	float orthoWidth = orthoHeight * aspect;
	glOrthof(-orthoWidth, orthoWidth, -orthoHeight, orthoHeight, -1000.0f, 1000.0f);

	// Setup Isometric Camera (classic Indev angles: pitch 30 deg, yaw 45 deg)
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity2();
	glRotatef2(30.0f, 1.0f, 0.0f, 0.0f);
	glRotatef2(45.0f, 0.0f, 1.0f, 0.0f);
	glTranslatef2((float)-targetX, (float)-targetY, (float)-targetZ);

	glEnable2(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable2(GL_CULL_FACE);
	glEnable2(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1f);
	glDisable2(GL_FOG);

	textures->loadAndBindTexture("terrain.png");
	Lighting::turnOff();

	AllowAllCuller culler;
	levelRenderer->cull(&culler, 0.0f);

	// Pass 0: Opaque blocks
	levelRenderer->render((Mob*)player, 0, 0.0f);

	// Pass 1: Cutout blocks (foliage, saplings, flowers)
	glEnable2(GL_ALPHA_TEST);
	levelRenderer->render((Mob*)player, 1, 0.0f);

	// Render Entities (mobs, items, player)
	Lighting::turnOn(this);
	levelRenderer->renderEntities(player->getPos(0.0f), &culler, 0.0f);
	Lighting::turnOff();

	// Pass 2: Translucent blocks (water, ice)
	glEnable2(GL_BLEND);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	textures->loadAndBindTexture("terrain.png");
	levelRenderer->render((Mob*)player, 2, 0.0f);
	glDisable2(GL_BLEND);

	// Read pixels from framebuffer
	std::vector<unsigned char> pixels(sw * sh * 4);
	glReadPixels(0, 0, sw, sh, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

	// Restore GL state
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix2();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix2();

#ifdef _WIN32
	_mkdir("screenshots");
#else
	mkdir("screenshots", 0755);
#endif

	time_t rawtime;
	time(&rawtime);
	struct tm* timeinfo = localtime(&rawtime);
	char timeStr[64];
	strftime(timeStr, sizeof(timeStr), "%Y-%m-%d_%H.%M.%S", timeinfo);

	std::string filename = "screenshots/mc_map_" + std::string(timeStr) + ".png";
	bool saved = savePngToFile(filename, sw, sh, pixels.data(), true);
	savePngToFile("screenshots/mc_map.png", sw, sh, pixels.data(), true);

	if (saved) {
		gui.addMessage("Saved screenshot as " + filename);
	} else {
		gui.addMessage("Failed to save screenshot!");
	}
#endif
}
