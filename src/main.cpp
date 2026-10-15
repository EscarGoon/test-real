// Inspo Generator v2 - Geode mod entry point.
// Adds a toolbar to the level editor:
//   Inspire  - generate a new batch around the camera
//   Reroll   - new shapes, same colors
//   Recolor  - same shapes, new palette
//   < > ^ v  - move the last batch (objects + its triggers together)
//   Delete   - remove the last batch
//   Gear     - open the mod settings

#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/ui/GeodeUI.hpp>

#include "gen/Generator.hpp"

#include <functional>
#include <random>

using namespace geode::prelude;

namespace {

constexpr float GROUND_Y = 90.f; // editor object layer y of the ground line

template <class T>
T setting(char const* key) { return Mod::get()->getSettingValue<T>(key); }

uint32_t randomSeed() {
    static std::random_device rd;
    return rd() & 0x7fffffff;
}

uint32_t seedFromSetting() {
    auto text = setting<std::string>("seed");
    if (text.empty()) return randomSeed();
    char* end = nullptr;
    unsigned long v = std::strtoul(text.c_str(), &end, 10);
    if (end && *end == '\0') return (uint32_t)v;
    return (uint32_t)(std::hash<std::string>{}(text) & 0x7fffffff);
}

int motionLevel() {
    auto m = setting<std::string>("motion");
    if (m == "static") return 0;
    if (m == "ambient") return 1;
    return 2;
}

int highestGroupInLevel(LevelEditorLayer* lel) {
    int maxG = 0;
    if (!lel || !lel->m_objects) return 0;
    for (auto obj : CCArrayExt<GameObject*>(lel->m_objects)) {
        if (!obj || !obj->m_groups) continue;
        for (int i = 0; i < obj->m_groupCount && i < 10; i++) maxG = std::max(maxG, (int)(*obj->m_groups)[i]);
    }
    return maxG;
}

// Writes the palette into the level's starting colors so the editor shows the
// real colors right away (color triggers only apply in playtest/preview).
// If this fails to compile against your Geode bindings, configure with
// -DINSPO_EDITOR_COLORS=OFF; everything else still works.
void applyEditorColors(LevelEditorLayer* lel, inspo::GenResult const& r, bool levelColors) {
#if INSPO_EDITOR_COLORS
    if (!lel || !lel->m_levelSettings || !lel->m_levelSettings->m_effectManager) return;
    auto em = lel->m_levelSettings->m_effectManager;
    auto set = [&](int ch, inspo::RGB c, float op, bool blend) {
        auto action = em->getColorAction(ch);
        if (!action) return;
        action->m_fromColor = ccc3(c.r, c.g, c.b);
        action->m_color = ccc3(c.r, c.g, c.b);
        action->m_fromOpacity = op;
        action->m_currentOpacity = op;
        action->m_blending = blend;
    };
    for (int i = 0; i < inspo::ROLE_COUNT; i++) {
        auto const& rc = r.palette.roles[i];
        set(r.palette.ch((inspo::Role)i), rc.c, (float)rc.opacity, rc.blend);
    }
    if (levelColors) {
        set(inspo::gd::ch::BG, r.palette.bg, 1.f, false);
        set(inspo::gd::ch::GROUND, r.palette.ground, 1.f, false);
        set(inspo::gd::ch::LINE, r.palette.line, 1.f, true);
    }
    lel->updateLevelColors();
#else
    (void)lel; (void)r; (void)levelColors;
#endif
}

} // namespace

class $modify(InspoEditorUI, EditorUI) {
    struct Fields {
        Ref<CCArray> batch;
        inspo::GenSettings last;
        bool hasLast = false;
        CCMenu* menu = nullptr;
    };

    bool init(LevelEditorLayer* lel) {
        if (!EditorUI::init(lel)) return false;
        this->buildToolbar();
        return true;
    }

    void showUI(bool show) {
        EditorUI::showUI(show);
        if (m_fields->menu) m_fields->menu->setVisible(show);
    }

    // ------------------------------------------------------------ toolbar
    CCMenuItemSpriteExtra* makeButton(char const* text, SEL_MenuHandler sel, char const* bg = "GJ_button_04.png") {
        auto spr = ButtonSprite::create(text, "bigFont.fnt", bg, .8f);
        spr->setScale(.45f);
        return CCMenuItemSpriteExtra::create(spr, this, sel);
    }

    void buildToolbar() {
        auto win = CCDirector::get()->getWinSize();
        auto menu = CCMenu::create();
        menu->setID("inspo-toolbar"_spr);

        std::vector<CCMenuItemSpriteExtra*> items = {
            makeButton("Inspire", menu_selector(InspoEditorUI::onInspire), "GJ_button_01.png"),
            makeButton("Reroll", menu_selector(InspoEditorUI::onReroll)),
            makeButton("Recolor", menu_selector(InspoEditorUI::onRecolor)),
            makeButton("<", menu_selector(InspoEditorUI::onMoveLeft)),
            makeButton(">", menu_selector(InspoEditorUI::onMoveRight)),
            makeButton("^", menu_selector(InspoEditorUI::onMoveUp)),
            makeButton("v", menu_selector(InspoEditorUI::onMoveDown)),
            makeButton("Delete", menu_selector(InspoEditorUI::onDeleteBatch), "GJ_button_06.png"),
            makeButton("...", menu_selector(InspoEditorUI::onSettings), "GJ_button_05.png"),
        };
        float total = 0, gap = 4.f;
        for (auto it : items) total += it->getScaledContentSize().width + gap;
        total -= gap;
        float x = -total / 2;
        for (auto it : items) {
            float w = it->getScaledContentSize().width;
            it->setPosition({x + w / 2, 0});
            menu->addChild(it);
            x += w + gap;
        }
        menu->setPosition({win.width / 2, win.height - 52.f});
        this->addChild(menu, 100);
        m_fields->menu = menu;
    }

    // ------------------------------------------------------------ batch helpers
    std::vector<GameObject*> liveBatch() {
        std::vector<GameObject*> out;
        if (!m_fields->batch) return out;
        for (auto obj : CCArrayExt<GameObject*>(m_fields->batch.data())) {
            if (obj && obj->getParent()) out.push_back(obj);
        }
        return out;
    }

    bool deleteBatch() {
        auto objs = liveBatch();
        if (objs.empty()) return false;
        auto arr = CCArray::create();
        for (auto o : objs) arr->addObject(o);
        this->deselectAll();
        this->selectObjects(arr, true);
        this->onDeleteSelected(nullptr);
        m_fields->batch = Ref<CCArray>();
        return true;
    }

    inspo::GenSettings settingsFromMod() {
        inspo::GenSettings s;
        s.theme = setting<std::string>("theme");
        s.harmony = setting<std::string>("harmony");
        s.layout = setting<std::string>("layout");
        s.count = (int)setting<int64_t>("structures");
        s.detail = (int)setting<int64_t>("detail");
        s.motion = motionLevel();
        s.background = setting<bool>("background");
        s.parallax = setting<bool>("parallax");
        s.entrance = setting<bool>("entrance");
        s.setLevelColors = setting<bool>("level-colors");
        s.startAtLevelBegin = setting<bool>("start-at-level-begin");
        s.colorStart = (int)setting<int64_t>("color-start");
        s.solidId = (int)setting<int64_t>("solid-id");
        s.groupStart = std::max((int)setting<int64_t>("group-start"), highestGroupInLevel(m_editorLayer) + 1);

        auto win = CCDirector::get()->getWinSize();
        auto center = m_editorLayer->m_objectLayer->convertToNodeSpace({win.width / 2, win.height / 2});
        s.centerX = center.x;
        s.centerY = center.y - GROUND_Y;
        return s;
    }

    void run(inspo::GenSettings const& s) {
        if (s.groupStart > 9500) {
            Notification::create("Out of free group IDs", NotificationIcon::Error)->show();
            return;
        }
        auto res = inspo::generate(s);
        auto objs = m_editorLayer->createObjectsFromString(res.levelString, true, true);
        if (!objs || objs->count() == 0) {
            Notification::create("Generation failed", NotificationIcon::Error)->show();
            return;
        }
        m_fields->batch = objs;
        m_fields->last = s;
        m_fields->hasLast = true;
        if (setting<bool>("editor-preview-colors")) applyEditorColors(m_editorLayer, res, s.setLevelColors);

        this->deselectAll();
        this->selectObjects(objs, true);
        this->updateButtons();

        std::string names;
        for (auto const& p : res.placed) names += (names.empty() ? "" : ", ") + p;
        Notification::create(
            fmt::format("{} | {} objs | seed {} | {}", res.palette.name, res.objects, s.seed, names),
            NotificationIcon::Success, 3.f
        )->show();
        log::info("Inspo batch: seed={} paletteSeed={} groups {}-{} objects={} triggers={}", s.seed, s.paletteSeed,
                  res.firstGroup, res.nextGroup - 1, res.objects, res.triggers);
    }

    // ------------------------------------------------------------ actions
    void onInspire(CCObject*) {
        auto s = settingsFromMod();
        s.seed = seedFromSetting();
        s.paletteSeed = s.seed;
        run(s);
    }

    void onReroll(CCObject*) {
        if (!m_fields->hasLast) return onInspire(nullptr);
        auto s = m_fields->last;
        deleteBatch();
        s.seed = randomSeed();
        run(s);
    }

    void onRecolor(CCObject*) {
        if (!m_fields->hasLast) return onInspire(nullptr);
        auto s = m_fields->last;
        deleteBatch();
        s.paletteSeed = randomSeed();
        if (s.theme != "random" && setting<bool>("recolor-keeps-theme") == false) s.theme = "random";
        run(s);
    }

    void moveBatch(float dx, float dy) {
        auto objs = liveBatch();
        if (objs.empty()) {
            Notification::create("No batch to move", NotificationIcon::Info)->show();
            return;
        }
        float step = (float)setting<int64_t>("nudge-blocks") * 30.f;
        for (auto o : objs) this->moveObject(o, {dx * step, dy * step});
        m_fields->last.centerX += dx * step;
        m_fields->last.centerY += dy * step;
        this->updateButtons();
    }

    void onMoveLeft(CCObject*)  { moveBatch(-1, 0); }
    void onMoveRight(CCObject*) { moveBatch(1, 0); }
    void onMoveUp(CCObject*)    { moveBatch(0, 1); }
    void onMoveDown(CCObject*)  { moveBatch(0, -1); }

    void onDeleteBatch(CCObject*) {
        if (!deleteBatch()) Notification::create("No batch to delete", NotificationIcon::Info)->show();
    }

    void onSettings(CCObject*) { openSettingsPopup(Mod::get(), false); }
};
