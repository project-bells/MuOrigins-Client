#pragma once

#include <cstdint>

namespace HelperProtocol {

constexpr std::uint8_t kHead = 0xFA;
constexpr std::uint8_t kVersion = 1;

enum Flags : std::uint8_t {
    AutoAttack = 0x01,
    AutoHp = 0x02,
    AutoMp = 0x04,
    AutoBuff = 0x08,
};

#pragma pack(push, 1)
struct Settings {
    std::uint8_t version = kVersion;
    std::uint8_t flags = AutoAttack | AutoHp | AutoMp;
    std::uint8_t huntingRange = 5;
    std::uint8_t hpThreshold = 50;
    std::uint8_t mpThreshold = 30;
    std::uint8_t primarySkill = 0;
    std::uint8_t secondarySkill = 0;
    std::uint8_t secondaryEvery = 5;
};

struct Status {
    bool received = false;
    bool active = false;
    std::uint8_t result = 0;
    std::uint16_t targetIndex = 0xFFFF;
    Settings settings{};
};
#pragma pack(pop)

const Status& GetStatus();
void Reset();
bool SendConfiguration(const Settings& settings);
void Start();
void Stop();
void RequestSync();
void RequestOfflineHunting();
void Receive(const std::uint8_t* packet, int size, std::uint8_t header, std::uint8_t subCode);

} // namespace HelperProtocol
