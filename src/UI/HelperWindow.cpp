#include "stdafx.h"
#include "UI/HelperWindow.h"
#include "Net/HelperProtocol.h"

namespace HelperWindow {
namespace {

constexpr int kWidth = 198;
constexpr int kHeight = 362;
constexpr int kX = 640 - kWidth - 12;
constexpr int kY = 62;
constexpr int kRowHeight = 20;

bool g_Open = false;
HelperProtocol::Settings g_Settings;
bool g_StartPending = false;
DWORD g_StartRequestedAt = 0;
bool g_InitialSkillSelected = false;

bool IsLearnedSkill(std::uint8_t skill)
{
    if (skill == 0) return true; // Basic attack
    const BYTE* attributes = reinterpret_cast<const BYTE*>(DAT_07cf1ff4);
    if (!attributes) return false;
    for (int slot = 0; slot < 20; ++slot) {
        if (attributes[0x57 + slot] == skill) return true;
    }
    return false;
}

std::uint8_t CurrentSkill()
{
    const BYTE* attributes = reinterpret_cast<const BYTE*>(DAT_07cf1ff4);
    const BYTE* hero = reinterpret_cast<const BYTE*>(DAT_07abf5d8);
    if (!attributes || !hero || hero[913] >= 20) return 0;
    const std::uint8_t skill = attributes[0x57 + hero[913]];
    return skill < 57 ? skill : 0;
}

std::uint8_t CycleLearnedSkill(std::uint8_t current, int direction)
{
    std::uint8_t skills[21] = {};
    int count = 1; // index zero is Basic Attack
    const BYTE* attributes = reinterpret_cast<const BYTE*>(DAT_07cf1ff4);
    if (attributes) {
        for (int slot = 0; slot < 20 && count < 21; ++slot) {
            const std::uint8_t skill = attributes[0x57 + slot];
            if (skill == 0 || skill >= 57) continue;
            bool duplicate = false;
            for (int i = 0; i < count; ++i) duplicate |= skills[i] == skill;
            if (!duplicate) skills[count++] = skill;
        }
    }

    int index = 0;
    for (int i = 0; i < count; ++i) {
        if (skills[i] == current) { index = i; break; }
    }
    index = (index + direction + count) % count;
    return skills[index];
}

const char* SkillName(std::uint8_t skill)
{
    if (skill == 0) return "Basic Attack";
    if (skill >= 64) return "Unknown";
    const char* name = reinterpret_cast<const char*>(SkillAttribute.Raw + skill * 0x28);
    return (name && name[0]) ? name : "Unknown";
}

const char* ResultName(std::uint8_t result)
{
    switch (result) {
        case HelperProtocol::ResultOk: return "OK";
        case HelperProtocol::ResultBadVersion: return "BAD VERSION";
        case HelperProtocol::ResultBadSettings: return "BAD SETTINGS";
        case HelperProtocol::ResultRateLimited: return "RATE LIMITED";
        case HelperProtocol::ResultNotReady: return "NOT READY";
        case HelperProtocol::ResultNotSupported: return "NOT SUPPORTED";
        case HelperProtocol::ResultSkillNotLearned: return "SKILL NOT LEARNED";
        default: return "UNKNOWN";
    }
}

void NormalizeSkills()
{
    if (!IsLearnedSkill(g_Settings.primarySkill)) g_Settings.primarySkill = CurrentSkill();
    if (!IsLearnedSkill(g_Settings.primarySkill)) g_Settings.primarySkill = 0;
    if (!IsLearnedSkill(g_Settings.secondarySkill)) g_Settings.secondarySkill = 0;
}

void ApplyAndStart()
{
    NormalizeSkills();
    if (!HelperProtocol::SendConfiguration(g_Settings)) return;
    // The server rate-limits Helper commands. Sending Start in the same frame
    // as Config makes the second packet fail, so finish the sequence later.
    g_StartPending = true;
    g_StartRequestedAt = GetTickCount();
}

bool Inside(int x, int y, int width, int height)
{
    const int mouseX = static_cast<int>(MouseX);
    const int mouseY = static_cast<int>(MouseY);
    return mouseX >= x && mouseX < x + width && mouseY >= y && mouseY < y + height;
}

bool Clicked(int x, int y, int width, int height)
{
    return Inside(x, y, width, height) && MouseLButton && MouseLButtonPush;
}

void ConsumeClick()
{
    MouseLButtonPush = 0;
    DAT_083a413c = 0;
    DAT_07e11d28 = 0;
    PlayBuffer(0x19, 0, 0);
}

void DrawBox(int x, int y, int width, int height, float r, float g, float b, float alpha)
{
    EnableAlphaTest(true);
    glColor4f(r, g, b, alpha);
    GL_DrawRect((float)x, (float)y, (float)width, (float)height);
    GL_ResetState();
    glColor4f(1, 1, 1, 1);
}

void DrawTextLine(int x, int y, int width, const char* text, int align = 0)
{
    RenderText(x, y, const_cast<char*>(text), width, align, nullptr);
}

void DrawButton(int x, int y, int width, const char* text, bool active = false)
{
    const bool hover = Inside(x, y, width, 20);
    DrawBox(x, y, width, 20, active ? 0.35f : 0.16f, active ? 0.55f : 0.18f,
            active ? 0.18f : 0.12f, hover ? 0.95f : 0.82f);
    DrawBox(x, y, width, 1, 0.78f, 0.58f, 0.20f, 1.0f);
    DrawTextLine(x, y + 4, width, text, 1);
}

void DrawValueRow(int row, const char* label, int value)
{
    const int y = kY + 42 + row * kRowHeight;
    char text[80];
    sprintf_s(text, "%s: %d", label, value);
    DrawTextLine(kX + 8, y + 3, 116, text);
    DrawButton(kX + 132, y, 24, "-");
    DrawButton(kX + 164, y, 24, "+");
}

void DrawSkillRow(int row, const char* label, std::uint8_t skill)
{
    const int y = kY + 42 + row * kRowHeight;
    char text[96];
    sprintf_s(text, "%s: %s (%u)", label, SkillName(skill), (unsigned int)skill);
    DrawTextLine(kX + 8, y + 3, 116, text);
    DrawButton(kX + 132, y, 24, "<");
    DrawButton(kX + 164, y, 24, ">");
}

void DrawToggleRow(int row, const char* label, bool value)
{
    const int y = kY + 42 + row * kRowHeight;
    char text[80];
    sprintf_s(text, "%s: %s", label, value ? "ON" : "OFF");
    DrawButton(kX + 8, y, kWidth - 16, text, value);
}

void ChangeValue(int row, std::uint8_t& value, int minimum, int maximum, int step)
{
    const int y = kY + 42 + row * kRowHeight;
    if (Clicked(kX + 132, y, 24, 18)) {
        value = (std::uint8_t)((value - minimum < step) ? maximum : value - step);
        ConsumeClick();
    } else if (Clicked(kX + 164, y, 24, 18)) {
        value = (std::uint8_t)((maximum - value < step) ? minimum : value + step);
        ConsumeClick();
    }
}

void ChangeSkill(int row, std::uint8_t& skill)
{
    const int y = kY + 42 + row * kRowHeight;
    if (Clicked(kX + 132, y, 24, 18)) {
        skill = CycleLearnedSkill(skill, -1);
        ConsumeClick();
    } else if (Clicked(kX + 164, y, 24, 18)) {
        skill = CycleLearnedSkill(skill, 1);
        ConsumeClick();
    }
}

void ToggleFlag(int row, std::uint8_t flag)
{
    const int y = kY + 42 + row * kRowHeight;
    if (Clicked(kX + 8, y, kWidth - 16, 20)) {
        g_Settings.flags ^= flag;
        ConsumeClick();
    }
}

void ToggleLootFlag(int row, std::uint8_t flag)
{
    const int y = kY + 42 + row * kRowHeight;
    if (Clicked(kX + 8, y, kWidth - 16, 20)) {
        g_Settings.lootFlags ^= flag;
        ConsumeClick();
    }
}

} // namespace

bool IsMouseOver()
{
    return g_Open && Inside(kX, kY, kWidth, kHeight);
}

void UpdateInput()
{
    if (SceneFlag == 5 && DAT_00559c84 == 0 && PressKey(VK_F5)) {
        g_Open = !g_Open;
        if (g_Open && !g_InitialSkillSelected) {
            g_Settings.primarySkill = CurrentSkill();
            g_InitialSkillSelected = true;
        }
        PlayBuffer(0x19, 0, 0);
    }

    if (SceneFlag != 5) {
        g_Open = false;
        g_StartPending = false;
        g_InitialSkillSelected = false;
        return;
    }

    if (DAT_00559c84 == 0 && PressKey(VK_F8)) ApplyAndStart();
    if (DAT_00559c84 == 0 && PressKey(VK_F9)) {
        g_StartPending = false;
        HelperProtocol::Stop();
    }

    if (g_StartPending && GetTickCount() - g_StartRequestedAt >= 350) {
        g_StartPending = false;
        HelperProtocol::Start();
    }

    if (!g_Open) return;

    ToggleFlag(0, HelperProtocol::AutoAttack);
    ChangeValue(1, g_Settings.huntingRange, 1, 10, 1);
    ChangeValue(2, g_Settings.hpThreshold, 0, 100, 5);
    ChangeValue(3, g_Settings.mpThreshold, 0, 100, 5);
    ChangeSkill(4, g_Settings.primarySkill);
    ChangeSkill(5, g_Settings.secondarySkill);
    ChangeValue(6, g_Settings.secondaryEvery, 1, 20, 1);
    ToggleFlag(7, HelperProtocol::AutoBuff);
    ToggleLootFlag(8, HelperProtocol::LootZen);
    ToggleLootFlag(9, HelperProtocol::LootJewels);
    ToggleLootFlag(10, HelperProtocol::LootExcellent);
    ChangeValue(11, g_Settings.lootRange, 1, 5, 1);

    const int buttonsY = kY + 42 + 12 * kRowHeight + 5;
    if (Clicked(kX + 8, buttonsY, 56, 20)) {
        HelperProtocol::SendConfiguration(g_Settings);
        ConsumeClick();
    } else if (Clicked(kX + 71, buttonsY, 56, 20)) {
        ApplyAndStart();
        ConsumeClick();
    } else if (Clicked(kX + 134, buttonsY, 56, 20)) {
        g_StartPending = false;
        HelperProtocol::Stop();
        ConsumeClick();
    }
}

void Render()
{
    if (!g_Open || SceneFlag != 5) return;

    const DWORD oldText = m_dwTextColor;
    const DWORD oldBack = m_dwBackColor;
    m_dwTextColor = 0xFFFFFFFFu;
    m_dwBackColor = 0;

    DrawBox(kX, kY, kWidth, kHeight, 0.025f, 0.025f, 0.035f, 0.94f);
    DrawBox(kX, kY, kWidth, 2, 0.83f, 0.63f, 0.21f, 1.0f);
    DrawBox(kX, kY + kHeight - 2, kWidth, 2, 0.45f, 0.30f, 0.08f, 1.0f);
    DrawTextLine(kX + 8, kY + 9, kWidth - 16, "MU HELPER  [F5 TO CLOSE]", 1);

    const HelperProtocol::Status& status = HelperProtocol::GetStatus();
    char state[96];
    sprintf_s(state, "%s  |  %s",
              status.received ? (status.active ? "RUNNING" : "STOPPED") : "NO STATUS",
              status.received ? ResultName(status.result) : "WAITING");
    DrawTextLine(kX + 8, kY + 27, kWidth - 16, state, 1);

    DrawToggleRow(0, "Auto Attack", (g_Settings.flags & HelperProtocol::AutoAttack) != 0);
    DrawValueRow(1, "Hunting Range", g_Settings.huntingRange);
    DrawValueRow(2, "HP Potion %", g_Settings.hpThreshold);
    DrawValueRow(3, "MP Potion %", g_Settings.mpThreshold);
    DrawSkillRow(4, "Primary", g_Settings.primarySkill);
    DrawSkillRow(5, "Secondary", g_Settings.secondarySkill);
    DrawValueRow(6, "Secondary Every", g_Settings.secondaryEvery);
    DrawToggleRow(7, "Auto Buff", (g_Settings.flags & HelperProtocol::AutoBuff) != 0);
    DrawToggleRow(8, "Loot Zen", (g_Settings.lootFlags & HelperProtocol::LootZen) != 0);
    DrawToggleRow(9, "Loot Jewels", (g_Settings.lootFlags & HelperProtocol::LootJewels) != 0);
    DrawToggleRow(10, "Loot Excellent", (g_Settings.lootFlags & HelperProtocol::LootExcellent) != 0);
    DrawValueRow(11, "Loot Range", g_Settings.lootRange);

    const int buttonsY = kY + 42 + 12 * kRowHeight + 5;
    DrawButton(kX + 8, buttonsY, 56, "APPLY");
    DrawButton(kX + 71, buttonsY, 56, "START", status.active);
    DrawButton(kX + 134, buttonsY, 56, "STOP");
    DrawTextLine(kX + 8, buttonsY + 26, kWidth - 16, "F8 Start  |  F9 Stop", 1);

    m_dwTextColor = oldText;
    m_dwBackColor = oldBack;
}

} // namespace HelperWindow
