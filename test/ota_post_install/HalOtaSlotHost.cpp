#include <HalOtaSlot.h>

#include <cstring>
#include <vector>

namespace {
struct FakePartition {
  uint8_t subtype;
  size_t size;
};

FakePartition gInactivePartition{};
FakePartition gRunningPartition{};
bool gBootMatchesRunning = true;
int gRunningState = 0;  // ESP_OTA_IMG_VALID

constexpr int ESP_OTA_IMG_PENDING_VERIFY = 2;
constexpr int ESP_OTA_IMG_VALID = 3;
constexpr int ESP_OTA_IMG_UNDEFINED = 0;
constexpr int ESP_PARTITION_TYPE_APP = 1;
constexpr int ESP_PARTITION_SUBTYPE_APP_OTA_MIN = 0x10;
constexpr int ESP_PARTITION_SUBTYPE_APP_OTA_MAX = 0x1F;

HalOtaSlot::RunningImageState classify(int state) {
  switch (state) {
    case ESP_OTA_IMG_PENDING_VERIFY:
      return HalOtaSlot::RunningImageState::PendingVerify;
    case ESP_OTA_IMG_VALID:
      return HalOtaSlot::RunningImageState::Confirmed;
    case ESP_OTA_IMG_UNDEFINED:
      return HalOtaSlot::RunningImageState::Untracked;
    default:
      return HalOtaSlot::RunningImageState::Unsafe;
  }
}
}  // namespace

namespace OtaPostInstallTestHost {
int gConfirmCalls = 0;

void resetHalOtaSlotHost() {
  gInactivePartition = {};
  gRunningPartition = {ESP_PARTITION_SUBTYPE_APP_OTA_MIN, 1024 * 1024};
  gBootMatchesRunning = true;
  gRunningState = ESP_OTA_IMG_VALID;
  gConfirmCalls = 0;
}

void setInactiveSlotSize(size_t size) { gInactivePartition = {ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 1, size}; }

void setRunningPendingVerify() { gRunningState = ESP_OTA_IMG_PENDING_VERIFY; }

void setRunningConfirmed() { gRunningState = ESP_OTA_IMG_VALID; }

int confirmCallCount() { return gConfirmCalls; }

std::vector<uint8_t>& inactiveSlotBytes();

void overwriteInactiveSlot(size_t offset, const void* data, size_t length);
}  // namespace OtaPostInstallTestHost

HalOtaSlot HalOtaSlot::inactive() {
  if (gInactivePartition.size == 0) return {};
  return HalOtaSlot(&gInactivePartition, gInactivePartition.size);
}

HalOtaSlot::RunningImageState HalOtaSlot::runningImageState() {
  if (!gBootMatchesRunning) return RunningImageState::Unsafe;
  return classify(gRunningState);
}

bool HalOtaSlot::confirmRunningImage() {
  switch (runningImageState()) {
    case RunningImageState::Confirmed:
    case RunningImageState::Untracked:
      return true;
    case RunningImageState::PendingVerify:
      ++OtaPostInstallTestHost::gConfirmCalls;
      gRunningState = ESP_OTA_IMG_VALID;
      return runningImageState() == RunningImageState::Confirmed;
    case RunningImageState::Unsafe:
      return false;
  }
  return false;
}

bool HalOtaSlot::safeForScratchWrite() const {
  if (!valid()) return false;
  switch (runningImageState()) {
    case RunningImageState::Confirmed:
    case RunningImageState::Untracked:
      return true;
    case RunningImageState::PendingVerify:
    case RunningImageState::Unsafe:
      return false;
  }
  return false;
}

bool HalOtaSlot::read(size_t offset, void* data, size_t length) const {
  auto& bytes = OtaPostInstallTestHost::inactiveSlotBytes();
  if (!contains(offset, length) || offset + length > bytes.size()) return false;
  std::memcpy(data, bytes.data() + offset, length);
  return true;
}

bool HalOtaSlot::erase(size_t offset, size_t length) const {
  if (!contains(offset, length) || offset % ERASE_SIZE != 0 || length % ERASE_SIZE != 0) return false;
  auto& bytes = OtaPostInstallTestHost::inactiveSlotBytes();
  std::memset(bytes.data() + offset, 0xFF, length);
  return true;
}

bool HalOtaSlot::write(size_t offset, const void* data, size_t length) const {
  OtaPostInstallTestHost::overwriteInactiveSlot(offset, data, length);
  return contains(offset, length);
}

bool HalOtaSlot::contains(size_t offset, size_t length) const {
  return valid() && offset <= size_ && length <= size_ - offset;
}

namespace OtaPostInstallTestHost {
std::vector<uint8_t> gInactiveBytes;

std::vector<uint8_t>& inactiveSlotBytes() {
  if (gInactiveBytes.size() != gInactivePartition.size) {
    gInactiveBytes.assign(gInactivePartition.size, 0);
  }
  return gInactiveBytes;
}

void overwriteInactiveSlot(const size_t offset, const void* data, const size_t length) {
  auto& bytes = inactiveSlotBytes();
  if (offset + length <= bytes.size()) {
    std::memcpy(bytes.data() + offset, data, length);
  }
}
}  // namespace OtaPostInstallTestHost
