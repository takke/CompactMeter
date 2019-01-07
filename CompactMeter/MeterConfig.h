#pragma once

enum MeterId {
    METER_ID_UNKNOWN = 0,
    METER_ID_CPU = 100,
    METER_ID_CORES = 101,
    METER_ID_MEMORY = 200,
    METER_ID_NETWORK = 300,
    METER_ID_DRIVES = 400,
};

/**
 * メーターの設定値
 */
struct MeterConfig {
    MeterId id;

    // 当初は int(boolean) だったので互換性のため BOOL とする
    BOOL enable;
//    int backgroundColor;

    MeterConfig(MeterId id_, BOOL enable_ = true)
        : id(id_), enable(enable_)
    {}
    MeterConfig() : id(METER_ID_UNKNOWN), enable(false)
    {}

    LPCWSTR getName() const {

        switch (id) {
        case METER_ID_CPU:
            return L"CPU";
        case METER_ID_CORES:
            return L"Core";
        case METER_ID_MEMORY:
            return L"Memory";
        case METER_ID_NETWORK:
            return L"Network";
        case METER_ID_DRIVES:
            return L"Drives";
        case METER_ID_UNKNOWN:
        default:
            return L"Unknown";
        }
    }

    template<class Archive>
    void serialize(Archive & archive)
    {
        archive(CEREAL_NVP(id), CEREAL_NVP(enable));
    }
};

