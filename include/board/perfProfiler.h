#pragma once

#include <chrono>
#include <string>

struct MakeUnmakeProfile;

class PerformanceProfiler {
private:
    MakeUnmakeProfile* profile = nullptr;

public:
    void startTimer(const std::string& operation);
    void endTimer(const std::string& operation);
    const MakeUnmakeProfile& getProfile() const;
};