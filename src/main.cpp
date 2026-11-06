#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>

using namespace geode::prelude;

static float rnd(float a, float b) {
	return a + (b - a) * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));
}

class RainLayer : public CCNode {
protected:
	struct Drop {
		CCSprite* spr;
		float vy;    // píxeles/seg hacia abajo
		float vx;    // deriva horizontal
		float spin;  // grados/seg (0 = no gira)
	};
	std::vector<Drop> m_drops;
	float m_spawnTimer = 0.f;

	bool init() override {
		if (!CCNode::init()) return false;
		this->scheduleUpdate();
		return true;
	}

	void spawn() {
		auto mod = Mod::get();
		auto win = CCDirector::get()->getWinSize();

		auto spr = CCSprite::create("gato.png"_spr);
		if (!spr) return;

		float size = static_cast<float>(mod->getSettingValue<double>("drop-size"));
		float targetW = win.height * size;
		float scale = targetW / spr->getContentSize().width;
		spr->setScale(scale);
		spr->setOpacity(static_cast<GLubyte>(mod->getSettingValue<int64_t>("opacity")));

		float h = spr->getContentSize().height * scale;
		spr->setPosition({ rnd(0.f, win.width), win.height + h });

		float speedMul = static_cast<float>(mod->getSettingValue<double>("fall-speed"));
		int chance = static_cast<int>(mod->getSettingValue<int64_t>("spin-chance"));

		Drop d;
		d.spr = spr;
		d.vy = rnd(120.f, 320.f) * speedMul;
		d.vx = rnd(-15.f, 15.f);
		if ((std::rand() % 100) < chance) {
			d.spin = rnd(90.f, 360.f) * ((std::rand() % 2) ? 1.f : -1.f);
		} else {
			d.spin = 0.f;
			spr->setRotation(rnd(-12.f, 12.f));
		}
		this->addChild(spr);
		m_drops.push_back(d);
	}

	void update(float dt) override {
		auto mod = Mod::get();
		bool hide = PlayLayer::get() && !mod->getSettingValue<bool>("show-in-level");
		this->setVisible(!hide);
		if (hide) return;

		if (dt > 0.1f) dt = 0.1f;
		int maxDrops = static_cast<int>(mod->getSettingValue<int64_t>("max-drops"));

		m_spawnTimer -= dt;
		if (m_spawnTimer <= 0.f && static_cast<int>(m_drops.size()) < maxDrops) {
			this->spawn();
			m_spawnTimer = rnd(0.05f, 0.25f) * (25.f / static_cast<float>(maxDrops));
		}

		for (size_t i = 0; i < m_drops.size();) {
			auto& d = m_drops[i];
			auto pos = d.spr->getPosition();
			pos.y -= d.vy * dt;
			pos.x += d.vx * dt;
			d.spr->setPosition(pos);
			if (d.spin != 0.f) d.spr->setRotation(d.spr->getRotation() + d.spin * dt);

			float h = d.spr->getContentSize().height * d.spr->getScale();
			if (pos.y < -h) {
				d.spr->removeFromParent();
				m_drops.erase(m_drops.begin() + i);
			} else {
				++i;
			}
		}
	}

public:
	static RainLayer* create() {
		auto ret = new RainLayer();
		if (ret && ret->init()) {
			ret->autorelease();
			return ret;
		}
		CC_SAFE_DELETE(ret);
		return nullptr;
	}
};

static Ref<RainLayer> g_rain = nullptr;

// Mueve la capa de lluvia a cada escena nueva (menus, niveles, editor...)
class $modify(RainDirector, CCDirector) {
	void willSwitchToScene(CCScene* scene) {
		CCDirector::willSwitchToScene(scene);
		if (!scene) return;
		if (!g_rain) g_rain = RainLayer::create();
		if (!g_rain) return;
		g_rain->removeFromParent();
		scene->addChild(g_rain, 1000000);
	}
};
