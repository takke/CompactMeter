#pragma once

class StopWatch {

private:
    std::vector<DWORD> durations;

    LARGE_INTEGER start, end;
    LARGE_INTEGER freq;

public:
    StopWatch() : freq({})
    {
    }

    void Start() {

        if (freq.QuadPart == 0) {
            QueryPerformanceFrequency(&freq);
        }

        QueryPerformanceCounter(&start);
    }

    void Stop() {

        QueryPerformanceCounter(&end);

        DWORD elapsedMicroseconds = (DWORD)((end.QuadPart - start.QuadPart) * 1000000 / freq.QuadPart);
        durations.push_back(elapsedMicroseconds);

        if (durations.size() >= 30) {
            durations.erase(durations.begin());
        }
    }

    DWORD GetAverageDurationMicroseconds() {

        if (durations.size() == 0) {
            return 0;
        }

        DWORD a = 0;

        for (const auto& v : durations) {
            a += v;
        }
        a /= durations.size();

        return a;
    }

};