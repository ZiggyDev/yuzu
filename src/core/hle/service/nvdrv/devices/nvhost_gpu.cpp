// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/service/nvdrv/devices/nvhost_gpu.h"
#include "core/memory.h"
#include "video_core/gpu.h"
#include "video_core/memory_manager.h"

namespace Service::Nvidia::Devices {

nvhost_gpu::nvhost_gpu(Core::System& system, std::shared_ptr<nvmap> nvmap_dev)
    : nvdevice(system), nvmap_dev(std::move(nvmap_dev)) {}
nvhost_gpu::~nvhost_gpu() = default;

u32 nvhost_gpu::ioctl(Ioctl command, const std::vector<u8>& input, const std::vector<u8>& input2,
                      std::vector<u8>& output, std::vector<u8>& output2, IoctlCtrl& ctrl,
                      IoctlVersion version) {
    LOG_DEBUG(Service_NVDRV, "called, command=0x{:08X}, input_size=0x{:X}, output_size=0x{:X}",
              command.raw, input.size(), output.size());

    switch (static_cast<IoctlCommand>(command.raw)) {
    case IoctlCommand::IocSetNVMAPfdCommand:
        return SetNVMAPfd(input, output);
    case IoctlCommand::IocSetClientDataCommand:
        return SetClientData(input, output);
    case IoctlCommand::IocGetClientDataCommand:
        return GetClientData(input, output);
    case IoctlCommand::IocZCullBind:
        return ZCullBind(input, output);
    case IoctlCommand::IocSetErrorNotifierCommand:
        return SetErrorNotifier(input, output);
    case IoctlCommand::IocChannelSetPriorityCommand:
        return SetChannelPriority(input, output);
    case IoctlCommand::IocAllocGPFIFOEx2Command:
        return AllocGPFIFOEx2(input, output);
    case IoctlCommand::IocAllocObjCtxCommand:
        return AllocateObjectContext(input, output);
    case IoctlCommand::IocChannelGetWaitbaseCommand:
        return GetWaitbase(input, output);
    case IoctlCommand::IocChannelSetTimeoutCommand:
        return ChannelSetTimeout(input, output);
    case IoctlCommand::IocChannelSetTimeslice:
        return ChannelSetTimeslice(input, output);
    case IoctlCommand::IocSubmitGPFIFOExCommand:
        return SubmitGPFIFO(input, output, input2, version);
    default:
        break;
    }

    if (command.group == NVGPU_IOCTL_MAGIC) {
        if (command.cmd == NVGPU_IOCTL_CHANNEL_SUBMIT_GPFIFO) {
            return SubmitGPFIFO(input, output, input2, version);
        }
        if (command.cmd == NVGPU_IOCTL_CHANNEL_KICKOFF_PB) {
            return KickoffPB(input, output, input2, version);
        }
    }

    UNIMPLEMENTED_MSG("Unimplemented ioctl");
    return 0;
};

u32 nvhost_gpu::SetNVMAPfd(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlSetNvmapFD params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "called, fd={}", params.nvmap_fd);

    nvmap_fd = params.nvmap_fd;
    return 0;
}

u32 nvhost_gpu::SetClientData(const std::vector<u8>& input, std::vector<u8>& output) {
    LOG_DEBUG(Service_NVDRV, "called");

    IoctlClientData params{};
    std::memcpy(&params, input.data(), input.size());
    user_data = params.data;
    return 0;
}

u32 nvhost_gpu::GetClientData(const std::vector<u8>& input, std::vector<u8>& output) {
    LOG_DEBUG(Service_NVDRV, "called");

    IoctlClientData params{};
    std::memcpy(&params, input.data(), input.size());
    params.data = user_data;
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_gpu::ZCullBind(const std::vector<u8>& input, std::vector<u8>& output) {
    std::memcpy(&zcull_params, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "called, gpu_va={:X}, mode={:X}", zcull_params.gpu_va,
              zcull_params.mode);

    std::memcpy(output.data(), &zcull_params, output.size());
    return 0;
}

u32 nvhost_gpu::SetErrorNotifier(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlSetErrorNotifier params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service_NVDRV, "(STUBBED) called, offset={:X}, size={:X}, mem={:X}", params.offset,
                params.size, params.mem);

    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_gpu::SetChannelPriority(const std::vector<u8>& input, std::vector<u8>& output) {
    std::memcpy(&channel_priority, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "(STUBBED) called, priority={:X}", channel_priority);

    return 0;
}

u32 nvhost_gpu::AllocGPFIFOEx2(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlAllocGpfifoEx2 params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service_NVDRV,
                "(STUBBED) called, num_entries={:X}, flags={:X}, unk0={:X}, "
                "unk1={:X}, unk2={:X}, unk3={:X}",
                params.num_entries, params.flags, params.unk0, params.unk1, params.unk2,
                params.unk3);

    auto& gpu = system.GPU();
    params.fence_out.id = assigned_syncpoints;
    params.fence_out.value = gpu.GetSyncpointValue(assigned_syncpoints);
    assigned_syncpoints++;
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_gpu::AllocateObjectContext(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlAllocObjCtx params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service_NVDRV, "(STUBBED) called, class_num={:X}, flags={:X}", params.class_num,
                params.flags);

    params.obj_id = 0x0;
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_gpu::SubmitGPFIFO(const std::vector<u8>& input, std::vector<u8>& output,
                             const std::vector<u8>& input2, IoctlVersion version) {
    if (input.size() < sizeof(IoctlSubmitGpfifo)) {
        UNIMPLEMENTED();
    }
    IoctlSubmitGpfifo params{};
    std::memcpy(&params, input.data(), sizeof(IoctlSubmitGpfifo));
    LOG_TRACE(Service_NVDRV, "called, gpfifo={:X}, num_entries={:X}, flags={:X}", params.address,
              params.num_entries, params.flags.raw);

    Tegra::CommandList entries(params.num_entries);
    if (version == IoctlVersion::Version2) {
        ASSERT_MSG((input.size() + input2.size()) == sizeof(IoctlSubmitGpfifo) +
                                       params.num_entries * sizeof(Tegra::CommandListHeader),
                   "Incorrect input size");
        std::memcpy(entries.data(), input2.data(),
                    params.num_entries * sizeof(Tegra::CommandListHeader));
    } else {
        ASSERT_MSG(input.size() == sizeof(IoctlSubmitGpfifo) +
                                       params.num_entries * sizeof(Tegra::CommandListHeader),
                   "Incorrect input size");
        std::memcpy(entries.data(), &input[sizeof(IoctlSubmitGpfifo)],
                    params.num_entries * sizeof(Tegra::CommandListHeader));
    }

    UNIMPLEMENTED_IF(params.flags.add_wait.Value() != 0);
    // UNIMPLEMENTED_IF(params.flags.add_increment.Value() != 0);

    auto& gpu = system.GPU();
    u32 current_syncpoint_value = gpu.GetSyncpointValue(params.fence_out.id);
    if (params.flags.increment.Value() || params.flags.add_increment.Value()) {
        params.fence_out.value += current_syncpoint_value;
    } else {
        params.fence_out.value = current_syncpoint_value;
    }
    gpu.PushGPUEntries(std::move(entries));

    std::memcpy(output.data(), &params, sizeof(IoctlSubmitGpfifo));
    return 0;
}

u32 nvhost_gpu::KickoffPB(const std::vector<u8>& input, std::vector<u8>& output,
                          const std::vector<u8>& input2, IoctlVersion version) {
    if (input.size() < sizeof(IoctlSubmitGpfifo)) {
        UNIMPLEMENTED();
    }
    IoctlSubmitGpfifo params{};
    std::memcpy(&params, input.data(), sizeof(IoctlSubmitGpfifo));
    LOG_TRACE(Service_NVDRV, "called, gpfifo={:X}, num_entries={:X}, flags={:X}", params.address,
              params.num_entries, params.flags.raw);

    Tegra::CommandList entries(params.num_entries);
    if (version == IoctlVersion::Version2) {
        std::memcpy(entries.data(), input2.data(),
                    params.num_entries * sizeof(Tegra::CommandListHeader));
    } else {
        system.Memory().ReadBlock(params.address, entries.data(),
                                  params.num_entries * sizeof(Tegra::CommandListHeader));
    }
    UNIMPLEMENTED_IF(params.flags.add_wait.Value() != 0);
    UNIMPLEMENTED_IF(params.flags.add_increment.Value() != 0);

    auto& gpu = system.GPU();
    u32 current_syncpoint_value = gpu.GetSyncpointValue(params.fence_out.id);
    if (params.flags.increment.Value()) {
        params.fence_out.value += current_syncpoint_value;
    } else {
        params.fence_out.value = current_syncpoint_value;
    }
    gpu.PushGPUEntries(std::move(entries));

    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_gpu::GetWaitbase(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlGetWaitbase params{};
    std::memcpy(&params, input.data(), sizeof(IoctlGetWaitbase));
    LOG_INFO(Service_NVDRV, "called, unknown=0x{:X}", params.unknown);

    params.value = 0; // Seems to be hard coded at 0
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_gpu::ChannelSetTimeout(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlChannelSetTimeout params{};
    std::memcpy(&params, input.data(), sizeof(IoctlChannelSetTimeout));
    LOG_INFO(Service_NVDRV, "called, timeout=0x{:X}", params.timeout);

    return 0;
}

u32 nvhost_gpu::ChannelSetTimeslice(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlSetTimeslice params{};
    std::memcpy(&params, input.data(), sizeof(IoctlSetTimeslice));
    LOG_INFO(Service_NVDRV, "called, timeslice=0x{:X}", params.timeslice);

    channel_timeslice = params.timeslice;

    return 0;
}

} // namespace Service::Nvidia::Devices
