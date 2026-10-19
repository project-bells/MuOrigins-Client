#include "stdafx.h"
#include "Net/HelperProtocol.h"
#include "Net/Network.h"

namespace HelperProtocol {
namespace {

enum SubCode : std::uint8_t {
    Config = 0x00,
    StartCommand = 0x01,
    StopCommand = 0x02,
    StatusMessage = 0x03,
    OfflineCommand = 0x04,
    SyncCommand = 0x05,
};

#pragma pack(push, 1)
struct ConfigPacket {
    std::uint8_t type;
    std::uint8_t size;
    std::uint8_t head;
    std::uint8_t subCode;
    Settings settings;
};

struct CommandPacket {
    std::uint8_t type;
    std::uint8_t size;
    std::uint8_t head;
    std::uint8_t subCode;
    std::uint8_t version;
};

struct StatusPacket {
    std::uint8_t type;
    std::uint8_t size;
    std::uint8_t head;
    std::uint8_t subCode;
    std::uint8_t version;
    std::uint8_t active;
    std::uint8_t result;
    std::uint8_t reserved;
    std::uint16_t targetIndex;
    Settings settings;
};
#pragma pack(pop)

static_assert(sizeof(ConfigPacket) == 14, "Helper config wire layout");
static_assert(sizeof(CommandPacket) == 5, "Helper command wire layout");
static_assert(sizeof(StatusPacket) == 20, "Helper status wire layout");

Status g_Status;

void SendCommand(std::uint8_t subCode)
{
    CommandPacket packet{0xC1, sizeof(CommandPacket), kHead, subCode, kVersion};
    gNetwork.SendC1(reinterpret_cast<const BYTE*>(&packet), sizeof(packet));
}

bool IsValid(const Settings& settings)
{
    constexpr std::uint8_t allowedFlags = AutoAttack | AutoHp | AutoMp | AutoBuff;
    return settings.version == kVersion && settings.huntingRange >= 1 && settings.huntingRange <= 10 &&
           settings.hpThreshold <= 100 && settings.mpThreshold <= 100 &&
           settings.primarySkill < 57 && settings.secondarySkill < 57 &&
           settings.secondaryEvery >= 1 && settings.secondaryEvery <= 20 &&
           settings.lootRange >= 1 && settings.lootRange <= 5 &&
           (settings.lootFlags & ~(LootZen | LootJewels | LootExcellent)) == 0 &&
           (settings.flags & ~allowedFlags) == 0;
}

} // namespace

const Status& GetStatus()
{
    return g_Status;
}

void Reset()
{
    g_Status = Status{};
}

bool SendConfiguration(const Settings& settings)
{
    if (!IsValid(settings)) return false;
    ConfigPacket packet{0xC1, sizeof(ConfigPacket), kHead, Config, settings};
    gNetwork.SendC1(reinterpret_cast<const BYTE*>(&packet), sizeof(packet));
    return true;
}

void Start() { SendCommand(StartCommand); }
void Stop() { SendCommand(StopCommand); }
void RequestSync() { SendCommand(SyncCommand); }
void RequestOfflineHunting() { SendCommand(OfflineCommand); }

void Receive(const std::uint8_t* packet, int size, std::uint8_t header, std::uint8_t subCode)
{
    if (packet == nullptr || header != 0xC1 || subCode != StatusMessage || size != sizeof(StatusPacket)) return;
    const StatusPacket* message = reinterpret_cast<const StatusPacket*>(packet);
    if (message->version != kVersion || message->settings.version != kVersion) return;
    g_Status.received = true;
    g_Status.active = message->active != 0;
    g_Status.result = message->result;
    g_Status.targetIndex = message->targetIndex;
    g_Status.settings = message->settings;
}

} // namespace HelperProtocol
