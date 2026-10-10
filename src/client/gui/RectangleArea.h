#ifndef NET_MINECRAFT_CLIENT_GUI__RectangleArea_H__
#define NET_MINECRAFT_CLIENT_GUI__RectangleArea_H__

class IArea {
public:
	IArea() : deleteMe(true) {}
	virtual ~IArea() {}
	virtual bool isInside(float x, float y) = 0;
	bool deleteMe;
};

class RectangleArea : public IArea {
public:
	RectangleArea(float x0, float y0, float x1, float y1)
	:	_x0(x0), _x1(x1), _y0(y0), _y1(y1) {}
	RectangleArea() : _x0(0), _x1(0), _y0(0), _y1(0) {}

	virtual bool isInside(float x, float y) {
		return x >= _x0 && x <= _x1 && y >= _y0 && y <= _y1;
	}
	virtual float centerX() {
		return _x0 + (_x1 - _x0) * 0.5f;
	}
	virtual float centerY() {
		return _y0 + (_y1 - _y0) * 0.5f;
	}

	float _x0, _x1;
	float _y0, _y1;
};

#endif
