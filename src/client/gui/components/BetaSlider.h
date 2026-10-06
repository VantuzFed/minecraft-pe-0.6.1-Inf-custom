#ifndef NET_MINECRAFT_CLIENT_GUI_COMPONENTS__BetaSlider_H__
#define NET_MINECRAFT_CLIENT_GUI_COMPONENTS__BetaSlider_H__

#include "Button.h"
#include "../../Options.h"

class BetaSlider : public Button {
public:
	OptionId option;
	float value;
	bool dragging;

	BetaSlider(int id, OptionId opt);

	void updateFromOption(Options& o);
	void applyToOption(Options& o, Minecraft* mc);
	void updateMessage(Options& o);
	void startDrag(int mx, Options& o, Minecraft* mc);
	void drag(int mx, Options& o, Minecraft* mc);
	void stopDrag();

	virtual void renderBg(Minecraft* mc, int xm, int ym) override;
	virtual bool clicked(Minecraft* mc, int mx, int my) override;
	virtual void released(int mx, int my) override;
};

#endif /* NET_MINECRAFT_CLIENT_GUI_COMPONENTS__BetaSlider_H__ */
