#include <limits>

#include "Platform/RuntimeEnvironment.h"
#include "SDK/PalSignatures.h"
#include "Signatures.hpp"
#include "SigScanner/SinglePassSigScanner.hpp"
#include "Utility/Logging.h"
#include "Helpers/String.hpp"
#include "ASMHelper/ASMHelper.hpp"

using namespace RC;
using namespace RC::Unreal;

namespace Palworld {
    void SignatureManager::Initialize()
    {
        std::vector<SignatureContainer> SigContainerBox;
        SinglePassScanner::SignatureContainerMap SigContainerMap;

        for (auto& [ClassAndName, Signature] : Signatures)
        {
            SignatureContainer SigContainer = [=]() -> SignatureContainer {
                return {
                    {{Signature}},
                    [=](SignatureContainer& self) {
                        void* FunctionPointer = static_cast<void*>(self.get_match_address());

                        SignatureMap.emplace(ClassAndName, FunctionPointer);
                        PS::Log<LogLevel::Normal>(STR("Found {}: {}\n"), RC::to_generic_string(ClassAndName), FunctionPointer);

                        self.get_did_succeed() = true;

                        return true;
                    },
                    [=](const SignatureContainer& self) {
                        if (!self.get_did_succeed())
                        {
                            PS::Log<RC::LogLevel::Error>(STR("Failed to find signature for {}.\n"), RC::to_generic_string(ClassAndName));
                        }
                    }
                };
            }();
            SigContainerBox.emplace_back(SigContainer);
        }

        for (auto& [ClassAndName, Signature] : SignaturesCallResolve)
        {
            SignatureContainer SigContainer = [=]() -> SignatureContainer {
                return {
                    {{Signature}},
                    [=](SignatureContainer& self) {
                        void* FunctionPointer = static_cast<void*>(self.get_match_address());
                        void* FinalAddress = ASM::resolve_call(FunctionPointer);

                        SignatureMap.emplace(ClassAndName, FinalAddress);
                        PS::Log<LogLevel::Normal>(STR("Found {}: {}\n"), RC::to_generic_string(ClassAndName), FinalAddress);

                        self.get_did_succeed() = true;

                        return true;
                    },
                    [=](const SignatureContainer& self) {
                        if (!self.get_did_succeed())
                        {
                            PS::Log<RC::LogLevel::Error>(STR("Failed to find signature for {}.\n"), RC::to_generic_string(ClassAndName));
                        }
                    }
                };
            }();
            SigContainerBox.emplace_back(SigContainer);
        }

        SigContainerMap.emplace(ScanTarget::MainExe, SigContainerBox);
        const auto PreviousModuleSizeThreshold = SinglePassScanner::m_multithreading_module_size_threshold;
        if (PS::Platform::IsRunningUnderWine())
        {
            // SinglePassScanner uses std::async whenever the executable is
            // larger than this threshold. Wine can block while creating that
            // worker during UE4SS mod startup, before Unreal can initialize.
            // Force the scanner's existing synchronous path under Wine while
            // preserving parallel scanning on native Windows.
            SinglePassScanner::m_multithreading_module_size_threshold = std::numeric_limits<uint32_t>::max();
        }
        try
        {
            SinglePassScanner::start_scan(SigContainerMap);
        }
        catch (...)
        {
            SinglePassScanner::m_multithreading_module_size_threshold = PreviousModuleSizeThreshold;
            throw;
        }
        SinglePassScanner::m_multithreading_module_size_threshold = PreviousModuleSizeThreshold;
    }

    void* SignatureManager::GetSignature(const std::string& ClassAndFunction)
    {
        auto It = SignatureMap.find(ClassAndFunction);
        if (It != SignatureMap.end())
        {
            return It->second;
        }

        return nullptr;
    }
}
