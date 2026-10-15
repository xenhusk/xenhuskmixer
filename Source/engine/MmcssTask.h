#pragma once

#include <atomic>

namespace xenhusk
{

// Registers the calling thread with MMCSS. activate() and revert() must run on that same thread.
class MmcssTask
{
public:
    MmcssTask() = default;
    ~MmcssTask();

    MmcssTask(const MmcssTask&) = delete;
    MmcssTask& operator=(const MmcssTask&) = delete;

    bool activate();
    void revert();
    bool isActive() const noexcept;

private:
    void* taskHandle = nullptr;
    std::atomic<bool> active { false };
};

} // namespace xenhusk
