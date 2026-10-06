#include "MouseHandler.h"
#include "player/input/ITurnInput.h"
#include "../AppPlatform.h"

MouseHandler::MouseHandler( ITurnInput* turnInput )
:	xd(0.0f),
	yd(0.0f),
	toSkip(0),
	_turnInput(turnInput),
	_platform(NULL)
{}

MouseHandler::MouseHandler()
:	xd(0.0f),
	yd(0.0f),
	toSkip(0),
	_turnInput(0),
	_platform(NULL)
{}

MouseHandler::~MouseHandler() {
}

void MouseHandler::setTurnInput( ITurnInput* turnInput ) {
	_turnInput = turnInput;
}

void MouseHandler::setPlatform( AppPlatform* platform ) {
	_platform = platform;
}

void MouseHandler::grab() {
	xd = 0;
	yd = 0;
	if (_platform) {
		_platform->setMouseGrabbed(true);
	}
}

void MouseHandler::release() {
	if (_platform) {
		_platform->setMouseGrabbed(false);
	}
}

void MouseHandler::poll() {
	if (_turnInput != 0) {
		TurnDelta td = _turnInput->getTurnDelta();
		xd = td.x;
		yd = td.y;
	}
}
