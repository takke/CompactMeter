#pragma once

class FpsCounter
{
public:
    std::vector<DWORD> ticks;
    int start;  // 最古の要素のインデックス
    int last;   // 最新の要素のインデックス

public:
    FpsCounter()
        : start(0), last(0)
    {
        ticks.resize(50);

        // fill
        DWORD tick = GetTickCount();
        for (auto it = ticks.begin(); it != ticks.end(); it++) {
            *it = tick;
        }
    }

    ~FpsCounter()
    {
    }

    // 描画されたタイミングを記録する
    void CountOnDraw() {
        DWORD tick = GetTickCount();

        last++;
        if (last >= (int)ticks.size()) {
            last = 0;
        }
        ticks[last] = tick;

        if (start == last) {
            start++;
            if (start >= (int)ticks.size()) {
                start = 0;
            }
        }

    }

    int GetAverageFps() {

        int frames = ticks.size();
        int durationMs = GetTickCount() - ticks[start];

        return frames * 1000 / durationMs;
    }
};
