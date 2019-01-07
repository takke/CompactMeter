#pragma once

enum MeterId {
    METER_ID_UNKNOWN = 0,
    METER_ID_CPU = 100,
    METER_ID_CORES = 101,
    METER_ID_MEMORY = 200,
    METER_ID_NETWORK = 300,

    // all は v1.1.0 までの方式
    // v1.2.0 以降は各ドライブ別に設定する
    METER_ID_DRIVES_ALL_DEPRECATED = 400,
    METER_ID_DRIVE_A = 401,
    METER_ID_DRIVE_Z = 426,
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
        case METER_ID_DRIVES_ALL_DEPRECATED:
            return L"Drives(Deprecated)";
        case METER_ID_UNKNOWN:
        default:
            if (METER_ID_DRIVE_A <= id && id <= METER_ID_DRIVE_Z) {
                switch (id - METER_ID_DRIVE_A) {
                case  0: return L"Drive(A)";
                case  1: return L"Drive(B)";
                case  2: return L"Drive(C)";
                case  3: return L"Drive(D)";
                case  4: return L"Drive(E)";
                case  5: return L"Drive(F)";
                case  6: return L"Drive(G)";
                case  7: return L"Drive(H)";
                case  8: return L"Drive(I)";
                case  9: return L"Drive(J)";
                case 10: return L"Drive(K)";
                case 11: return L"Drive(L)";
                case 12: return L"Drive(M)";
                case 13: return L"Drive(N)";
                case 14: return L"Drive(O)";
                case 15: return L"Drive(P)";
                case 16: return L"Drive(Q)";
                case 17: return L"Drive(R)";
                case 18: return L"Drive(S)";
                case 19: return L"Drive(T)";
                case 20: return L"Drive(U)";
                case 21: return L"Drive(V)";
                case 22: return L"Drive(W)";
                case 23: return L"Drive(X)";
                case 24: return L"Drive(Y)";
                case 25: return L"Drive(Z)";
                }
            }
            return L"Unknown";
        }
    }

    template<class Archive>
    void serialize(Archive & archive)
    {
        archive(CEREAL_NVP(id), CEREAL_NVP(enable));
    }
};

