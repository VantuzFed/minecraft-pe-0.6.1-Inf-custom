#include "BetaSlider.h"
#include "../../Minecraft.h"
#include "../../renderer/Textures.h"
#include "../../../locale/I18n.h"
#include "../../../util/Mth.h"

BetaSlider::BetaSlider(int id, OptionId opt)
	: Button(id, ""), option(opt), value(0.0f), dragging(false) {
	height = 20;
}

void BetaSlider::updateFromOption(Options& o) {
	if (option == OPTIONS_FOV) {
		int fov = o.getIntValue(OPTIONS_FOV);
		value = Mth::clamp(float(fov - 70) / 40.0f, 0.0f, 1.0f);
	} else if (option == OPTIONS_SENSITIVITY) {
		value = Mth::clamp(o.getProgressValue(OPTIONS_SENSITIVITY), 0.0f, 1.0f);
	} else if (option == OPTIONS_MUSIC_VOLUME) {
		value = Mth::clamp(o.music, 0.0f, 1.0f);
	} else if (option == OPTIONS_SOUND_VOLUME) {
		value = Mth::clamp(o.sound, 0.0f, 1.0f);
	}
	updateMessage(o);
}

void BetaSlider::applyToOption(Options& o, Minecraft* mc) {
	if (option == OPTIONS_FOV) {
		int fov = (int)(70.0f + value * 40.0f + 0.5f);
		o.set(OPTIONS_FOV, fov);
	} else if (option == OPTIONS_SENSITIVITY) {
		o.set(OPTIONS_SENSITIVITY, value);
	} else if (option == OPTIONS_MUSIC_VOLUME) {
		o.set(OPTIONS_MUSIC_VOLUME, value);
		o.music = value;
	} else if (option == OPTIONS_SOUND_VOLUME) {
		o.set(OPTIONS_SOUND_VOLUME, value);
		o.sound = value;
	}
	updateMessage(o);
}

void BetaSlider::updateMessage(Options& o) {
	if (option == OPTIONS_MUSIC_VOLUME) {
		int pct = (int)(value * 100.0f + 0.5f);
		msg = I18n::get("options.music") + ": " + (pct == 0 ? I18n::get("options.off") : (std::to_string(pct) + "%"));
	} else if (option == OPTIONS_SOUND_VOLUME) {
		int pct = (int)(value * 100.0f + 0.5f);
		msg = I18n::get("options.sound") + ": " + (pct == 0 ? I18n::get("options.off") : (std::to_string(pct) + "%"));
	} else if (option == OPTIONS_SENSITIVITY) {
		int pct = (int)(value * 100.0f + 0.5f);
		if (pct == 0) msg = I18n::get("options.sensitivity") + ": " + I18n::get("options.sensitivity.min");
		else if (pct == 100) msg = I18n::get("options.sensitivity") + ": " + I18n::get("options.sensitivity.max");
		else msg = I18n::get("options.sensitivity") + ": " + std::to_string(pct) + "%";
	} else if (option == OPTIONS_FOV) {
		int fov = (int)(70.0f + value * 40.0f + 0.5f);
		if (fov == 70) msg = I18n::get("options.fov") + ": " + I18n::get("options.fov.min");
		else if (fov == 110) msg = I18n::get("options.fov") + ": " + I18n::get("options.fov.max");
		else msg = I18n::get("options.fov") + ": " + std::to_string(fov);
	}
}

void BetaSlider::startDrag(int mx, Options& o, Minecraft* mc) {
	dragging = true;
	drag(mx, o, mc);
}

void BetaSlider::drag(int mx, Options& o, Minecraft* mc) {
	if (!dragging) return;
	value = Mth::clamp(float(mx - (x + 4)) / float(width - 8), 0.0f, 1.0f);
	applyToOption(o, mc);
}

void BetaSlider::stopDrag() {
	dragging = false;
}

bool BetaSlider::clicked(Minecraft* mc, int mx, int my) {
	if (Button::clicked(mc, mx, my)) {
		startDrag(mx, mc->options, mc);
		return true;
	}
	return false;
}

void BetaSlider::released(int mx, int my) {
	Button::released(mx, my);
	stopDrag();
}

void BetaSlider::renderBg(Minecraft* mc, int xm, int ym) {
	mc->textures->loadAndBindTexture("gui/gui.png");
	glColor4f2(1, 1, 1, 1);
	// Normal button background (disabled state: row 0 at v=46)
	blit(x, y, 0, 46, width / 2, height, 0, 20);
	blit(x + width / 2, y, 200 - width / 2, 46, width / 2, height, 0, 20);

	// Slider thumb (8 pixels wide, 20 pixels high)
	int thumbX = x + (int)(value * (width - 8));
	int yImage = (hovered(mc, xm, ym) || dragging) ? 2 : 1;
	blit(thumbX, y, 0, 46 + yImage * 20, 4, 20, 0, 20);
	blit(thumbX + 4, y, 196, 46 + yImage * 20, 4, 20, 0, 20);
}
