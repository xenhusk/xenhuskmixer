#include "MmcssTask.h"

#if JUCE_WINDOWS || defined(_WIN32)
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>
 #include <avrt.h>
#endif

namespace xenhusk
{

MmcssTask::~MmcssTask()
{
    revert();
}

bool MmcssTask::activate()
{
#if JUCE_WINDOWS || defined(_WIN32)
    if (taskHandle != nullptr)
        return true;

    DWORD taskIndex = 0;
    taskHandle = AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIndex);

    const bool armed = taskHandle != nullptr;
    active.store(armed, std::memory_order_relaxed);
    return armed;
#else
    active.store(false, std::memory_order_relaxed);
    return false;
#endif
}

void MmcssTask::revert()
{
#if JUCE_WINDOWS || defined(_WIN32)
    if (taskHandle != nullptr)
        AvRevertMmThreadCharacteristics(static_cast<HANDLE>(taskHandle));

    taskHandle = nullptr;
#endif
    active.store(false, std::memory_order_relaxed);
}

bool MmcssTask::isActive() const noexcept
{
    return active.load(std::memory_order_relaxed);
}

} // namespace xenhusk
