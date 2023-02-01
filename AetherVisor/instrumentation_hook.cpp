#include "instrumentation_hook.h"

namespace Instrumentation
{
	void* callbacks[4];

	bool InvokeHook(VcpuData* vcpu_data, HOOK_ID handler, bool is_kernel)
	{
		auto vmroot_cr3 = __readcr3();

		__writecr3(vcpu_data->guest_vmcb.save_state_area.cr3.Flags);

		auto guest_rip = vcpu_data->guest_vmcb.save_state_area.rip;

		DbgPrint("guest_rip %p is_kernel %i \n", guest_rip, is_kernel);

		int callback_privilege = ((uintptr_t)callbacks[handler] > 0x7FFFFFFFFFFF) ? 3 : 0;

		int rip_privilege = (guest_rip > 0x7FFFFFFFFFFF) ? 3 : 0;

		if (callback_privilege == rip_privilege || handler == sandbox_readwrite)
		{
			vcpu_data->guest_vmcb.save_state_area.rip = (uintptr_t)callbacks[handler];

			vcpu_data->guest_vmcb.save_state_area.rsp -= 8;

			*(uintptr_t*)vcpu_data->guest_vmcb.save_state_area.rsp = guest_rip;
		}
		else
		{
			DbgPrint("ADDRESS SPACE MISMATCH \n");
			__writecr3(vmroot_cr3);

			return FALSE;
		}

		vcpu_data->guest_vmcb.control_area.ncr3 = Hypervisor::Get()->ncr3_dirs[primary];

		vcpu_data->guest_vmcb.control_area.vmcb_clean &= 0xFFFFFFEF;
		vcpu_data->guest_vmcb.control_area.tlb_control = 1;

		__writecr3(vmroot_cr3);

		return TRUE;
	}
};
