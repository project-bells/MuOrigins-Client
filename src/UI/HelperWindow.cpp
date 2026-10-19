#include "stdafx.h"
#include "UI/HelperWindow.h"
#include "Net/HelperProtocol.h"

namespace HelperWindow {
namespace {

constexpr int kX = 12;
constexpr int kY = 58;
constexpr int kWidth = 230;
constexpr int kHeight = 318;
constexpr int kRowHeight = 24;

bool g_Open = false;
HelperProtocol::Settings g_Settings;

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
    DrawTextLine(kX + 8, y + 4, 130, text);
    DrawButton(kX + 154, y, 28, "-");
    DrawButton(kX + 188, y, 28, "+");
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
    if (Clicked(kX + 154, y, 28, 20)) {
        value = (std::uint8_t)((value - minimum < step) ? maximum : value - step);
        ConsumeClick();
    } else if (Clicked(kX + 188, y, 28, 20)) {
        value = (std::uint8_t)((maximum - value < step) ? minimum : value + step);
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

} // namespace

bool IsMouseOver()
{
    return g_Open && Inside(kX, kY, kWidth, kHeight);
}

void UpdateInput()
{
    static bool hWasDown = false;
    const bool hDown = (GetAsyncKeyState('H') & 0x8000) != 0;
    if (SceneFlag == 5 && DAT_00559c84 == 0 && hDown && !hWasDown) {
        g_Open = !g_Open;
        PlayBuffer(0x19, 0, 0);
    }
    hWasDown = hDown;

    if (SceneFlag != 5) {
        g_Open = false;
        return;
    }
    if (!g_Open) return;

    ToggleFlag(0, HelperProtocol::AutoAttack);
    ChangeValue(1, g_Settings.huntingRange, 1, 10, 1);
    ChangeValue(2, g_Settings.hpThreshold, 0, 100, 5);
    ChangeValue(3, g_Settings.mpThreshold, 0, 100, 5);
    ChangeValue(4, g_Settings.primarySkill, 0, 56, 1);
    ChangeValue(5, g_Settings.secondarySkill, 0, 56, 1);
    ChangeValue(6, g_Settings.secondaryEvery, 1, 20, 1);
    ToggleFlag(7, HelperProtocol::AutoBuff);

    const int buttonsY = kY + 42 + 8 * kRowHeight + 6;
    if (Clicked(kX + 8, buttonsY, 66, 20)) {
        HelperProtocol::SendConfiguration(g_Settings);
        ConsumeClick();
    } else if (Clicked(kX + 82, buttonsY, 66, 20)) {
        HelperProtocol::Start();
        ConsumeClick();
    } else if (Clicked(kX + 156, buttonsY, 66, 20)) {
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
    DrawTextLine(kX + 8, kY + 9, kWidth - 16, "MU HELPER  [H TO CLOSE]", 1);

    const HelperProtocol::Status& status = HelperProtocol::GetStatus();
    char state[96];
    sprintf_s(state, "Server: %s  Result: %u",
              status.received ? (status.active ? "RUNNING" : "STOPPED") : "NO STATUS",
              (unsigned int)status.result);
    DrawTextLine(kX + 8, kY + 27, kWidth - 16, state, 1);

    DrawToggleRow(0, "Auto Attack", (g_Settings.flags & HelperProtocol::AutoAttack) != 0);
    DrawValueRow(1, "Hunting Range", g_Settings.huntingRange);
    DrawValueRow(2, "HP Potion %", g_Settings.hpThreshold);
    DrawValueRow(3, "MP Potion %", g_Settings.mpThreshold);
    DrawValueRow(4, "Primary Skill ID", g_Settings.primarySkill);
    DrawValueRow(5, "Secondary Skill ID", g_Settings.secondarySkill);
    DrawValueRow(6, "Secondary Every", g_Settings.secondaryEvery);
    DrawToggleRow(7, "Auto Buff", (g_Settings.flags & HelperProtocol::AutoBuff) != 0);

    const int buttonsY = kY + 42 + 8 * kRowHeight + 6;
    DrawButton(kX + 8, buttonsY, 66, "APPLY");
    DrawButton(kX + 82, buttonsY, 66, "START", status.active);
    DrawButton(kX + 156, buttonsY, 66, "STOP");
    DrawTextLine(kX + 8, buttonsY + 26, kWidth - 16, "Apply, wait for Result 0, then Start.", 1);

    m_dwTextColor = oldText;
    m_dwBackColor = oldBack;
}

} // namespace HelperWindow
