#pragma once

namespace UECustom {
    struct FGuid {
        int A = 0;
        int B = 0;
        int C = 0;
        int D = 0;

        friend bool operator==(const UECustom::FGuid& X, const UECustom::FGuid& Y)
        {
            return ((X.A ^ Y.A) | (X.B ^ Y.B) | (X.C ^ Y.C) | (X.D ^ Y.D)) == 0;
        }

        friend bool operator!=(const UECustom::FGuid& X, const UECustom::FGuid& Y)
        {
            return ((X.A ^ Y.A) | (X.B ^ Y.B) | (X.C ^ Y.C) | (X.D ^ Y.D)) != 0;
        }

        friend bool operator<(const UECustom::FGuid& X, const UECustom::FGuid& Y)
        {
            return	((X.A < Y.A) ? true : ((X.A > Y.A) ? false :
                ((X.B < Y.B) ? true : ((X.B > Y.B) ? false :
                    ((X.C < Y.C) ? true : ((X.C > Y.C) ? false :
                        ((X.D < Y.D) ? true : ((X.D > Y.D) ? false : false))))))));
        }
    };
}

namespace std {
    template <>
    struct hash<UECustom::FGuid> {
        std::size_t operator()(const UECustom::FGuid& Guid) const noexcept {
            std::size_t h1 = std::hash<uint32_t>()(Guid.A);
            std::size_t h2 = std::hash<uint32_t>()(Guid.B);
            std::size_t h3 = std::hash<uint32_t>()(Guid.C);
            std::size_t h4 = std::hash<uint32_t>()(Guid.D);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };
}